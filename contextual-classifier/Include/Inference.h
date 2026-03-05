// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef INFERENCE_H
#define INFERENCE_H

#include "ContextualClassifier.h"
#include <cstdint>
#include <map>
#include <string>

class Inference {
public:
    explicit Inference(const std::string &model_path) : model_path_(model_path) {}
    virtual ~Inference() = default;

    virtual CC_TYPE Classify(pid_t pid) {
        (void)pid;
        // Base implementation: no ML, just return default".
        return CC_APP;
    }

protected:
    std::string model_path_;
};

#endif // INFERENCE_H
