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

class MLInference : public Inference {
  public:
    MLInference(const std::string &ft_model_path);
    ~MLInference();
    CC_TYPE Classify(int process_pid) override;
  private:
    // Derived implementation using fastText.
    uint32_t predict(int pid,
                     const std::map<std::string, std::string> &raw_data,
                     std::string &cat);
    fasttext::FastText ft_model_;
    std::mutex predict_mutex_;

    std::vector<std::string> classes_;
    std::vector<std::string> text_cols_;
    int embedding_dim_;

    std::string normalize_text(const std::string &text);

};

#endif // ML_INFERENCE_H
