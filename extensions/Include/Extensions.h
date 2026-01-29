// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

/*!
 * \file  Extensions.h
 */

#ifndef URM_EXTENSIONS_H
#define URM_EXTENSIONS_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

typedef void (*ResourceLifecycleCallback)(void*);
typedef void (*PostProcessingCallback)(void*);

typedef struct {
    pid_t mPid;
    uint32_t mSigId;
    uint32_t mSigType;
} PostProcessCBData;

/**
 * @enum ConfigType
 * @brief Different Config (via YAML) Types supported.
 * @details Note, the Config File corresponding to each config type
 *          can be altered via the Extensions interface.
 */
enum ConfigType {
    RESOURCE_CONFIG,
    PROPERTIES_CONFIG,
    SIGNALS_CONFIG,
    EXT_FEATURES_CONFIG,
    TARGET_CONFIG,
    INIT_CONFIG,
    APP_CONFIG,
    TOTAL_CONFIGS_COUNT,
};

/**
 * @brief Extensions
 * @details Provides an Interface for Customizing Resource Tuner Behaviour. Through the Extension Interface,
 *          Custom Resource Callbacks / Appliers as well as Custom Config Files (for example: Resource
 *          Configs or Signal Configs) can be specified.
 */
class Extensions {
private:
    static std::vector<std::string> mModifiedConfigFiles;
    static std::unordered_map<uint32_t, ResourceLifecycleCallback> mResourceApplierCallbacks;
    static std::unordered_map<uint32_t, ResourceLifecycleCallback> mResourceTearCallbacks;
    static std::unordered_map<std::string, PostProcessingCallback> mPostProcessCallbacks;

public:
    Extensions(uint32_t resCode, int8_t callbackType, ResourceLifecycleCallback callback);
    Extensions(ConfigType configType, std::string yamlFile);
    Extensions(const std::string& identifier, PostProcessingCallback callback);

    static std::vector<std::pair<uint32_t, ResourceLifecycleCallback>> getResourceApplierCallbacks();
    static std::vector<std::pair<uint32_t, ResourceLifecycleCallback>> getResourceTearCallbacks();

    static std::string getResourceConfigFilePath();
    static std::string getPropertiesConfigFilePath();
    static std::string getSignalsConfigFilePath();
    static std::string getExtFeaturesConfigFilePath();
    static std::string getTargetConfigFilePath();
    static std::string getInitConfigFilePath();
    static std::string getAppConfigFilePath();

    static PostProcessingCallback getPostProcessingCallback(const std::string& identifier);
};

#define CONCAT(a, b) a ## b

/**
 * \def URM_REGISTER_RES_APPLIER_CB(resCode, resourceApplierCallback)
 * \brief Register a Customer Resource Applier Callback for a particular ResCode
 * \param resCode An unsigned 32-bit integer representing the Resource ResCode.
 * \param resourceApplierCallback A function Pointer to the Custom Applier Callback.
 *
 * \note This macro must be used in the Global Scope.
 */
#define URM_REGISTER_RES_APPLIER_CB(resCode, resourceApplierCallback) \
        static Extensions CONCAT(_resourceApplier, resCode)(resCode, 0, resourceApplierCallback);

/**
 * \def URM_REGISTER_RES_TEAR_CB(resCode, resourceTearCallback)
 * \brief Register a Customer Resource Teardown Callback for a particular ResCode
 * \param resCode An unsigned 32-bit integer representing the Resource ResCode.
 * \param resourceTearCallback A function Pointer to the Custom Teardown Callback.
 *
 * \note This macro must be used in the Global Scope.
 */
#define URM_REGISTER_RES_TEAR_CB(resCode, resourceTearCallback) \
        static Extensions CONCAT(_resourceTear, resCode)(resCode, 1, resourceTearCallback);

/**
 * \def URM_REGISTER_CONFIG(configType, yamlFile)
 * \brief Register custom Config (YAML) file. This Macro can be used to register
 *        Resource Configs File, Signal Configs file and others with Resource Tuner.
 * \param configType The type of Config for which the Custom YAML file has to be specified.
 * \param yamlFile File Path of this Config YAML file.
 *
 * \note This macro must be used in the Global Scope.
 */
#define URM_REGISTER_CONFIG(configType, yamlFile) \
        static Extensions CONCAT(_regConfig, configType)(configType, yamlFile);

/**
 * \def URM_REGISTER_POST_PROCESS_CB(identifier, callback)
 * \brief Register post processing callbacks for different workloads and per-process.
 * \param identifier The key (string) identifier for the process or workload
 * \param callback The post processing Callback to be registered
 *
 * \note This macro must be used in the Global Scope.
 */
#define URM_REGISTER_POST_PROCESS_CB(identifier, callback) \
        static Extensions CONCAT(callback, __LINE__)(identifier, callback);

#endif
