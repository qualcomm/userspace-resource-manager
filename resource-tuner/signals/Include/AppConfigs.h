// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef APP_CONFIG_REGISTRY_H
#define APP_CONFIG_REGISTRY_H

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>

#include "Logger.h"
#include "UrmPlatformAL.h"
#include "ErrCodes.h"

#include "RestuneVisibility.h"

typedef struct {
    std::string mAppName;
    int32_t mNumThreads;
    std::string* mThreadNameList;
    int32_t* mCGroupIds;
    int32_t mNumSignals;
    uint32_t* mSignalCodes;
} AppConfig;

class RESTUNE_INTERNAL_EXPORT AppConfigs {
private:
    static std::shared_ptr<AppConfigs> appConfigRegistryInstance;
    std::unordered_map<std::string, AppConfig*> mAppConfig;

public:
    void registerAppConfig(AppConfig* appConfig);
    AppConfig* getAppConfig(const std::string& appName);

    static std::shared_ptr<AppConfigs> getInstance() {
        if(appConfigRegistryInstance == nullptr) {
            try {
                appConfigRegistryInstance = std::shared_ptr<AppConfigs>(new AppConfigs());
            } catch(const std::bad_alloc& e) {
                LOGE("RESTUNE_SIGNAL_REGISTRY",
                     "Failed to allocate memory for AppConfigs instance: " + std::string(e.what()));
                return nullptr;
            }
        }
        return appConfigRegistryInstance;
    }
};

class AppConfigBuilder {
private:
    AppConfig* mAppConfig;

public:
    AppConfigBuilder();

    ErrCode setAppName(const std::string& name);
    ErrCode setNumThreads(int32_t count);
    ErrCode addThreadMapping(int32_t index, const std::string& threadName, const std::string& cGroup);
    ErrCode setNumSigCodes(int32_t sigCount);
    ErrCode addSigCode(int32_t index, const std::string& sigCodeStr);

    AppConfig* build();
};

#endif
