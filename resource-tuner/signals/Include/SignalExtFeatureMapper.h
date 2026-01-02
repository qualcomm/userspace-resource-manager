// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef SIGNAL_EXT_FEATURE_MAPPER_H
#define SIGNAL_EXT_FEATURE_MAPPER_H

#include <vector>
#include <unordered_map>
#include <memory>

#include "SignalRegistry.h"

class SignalExtFeatureMapper {
private:
    static std::shared_ptr<SignalExtFeatureMapper> signalExtFeatureMapperInstance;
    std::unordered_map<uint32_t, std::vector<uint32_t>> mSignalTofeaturesMap;

public:
    int8_t addFeature(uint64_t signal, int32_t feature);

    int8_t getFeatures(uint64_t signal, std::vector<uint32_t>& features);

    static std::shared_ptr<SignalExtFeatureMapper> getInstance() {
        if(signalExtFeatureMapperInstance == nullptr) {
            signalExtFeatureMapperInstance = std::shared_ptr<SignalExtFeatureMapper> (new SignalExtFeatureMapper());
        }
        return signalExtFeatureMapperInstance;
    }
};

#endif
