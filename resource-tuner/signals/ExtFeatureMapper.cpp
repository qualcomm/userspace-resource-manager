// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SignalExtFeatureMapper.h"

std::shared_ptr<SignalExtFeatureMapper> SignalExtFeatureMapper::signalExtFeatureMapperInstance = nullptr;

int8_t SignalExtFeatureMapper::addFeature(uint64_t signalCode, int32_t feature) {
    if(SignalRegistry::getInstance()->getSignalConfigById(signalCode) == nullptr) {
        return false;
    }

    this->mSignalTofeaturesMap[signalCode].push_back(feature);
    return true;
}

int8_t SignalExtFeatureMapper::getFeatures(uint64_t signalCode, std::vector<uint32_t>& features) {
    if(this->mSignalTofeaturesMap.find(signalCode) == this->mSignalTofeaturesMap.end()) {
        return false;
    }

    features = mSignalTofeaturesMap[signalCode];
    return true;
}
