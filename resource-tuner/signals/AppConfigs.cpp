// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "AppConfigs.h"

std::shared_ptr<AppConfigs> AppConfigs::appConfigRegistryInstance = nullptr;

void AppConfigs::registerAppConfig(AppConfig* appConfig) {
    if(appConfig == nullptr) return;
    this->mAppConfig[appConfig->mAppName] = appConfig;
}

AppConfig* AppConfigs::getAppConfig(const std::string& name) {
    return this->mAppConfig[name];
}

AppConfigBuilder::AppConfigBuilder() {
    this->mAppConfig = new(std::nothrow) AppConfig();
    this->mAppConfig->isMovementAllowed = true;
}

ErrCode AppConfigBuilder::setAppName(const std::string& name) {
    if(this->mAppConfig == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mAppConfig->mAppName = name;
    return RC_SUCCESS;
}

ErrCode AppConfigBuilder::setMovementType(const std::string& movementAllowed) {
    if(this->mAppConfig == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    if(movementAllowed != "true") {
        this->mAppConfig->isMovementAllowed = false;
    }
    return RC_SUCCESS;
}

ErrCode AppConfigBuilder::setNumThreads(int32_t threadCnt) {
    if(this->mAppConfig == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    try {
        this->mAppConfig->mNumThreads = threadCnt;
        this->mAppConfig->mThreadNameList = new std::string[this->mAppConfig->mNumThreads];
        this->mAppConfig->mCGroupIds = new std::int32_t[this->mAppConfig->mNumThreads];
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_INVALID_VALUE;
}

ErrCode AppConfigBuilder::addThreadMapping(int32_t index,
                                           const std::string& threadName,
                                           const std::string& cGroup) {
    if(this->mAppConfig == nullptr || this->mAppConfig->mThreadNameList == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    int32_t cGroupID = -1;

    try {
        cGroupID = std::stoi(cGroup);

    } catch(const std::exception& e) {
        int8_t found = false;
        cGroupID = getResCodeFromString(cGroup.c_str(), &found);
        if(!found) {
            cGroupID = -1;
        }
    }

    if(cGroupID != -1) {
        this->mAppConfig->mThreadNameList[index] = threadName;
        this->mAppConfig->mCGroupIds[index] = cGroupID;
        return RC_SUCCESS;
    }

    return RC_INVALID_VALUE;
}

ErrCode AppConfigBuilder::setNumSigCodes(int32_t count) {
    if(this->mAppConfig == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    try {
        this->mAppConfig->mNumSignals = count;
        this->mAppConfig->mSignalCodes = new uint32_t[this->mAppConfig->mNumSignals];
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_INVALID_VALUE;
}

ErrCode AppConfigBuilder::addSigCode(int32_t index, const std::string& sigCodeStr) {
    if(this->mAppConfig == nullptr || this->mAppConfig->mSignalCodes == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    try {
        uint32_t sigCode = (uint32_t)stol(sigCodeStr, nullptr, 0);
        this->mAppConfig->mSignalCodes[index] = sigCode;
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_INVALID_VALUE;
}

AppConfig* AppConfigBuilder::build() {
    return this->mAppConfig;
}
