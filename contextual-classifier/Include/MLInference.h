// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include "AuxRoutines.h"
#include "Inference.h"
#include <vector>

#include <floret/fasttext.h>
#include <mutex>
#include <string>
#include <regex>

class MLInference : public Inference {
  public:
    MLInference(const std::string &ft_model_path);
    ~MLInference();
    CC_TYPE Classify(int process_pid) override;
  private:
    // Derived implementation using fastText.
    uint32_t Predict(int pid,
                     const std::map<std::string, std::string> &raw_data,
                     std::string &cat);

    fasttext::FastText ft_model_;
    std::mutex predict_mutex_;

    std::vector<std::string> classes_;
    std::vector<std::string> text_cols_;
    int embedding_dim_;

    // initialize a set having string that we can ignore.
    const std::set<std::string> REMOVE_KEYWORDS = {
        "unconfined", "user.slice", "user-n.slice", "user@n.service",
        "app.slice", "app-org.gnome.terminal.slice", "vte-spawn-n.scope",
        "usr", "bin", "lib"
    };
    
    const std::set<std::string> BROWSER_TERMS = {
        "httrack", "konqueror", "amfora", "luakit", "epiphany",
        "firefox", "chrome", "chromium", "webkit", "gecko", "safari",
        "opera", "brave", "vivaldi", "edge", "lynx", "w3m", "falkon"
    };
    
    std::regex user_slice_pattern_;
    std::regex user_service_pattern_;
    std::regex vte_spawn_pattern_;
    std::regex decimal_pattern_;
    std::regex hex_pattern_;
    std::regex long_number_pattern_;
    
    // Method to clean the text as same as we are doing in floret model building.
    std::string CleanTextPython(const std::string &input);

    void LogTopKPredictions(
        int pid,
        const std::vector<std::pair<fasttext::real, int32_t>>& predictions,
        int k);

};

#endif // ML_INFERENCE_H
