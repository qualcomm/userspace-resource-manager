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

MLInference::MLInference(const std::string &ft_model_path)
    : Inference(ft_model_path) {
    text_cols_ = {"attr", "cgroup",  "cmdline", "comm", "maps",
                  "fds",  "environ", "exe",     "logs"};

    syslog(LOG_DEBUG, "Loading fastText model from: %s", ft_model_path.c_str());
    try {
        ft_model_.loadModel(ft_model_path);
        embedding_dim_ = ft_model_.getDimension();
        syslog(LOG_DEBUG, "fastText model loaded. Embedding dimension: %d",
               embedding_dim_);
    } catch (const std::exception &e) {
        syslog(LOG_CRIT, "Failed to load fastText model: %s", e.what());
        throw;
    }

    syslog(LOG_INFO, "MLInference initialized. fastText dim: %d",
           embedding_dim_);
    (void)ft_model_path;
}

MLInference::~MLInference() = default;

std::string MLInference::normalize_text(const std::string &text) {
    std::string s = text;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

CC_TYPE MLInference::Classify(int process_pid) {
    //LOGD(CLASSIFIER_TAG,
    //     format_string("Starting classification for PID:%d", process_pid));

    const std::string proc_path = "/proc/" + std::to_string(process_pid);
    CC_TYPE contextType = CC_APP;
    std::map<std::string, std::string> raw_data;
    std::string predicted_label;

    auto start_collect = std::chrono::high_resolution_clock::now();
    int collect_rc = FeatureExtractor::CollectAndStoreData(
        process_pid, raw_data, false);
    auto end_collect = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed_collect =
        end_collect - start_collect;
    //LOGD(CLASSIFIER_TAG,
    //     format_string("Data collection for PID:%d took %f ms (rc=%d)",
    //                   process_pid, elapsed_collect.count(), collect_rc));

    if (collect_rc != 0) {
        // Process exited or collection failed; skip further work.
        return contextType;
    }

    //LOGD(CLASSIFIER_TAG,
    //     format_string("Text features collected for PID:%d", process_pid));

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
        //if (Inference) {
            uint32_t rc = predict(process_pid, raw_data, predicted_label);
            auto end_inference = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed_inference =
                end_inference - start_inference;
            LOGD(CLASSIFIER_TAG,
                 format_string("Inference for PID:%d took %f ms (rc=%u)",
                               process_pid, elapsed_inference.count(), rc));
            if (rc != 0) {
                // Inference failed, keep contextType as UNKNOWN.
                predicted_label.clear();
            }
        /*} else {
            LOGW(CLASSIFIER_TAG,
                 format_string("No Inference object available for PID:%d",
                               process_pid));
        }*/

        // Map stripped label -> CC_APP enum.
        // MLInference::predict() returns after stripping "__label__".
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


uint32_t MLInference::predict(
    int pid,
    const std::map<std::string, std::string> &raw_data,
    std::string &cat) {
    const std::lock_guard<std::mutex> lock(predict_mutex_);
    syslog(LOG_DEBUG, "Starting prediction.");

    std::string concatenated_text;
    for (const auto &col : text_cols_) {
        auto it = raw_data.find(col);
        if (it != raw_data.end()) {
            concatenated_text += normalize_text(it->second) + " ";
        } else {
            concatenated_text += " ";
        }
    }
    if (!concatenated_text.empty() && concatenated_text.back() == ' ') {
        concatenated_text.pop_back();
    }

    if (concatenated_text.empty()) {
        syslog(LOG_WARNING, "No text features found.");
        cat = "Unknown";
        return 1;
    }

    syslog(LOG_DEBUG, "Calling fastText predict().");

    concatenated_text += "\n";
    std::istringstream iss(concatenated_text);

    std::vector<std::pair<fasttext::real, int>> predictions;

    std::vector<int> words, labels;
    ft_model_.getDictionary()->getLine(iss, words, labels);

    ft_model_.predict(1, words, predictions, 0.0);

    if (predictions.empty()) {
        syslog(LOG_WARNING, "fastText returned no predictions.");
        cat = "Unknown";
        return 1;
    }

    fasttext::real probability = predictions[0].first;
    if (probability < 0) {
        probability = std::exp(probability);
    }
    int label_id = predictions[0].second;

    std::string predicted_label = ft_model_.getDictionary()->getLabel(label_id);

    std::string prefix = "__label__";
    if (predicted_label.rfind(prefix, 0) == 0) {
        predicted_label = predicted_label.substr(prefix.length());
    }

    std::string comm = "unknown";
    if (raw_data.count("comm")) {
        comm = raw_data.at("comm");
    }

    syslog(
        LOG_INFO,
        "Prediction complete. PID: %d, Comm: %s, Class: %s, Probability: %.4f",
        pid, comm.c_str(), predicted_label.c_str(), probability);

    cat = predicted_label;
    return 0;
}
