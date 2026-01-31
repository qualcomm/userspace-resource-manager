// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ExtFeaturesRegistry.h"

static void* openLib(const std::string& libPath) {
    void* handle = dlopen(libPath.c_str(), RTLD_LAZY);
    if(handle == nullptr) {
        TYPELOGV(EXT_FEATURE_CONFIGS_ELEM_LIB_OPEN_FAILED, libPath.c_str());
        return nullptr;
    }
    return handle;
}

std::shared_ptr<ExtFeaturesRegistry> ExtFeaturesRegistry::extFeaturesRegistryInstance = nullptr;

ExtFeaturesRegistry::ExtFeaturesRegistry() {
    this->mTotalExtFeatures = 0;
}

void ExtFeaturesRegistry::registerExtFeature(ExtFeatureInfo* featureInfo) {
    if(featureInfo == nullptr) {
        return;
    }

    if(featureInfo->mFeatureLib.length() == 0) {
        if(featureInfo->mSignalsSubscribedTo != nullptr) {
            delete featureInfo->mSignalsSubscribedTo;
        }
        delete featureInfo;
        return;
    }

    this->mSILMap[featureInfo->mFeatureId] = mTotalExtFeatures;
    this->mExtFeaturesConfigs.push_back(featureInfo);

    this->mTotalExtFeatures++;

    for(uint32_t signalCode: *featureInfo->mSignalsSubscribedTo) {
        SignalExtFeatureMapper::getInstance()->addFeature(signalCode, featureInfo->mFeatureId);
    }
}

std::vector<ExtFeatureInfo*> ExtFeaturesRegistry::getExtFeaturesConfigs() {
    return this->mExtFeaturesConfigs;
}

ExtFeatureInfo* ExtFeaturesRegistry::getExtFeatureConfigById(uint32_t featureId) {
    if(this->mSILMap.find(featureId) == this->mSILMap.end()) {
        LOGE("RESTUNE_EXT_FEATURES", "Ext Feature ID not found in the registry");
        return nullptr;
    }

    int32_t mExtFeaturesConfigsTableIndex = this->mSILMap[featureId];
    return this->mExtFeaturesConfigs[mExtFeaturesConfigsTableIndex];
}

int32_t ExtFeaturesRegistry::getExtFeaturesConfigCount() {
    return this->mExtFeaturesConfigs.size();
}

void ExtFeaturesRegistry::displayExtFeatures() {
    for(int32_t i = 0; i < this->mTotalExtFeatures; i++) {
        auto& extFeature = this->mExtFeaturesConfigs[i];

        LOGI("RESTUNE_EXT_FEATURES_REGISTRY", "Ext Feature ID: " + std::to_string(extFeature->mFeatureId));
        LOGI("RESTUNE_EXT_FEATURES_REGISTRY", "Ext Feature Name: " + extFeature->mFeatureName);
        LOGI("RESTUNE_EXT_FEATURES_REGISTRY", "Ext Feature Lib: " + extFeature->mFeatureLib);

        for(uint32_t signalCode: *extFeature->mSignalsSubscribedTo) {
            LOGI("RESTUNE_EXT_FEATURES_REGISTRY", "Ext Feature Signal ID: " + std::to_string(signalCode));
        }
    }
}

void ExtFeaturesRegistry::initializeFeatures() {
    for(int32_t i = 0; i < (int32_t)this->mExtFeaturesConfigs.size(); i++) {
        if(this->mExtFeaturesConfigs[i] == nullptr) continue;
        void* handle = openLib(this->mExtFeaturesConfigs[i]->mFeatureLib);

        if(handle != nullptr) {
            ExtFeature initCallback = (ExtFeature) dlsym(handle, INITIALIZE_FEATURE_ROUTINE);
            if(initCallback != nullptr) {
                initCallback();
            } else {
                TYPELOGV(EXT_FEATURE_ROUTINE_NOT_DEFINED,
                         INITIALIZE_FEATURE_ROUTINE,
                         this->mExtFeaturesConfigs[i]->mFeatureLib.c_str());
            }
        } else {
            LOGE("RESTUNE_EXT_FEATURES", "Error while opening Ext Feature Library");
        }
    }
}

void ExtFeaturesRegistry::teardownFeatures() {
    for(int32_t i = 0; i < (int32_t)this->mExtFeaturesConfigs.size(); i++) {
        if(this->mExtFeaturesConfigs[i] == nullptr) continue;
        void* handle = openLib(this->mExtFeaturesConfigs[i]->mFeatureLib);

        if(handle != nullptr) {
            ExtFeature tearCallback = (ExtFeature) dlsym(handle, TEARDOWN_FEATURE_ROUTINE);
            if(tearCallback != nullptr) {
                tearCallback();
            } else {
                TYPELOGV(EXT_FEATURE_ROUTINE_NOT_DEFINED,
                         TEARDOWN_FEATURE_ROUTINE,
                         this->mExtFeaturesConfigs[i]->mFeatureLib.c_str());
            }
            dlclose(handle);
        } else {
            LOGE("RESTUNE_EXT_FEATURES", "Error while opening Ext Feature Library");
        }
    }
}

ErrCode ExtFeaturesRegistry::relayToFeature(uint32_t featureId, Signal* signal) {
    ExtFeatureInfo* extFeatureInfo = this->getExtFeatureConfigById(featureId);
    if(extFeatureInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    void* handle = openLib(extFeatureInfo->mFeatureLib);

    if(handle != nullptr) {
        RelayFeature relayCallback = (RelayFeature) dlsym(handle, RELAY_FEATURE_ROUTINE);
        if(relayCallback != nullptr) {
            relayCallback(signal->getSignalCode(),
                          signal->getAppName(),
                          signal->getScenario(),
                          signal->getNumArgs(),
                          signal->getListArgs());
        } else {
            TYPELOGV(EXT_FEATURE_ROUTINE_NOT_DEFINED,
                     RELAY_FEATURE_ROUTINE,
                     extFeatureInfo->mFeatureLib.c_str());
        }

        return RC_SUCCESS;

    } else {
        LOGE("RESTUNE_EXT_FEATURES", "Error while opening Ext Feature Library");
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ExtFeaturesRegistry::~ExtFeaturesRegistry() {
    for(int32_t i = 0; i < (int32_t)this->mExtFeaturesConfigs.size(); i++) {
        delete(this->mExtFeaturesConfigs[i]);
        this->mExtFeaturesConfigs[i] = nullptr;
    }
}

ExtFeatureInfoBuilder::ExtFeatureInfoBuilder() {
    this->mFeatureInfo = new(std::nothrow) ExtFeatureInfo();
}

ErrCode ExtFeatureInfoBuilder::setId(const std::string& mFeatureId) {
    if(this->mFeatureInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    this->mFeatureInfo->mFeatureId = 0;
    try {
        this->mFeatureInfo->mFeatureId = (uint32_t)stol(mFeatureId, nullptr, 0);
    } catch(const std::invalid_argument& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;

    } catch(const std::out_of_range& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

 ErrCode ExtFeatureInfoBuilder::setName(const std::string& featureName) {
    if(this->mFeatureInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    this->mFeatureInfo->mFeatureName = featureName;
    return RC_SUCCESS;
 }

ErrCode ExtFeatureInfoBuilder::setLib(const std::string& featureLib) {
    if(this->mFeatureInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    this->mFeatureInfo->mFeatureLib = featureLib;
    return RC_SUCCESS;
}

ErrCode ExtFeatureInfoBuilder::addSignalSubscribedTo(const std::string& sigCodeString) {
    if(this->mFeatureInfo == nullptr || sigCodeString.length() == 0) {
        return RC_INVALID_VALUE;
    }

    uint32_t sigCode = 0;
    try {
        sigCode = (uint32_t)stol(sigCodeString, nullptr, 0);
    } catch(const std::invalid_argument& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;

    } catch(const std::out_of_range& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;
    }

    if(this->mFeatureInfo->mSignalsSubscribedTo == nullptr) {
        this->mFeatureInfo->mSignalsSubscribedTo = new(std::nothrow) std::vector<uint32_t>;
    }

    if(this->mFeatureInfo->mSignalsSubscribedTo == nullptr) {
        return RC_INVALID_VALUE;
    }

    try {
        this->mFeatureInfo->mSignalsSubscribedTo->push_back(sigCode);
    } catch(const std::bad_alloc& e) {
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ExtFeatureInfo* ExtFeatureInfoBuilder::build() {
    return this->mFeatureInfo;
}
