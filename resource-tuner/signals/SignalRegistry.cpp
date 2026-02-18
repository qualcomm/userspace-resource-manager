// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SignalRegistry.h"

static const int32_t unsupportedResoure = -2;

static void freeSignalConfig(SignalInfo* signalInfo) {
    if(signalInfo != nullptr) {
        if(signalInfo->mPermissions != nullptr) {
            delete signalInfo->mPermissions;
            signalInfo->mPermissions = nullptr;
        }

        if(signalInfo->mDerivatives != nullptr) {
            delete signalInfo->mDerivatives;
            signalInfo->mDerivatives = nullptr;
        }

        if(signalInfo->mSignalResources != nullptr) {
            for(int32_t i = 0; i < (int32_t)signalInfo->mSignalResources->size(); i++) {
                delete (*signalInfo->mSignalResources)[i];
            }

            delete signalInfo->mSignalResources;
            signalInfo->mSignalResources = nullptr;
        }

        delete signalInfo;
    }
}

std::shared_ptr<SignalRegistry> SignalRegistry::signalRegistryInstance = nullptr;

SignalRegistry::SignalRegistry() {
    this->mTotalSignals = 0;
}

int8_t SignalRegistry::isSignalConfigMalformed(SignalInfo* sConf) {
    if(sConf == nullptr) return true;
    if(sConf->mSignalCategory == 0) return true;
    return false;
}

void SignalRegistry::registerSignal(SignalInfo* signalInfo) {
    if(this->isSignalConfigMalformed(signalInfo)) {
        freeSignalConfig(signalInfo);
        signalInfo = nullptr;
        return;
    }

    uint64_t signalBitmap = 0;
    signalBitmap |= ((uint32_t)signalInfo->mSignalID);
    signalBitmap |= ((uint32_t)signalInfo->mSignalCategory << 16);

    // Add the sub-type
    signalBitmap <<= 32; // Make Room
    signalBitmap |= ((uint32_t)signalInfo->mSigType);

    // Check for any conflict
    if(this->mSILMap.find(signalBitmap) != this->mSILMap.end()) {

        // Signal with the specified Category and SigID already exists
        // Overwrite it.
        int32_t signalTableIndex = this->getSignalTableIndex(signalBitmap);
        this->mSignalsConfigs[signalTableIndex] = signalInfo;

        this->mSILMap.erase(signalBitmap);
        this->mSILMap[signalBitmap] = signalTableIndex;

    } else {
        this->mSILMap[signalBitmap] = this->mTotalSignals;
        this->mSignalsConfigs.push_back(signalInfo);

        this->mTotalSignals++;
    }
}

std::vector<SignalInfo*> SignalRegistry::getSignalConfigs() {
    return this->mSignalsConfigs;
}

SignalInfo* SignalRegistry::getSignalConfigById(uint64_t sigCode) {
    if(this->mSILMap.find(sigCode) == this->mSILMap.end()) {
        TYPELOGV(SIGNAL_REGISTRY_SIGNAL_NOT_FOUND, GET_SIGNAL_ID(sigCode), GET_SIGNAL_TYPE(sigCode));
        return nullptr;
    }

    int32_t mResourceTableIndex = this->mSILMap[sigCode];
    return this->mSignalsConfigs[mResourceTableIndex];
}

SignalInfo* SignalRegistry::getSignalConfigById(uint32_t sigId, uint32_t sigType) {
    // Create the 64-bit index
    uint64_t signalBitmap = (uint64_t)sigId;

    // Add the sub-type
    signalBitmap <<= 32; // Make Room
    signalBitmap |= sigType;

    if(this->mSILMap.find(signalBitmap) == this->mSILMap.end()) {
        TYPELOGV(SIGNAL_REGISTRY_SIGNAL_NOT_FOUND, sigId, sigType);
        return nullptr;
    }

    int32_t mResourceTableIndex = this->mSILMap[signalBitmap];
    return this->mSignalsConfigs[mResourceTableIndex];
}

int32_t SignalRegistry::getSignalsConfigCount() {
    return this->mTotalSignals;
}

int32_t SignalRegistry::getSignalTableIndex(uint64_t signalCode) {
    if(this->mSILMap.find(signalCode) == this->mSILMap.end()) {
        return -1;
    }

    return this->mSILMap[signalCode];
}

SignalRegistry::~SignalRegistry() {
    for(int32_t i = 0; i < this->mTotalSignals; i++) {
        freeSignalConfig(this->mSignalsConfigs[i]);
    }
}

SignalInfoBuilder::SignalInfoBuilder() {
    this->mTargetRefCount = 0;
    this->mSignalInfo = new(std::nothrow) SignalInfo;
    if(this->mSignalInfo == nullptr) {
        return;
    }

    this->mSignalInfo->mSignalID = 0;
    this->mSignalInfo->mSignalCategory = 0;
    this->mSignalInfo->mSigType = 0;
    this->mSignalInfo->mSignalName = "";
    this->mSignalInfo->mTimeout = 1;

    this->mSignalInfo->mDerivatives = nullptr;
    this->mSignalInfo->mPermissions = nullptr;
    this->mSignalInfo->mSignalResources = nullptr;
}

ErrCode SignalInfoBuilder::setSignalID(const std::string& signalIdString) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mSignalInfo->mSignalID = 0;
    try {
        this->mSignalInfo->mSignalID = (uint16_t)stoi(signalIdString, nullptr, 0);

    } catch(const std::invalid_argument& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;

    } catch(const std::out_of_range& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode SignalInfoBuilder::setSignalCategory(const std::string& categoryString) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mSignalInfo->mSignalCategory = 0;
    try {
        this->mSignalInfo->mSignalCategory = (uint8_t)stoi(categoryString, nullptr, 0);

    } catch(const std::invalid_argument& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;

    } catch(const std::out_of_range& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode SignalInfoBuilder::setSignalType(const std::string& typeString) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mSignalInfo->mSigType = 0;

    try {
        this->mSignalInfo->mSigType = (uint32_t)stol(typeString, nullptr, 0);

    } catch(const std::invalid_argument& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;

    } catch(const std::out_of_range& e) {
        TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode SignalInfoBuilder::setName(const std::string& signalName) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mSignalInfo->mSignalName = signalName;
    return RC_SUCCESS;
}

ErrCode SignalInfoBuilder::setTimeout(const std::string& timeoutString) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    try {
        this->mSignalInfo->mTimeout = std::stoi(timeoutString);
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_INVALID_VALUE;
}

ErrCode SignalInfoBuilder::setIsEnabled(const std::string& isEnabledString) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    if(isEnabledString != "true") {
        this->mTargetRefCount = unsupportedResoure;
    }
    return RC_SUCCESS;
}

ErrCode SignalInfoBuilder::addPermission(const std::string& permissionString) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    if(this->mSignalInfo->mPermissions == nullptr) {
        this->mSignalInfo->mPermissions = new(std::nothrow) std::vector<enum Permissions>;
    }

    enum Permissions permission = PERMISSION_THIRD_PARTY;
    if(permissionString == "system") {
        permission = PERMISSION_SYSTEM;
    } else if(permissionString == "third_party") {
        permission = PERMISSION_THIRD_PARTY;
    } else {
        if(permissionString.length() != 0) {
            return RC_INVALID_VALUE;
        }
    }

    if(this->mSignalInfo->mPermissions != nullptr) {
        try {
            this->mSignalInfo->mPermissions->push_back(permission);
        } catch(const std::bad_alloc& e) {
            return RC_INVALID_VALUE;
        }
    } else {
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode SignalInfoBuilder::addTargetEnabled(const std::string& target) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    if(this->mTargetRefCount == unsupportedResoure) {
        return RC_RESOURCE_NOT_SUPPORTED;
    }

    // first entry
    if(this->mTargetRefCount == 0) {
        this->mTargetRefCount = -1;
    }

    if(target == UrmSettings::targetConfigs.targetName) {
        this->mTargetRefCount = 1;
    }
    return RC_SUCCESS;
}

ErrCode SignalInfoBuilder::addTargetDisabled(const std::string& target) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    if(this->mTargetRefCount == unsupportedResoure) {
        return RC_RESOURCE_NOT_SUPPORTED;
    }

    // first entry
    if(this->mTargetRefCount == 0) {
        this->mTargetRefCount = 1;
    }

    if(target == UrmSettings::targetConfigs.targetName) {
        this->mTargetRefCount = -1;
    }
    return RC_SUCCESS;
}

ErrCode SignalInfoBuilder::addDerivative(const std::string& derivative) {
    if(this->mSignalInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    if(this->mSignalInfo->mDerivatives == nullptr) {
        this->mSignalInfo->mDerivatives = new(std::nothrow) std::vector<std::string>();
    }

    if(this->mSignalInfo->mDerivatives != nullptr) {
        try {
            this->mSignalInfo->mDerivatives->push_back(derivative);
        } catch(const std::bad_alloc& e) {
            return RC_INVALID_VALUE;
        }
    } else {
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode SignalInfoBuilder::addResource(Resource* resource) {
    if(this->mSignalInfo == nullptr || resource == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    if(this->mSignalInfo->mSignalResources == nullptr) {
        this->mSignalInfo->mSignalResources = new(std::nothrow) std::vector<Resource*>();
    }

    if(this->mSignalInfo->mSignalResources != nullptr) {
        try {
            this->mSignalInfo->mSignalResources->push_back(resource);
        } catch(const std::bad_alloc& e) {
            return RC_INVALID_VALUE;
        }
    } else {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    return RC_SUCCESS;
}

SignalInfo* SignalInfoBuilder::build() {
    return this->mSignalInfo;
}

SignalInfoBuilder::~SignalInfoBuilder() {}

ResourceBuilder::ResourceBuilder() {
    this->mResource = new(std::nothrow) Resource;
}

ErrCode ResourceBuilder::setResCode(const std::string& resCodeString) {
    if(this->mResource == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    uint32_t resCode = 0;
    try {
        resCode = (uint32_t)stol(resCodeString, nullptr, 0);

    } catch(const std::exception& e) {
        int8_t found = false;
        resCode = getResCodeFromString(resCodeString.c_str(), &found);
        if(!found) {
            TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
            return RC_INVALID_VALUE;
        }
    }

    this->mResource->setResCode(resCode);
    return RC_SUCCESS;
}

ErrCode ResourceBuilder::setResInfo(const std::string& resInfoString) {
    if(this->mResource == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    int32_t resourceResInfo = 0;
    try {
        resourceResInfo = (int32_t)stoi(resInfoString, nullptr, 0);

    } catch(const std::exception& e) {
        int8_t found = false;
        resourceResInfo = getResCodeFromString(resInfoString.c_str(), &found);
        if(!found) {
            TYPELOGV(SIGNAL_REGISTRY_PARSING_FAILURE, e.what());
            return RC_INVALID_VALUE;
        }
    }

    this->mResource->setResInfo(resourceResInfo);
    return RC_SUCCESS;
}

ErrCode ResourceBuilder::setNumValues(int32_t valuesCount) {
    if(this->mResource == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mResource->setNumValues(valuesCount);
    return RC_SUCCESS;
}

ErrCode ResourceBuilder::addValue(int32_t index, const std::string& valueString) {
    if(this->mResource == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    int32_t value = -1;

    // Check if the value is a placeholder, i.e. the actual value will be populated
    // dynamically at runtime via the "list" argument passed to tuneSignal API.
    if(valueString != "%d") {
        try {
            value = std::stoi(valueString);

        } catch(const std::exception& e) {
            return RC_INVALID_VALUE;
        }
    }

    this->mResource->setValueAt(index, value);
    return RC_SUCCESS;
}

Resource* ResourceBuilder::build() {
    return this->mResource;
}
