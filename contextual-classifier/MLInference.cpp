// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "MLInference.h"
#include "ContextualClassifier.h"
#include "FeatureExtractor.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <syslog.h>
#include <vector>

#define CLASSIFIER_TAG "MLInference"

static std::string format_string(const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return std::string(buffer);
}

MLInference::MLInference(const std::string &ft_model_path) : Inference(ft_model_path) {
    
    text_cols_ = {
        "attr",                                                  // 1x weight
        "cgroup",                                                // 1x weight
        "cmdline", "cmdline", "cmdline", "cmdline", "cmdline",   // 5x weight
        "comm", "comm", "comm", "comm", "comm",                  // 5x weight
        "maps", "maps",                                          // 2x weight
        "fds",                                                   // 1x weight
        "environ",                                               // 1x weight
        "exe", "exe", "exe", "exe", "exe",                       // 5x weight
        "logs"                                                   // 1x weight
    };

    // Initialize regex patterns
    user_slice_pattern_   = std::regex("^user-\\d+\\.slice$");
    user_service_pattern_ = std::regex("^user@\\d+\\.service$");
    vte_spawn_pattern_    = std::regex("^vte-spawn-.*\\.scope$");
    decimal_pattern_      = std::regex("^\\d+(\\.\\d+)?$");
    hex_pattern_          = std::regex("0x[a-f0-9]+", std::regex::icase);
    long_number_pattern_  = std::regex("\\d{4,}");

    syslog(LOG_DEBUG, "Loading Floret model from: %s", ft_model_path.c_str());
    try {
        ft_model_.loadModel(ft_model_path);
        syslog(LOG_DEBUG, "Floret model Successfully loaded");

        embedding_dim_ = ft_model_.getDimension();
        syslog(LOG_DEBUG, "Floret model loaded. Embedding dimension: %d", embedding_dim_);

    } catch (const std::exception &e) {
        syslog(LOG_CRIT, "Failed to load Floret model: %s", e.what());
        throw;
    }

    syslog(LOG_INFO, "MLInference initialized. Floret dim: %d", embedding_dim_);
    (void)ft_model_path;
}

MLInference::~MLInference() = default;

std::string MLInference::CleanTextPython(const std::string &input) {
    if (input.empty()) {
        return "";
    }
    
    // Step 1: Convert to lowercase
    std::string line = input;
    std::transform(line.begin(), line.end(), line.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Step 2: Replace commas with spaces
    std::replace(line.begin(), line.end(), ',', ' ');

    // Replace brackets with spaces: [](){}
    for (char& c : line) {
        if (c == '[' || c == ']' || c == '(' || c == ')' || c == '{' || c == '}') {
            c = ' ';
        }
    }
    
    // Step 3: Split into tokens
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    // Step 4: Clean tokens (Python logic)
    std::vector<std::string> clean_tokens;
    std::set<std::string> seen;  // Track duplicates
    
    for (const auto& tok : tokens) {
        std::string t = tok;
        
        // Trim whitespace
        t.erase(0, t.find_first_not_of(" \t\n\r"));
        t.erase(t.find_last_not_of(" \t\n\r") + 1);
        
        if (t.empty()) {
            continue;
        }
        
        // Skip if in REMOVE_KEYWORDS
        if (REMOVE_KEYWORDS.find(t) != REMOVE_KEYWORDS.end()) {
            continue;
        }
        
        // Skip if matches user-N.slice pattern
        if (std::regex_match(t, user_slice_pattern_)) {
            continue;
        }
        
        // Skip if matches user@N.service pattern
        if (std::regex_match(t, user_service_pattern_)) {
            continue;
        }
        
        // Skip if matches vte-spawn-*.scope pattern
        if (std::regex_match(t, vte_spawn_pattern_)) {
            continue;
        }
        
        // Skip if it's just a number (integer or decimal)
        if (std::regex_match(t, decimal_pattern_)) {
            continue;
        }
        
        // Replace hex values with <hex>
        t = std::regex_replace(t, hex_pattern_, "<hex>");
        
        // Replace long numbers (4+ digits) with <num>
        t = std::regex_replace(t, long_number_pattern_, "<num>");
        
        // Keep important browser-related terms (even if duplicate)
        bool is_browser_term = false;
        for (const auto& browser_term : BROWSER_TERMS) {
            if (t.find(browser_term) != std::string::npos) {
                is_browser_term = true;
                break;
            }
        }
        
        if (is_browser_term) {
            clean_tokens.push_back(t);
            continue;  // Skip duplicate check for browser terms
        }
        if(!t.empty() && t.length() > 1){
            clean_tokens.push_back(t);
        }
    }
    
    // Step 5: Join tokens with spaces
    std::ostringstream result;
    for (size_t i = 0; i < clean_tokens.size(); ++i) {
        if (i > 0) {
            result << " ";
        }
        result << clean_tokens[i];
    }
    
    return result.str();
}

CC_TYPE MLInference::Classify(int process_pid) {
    LOGD(CLASSIFIER_TAG,
         format_string("Starting classification for PID:%d", process_pid));

    const std::string proc_path = "/proc/" + std::to_string(process_pid);
    CC_TYPE contextType = CC_APP;
    std::map<std::string, std::string> raw_data;
    std::string predicted_label;

    auto start_collect = std::chrono::high_resolution_clock::now();
    int collect_rc     = FeatureExtractor::CollectAndStoreData(process_pid,
                                                               raw_data,
                                                               false);

    auto end_collect = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed_collect =
        end_collect - start_collect;

    if (collect_rc != 0) {
        // Process exited or collection failed; skip further work.
        return contextType;
    }

    LOGD(CLASSIFIER_TAG,
         format_string("Text features collected for PID:%d", process_pid));

    if (!AuxRoutines::fileExists(proc_path)) {
        return contextType;
    }

    bool has_sufficient_features = false;
    for (const auto &kv : raw_data) {
        if (!kv.second.empty()) {
            has_sufficient_features = true;
            break;
        }
    }

    if(has_sufficient_features) {
        if(!AuxRoutines::fileExists(proc_path)) {
            return contextType;
        }

        LOGD(CLASSIFIER_TAG,
             format_string("Invoking ML inference for PID:%d", process_pid));

        auto start_inference = std::chrono::high_resolution_clock::now();

        uint32_t rc = Predict(process_pid, raw_data, predicted_label);
        auto end_inference = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed_inference = end_inference - start_inference;
        LOGD(CLASSIFIER_TAG, format_string("Inference for PID:%d took %f ms (rc=%u)",
                            process_pid, elapsed_inference.count(), rc));
        if (rc != 0) {
            // Inference failed, keep contextType as UNKNOWN.
            predicted_label.clear();
        }

        if (predicted_label == "app") {
            contextType = CC_APP;
        } else if (predicted_label == "browser") {
            contextType = CC_BROWSER;
        } else if (predicted_label == "game") {
            contextType = CC_GAME;
        } else if (predicted_label == "media") {
            contextType = CC_MULTIMEDIA;
        } else {
            contextType = CC_APP;
        }

        LOGD(CLASSIFIER_TAG,
             format_string("Predicted label '%s' mapped to contextType=%d",
                           predicted_label.c_str(),
                           static_cast<int>(contextType)));
    } else {
        LOGD(CLASSIFIER_TAG,
             format_string("Skipping ML inference for PID:%d due to "
                           "insufficient features.",
                           process_pid));
    }

    return contextType;
}

uint32_t MLInference::Predict(int pid,
                              const std::map<std::string, std::string> &raw_data,
                              std::string &cat) {
    
    std::lock_guard<std::mutex> lock(predict_mutex_);

    syslog(LOG_DEBUG, "Starting prediction for PID: %d", pid);

    // Build concatenated text
    std::string concatenated_text;
    for (const auto &col : text_cols_) {
        auto it = raw_data.find(col);
        if (it != raw_data.end()) {
            concatenated_text += it->second + " ";
        } else {
            concatenated_text += " ";
        }
    }
    
    if (!concatenated_text.empty() && concatenated_text.back() == ' ') {
        concatenated_text.pop_back();
    }

    if (concatenated_text.empty()) {
        syslog(LOG_WARNING, "No text features found for PID: %d", pid);
        cat = "Unknown";
        return 1;
    }
    

    // Apply cleaning same what we did during building model
    std::string cleaned_text = CleanTextPython(concatenated_text);

    if (cleaned_text.empty()) {
        syslog(LOG_WARNING, "Text became empty after cleaning for PID: %d", pid);
        cat = "Unknown";
        return 1;
    }

    // Prepare for prediction
    cleaned_text += "\n";
    std::istringstream iss(cleaned_text);

    // Use fasttext types (provided by Floret)
    const int k = 3;
    std::vector<std::pair<fasttext::real, int32_t>> predictions;
    predictions.reserve(k);
    
    std::vector<int32_t> words, labels;
    words.reserve(100);
    labels.reserve(10);

    // Convert text to word IDs
    ft_model_.getDictionary()->getLine(iss, words, labels);
    
    if (words.empty()) {
        syslog(LOG_WARNING, "No words extracted from text for PID: %d", pid);
        cat = "Unknown";
        return 1;
    }

    // Make prediction
    const fasttext::real threshold = 0.0;
    ft_model_.predict(k, words, predictions, threshold);

    if (predictions.empty()) {
        syslog(LOG_WARNING, "Floret returned no predictions for PID: %d", pid);
        cat = "Unknown";
        return 1;
    }

    // Extract top prediction
    fasttext::real probability = predictions[0].first;
    
    // Convert log probability to actual probability
    if (probability < 0) {
        probability = std::exp(probability);
    }
    int32_t label_id = predictions[0].second;

    // Get label string
    std::string predicted_label = ft_model_.getDictionary()->getLabel(label_id);

    // Remove "__label__" prefix
    const std::string prefix = "__label__";
    if (predicted_label.compare(0, prefix.length(), prefix) == 0) {
        predicted_label = predicted_label.substr(prefix.length());
    }

    // Get comm for logging
    std::string comm = "unknown";
    auto comm_it = raw_data.find("comm");
    if (comm_it != raw_data.end()) {
        comm = comm_it->second;
    }

    syslog(
        LOG_INFO,
        "Prediction complete. PID: %d, Comm: %s, Class: %s, Probability: %.4f",
        pid, comm.c_str(), predicted_label.c_str(), static_cast<float>(probability));

    cat = predicted_label;
    return 0;
}
