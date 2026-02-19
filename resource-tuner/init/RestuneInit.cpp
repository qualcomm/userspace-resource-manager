// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <dlfcn.h>
#include <cstdint>
#include <string>
#include <thread>
#include <memory>

#include "Config.h"
#include "ErrCodes.h"
#include "Extensions.h"
#include "AuxRoutines.h"
#include "RestuneInternal.h"
#include "SignalInternal.h"
#include "ResourceRegistry.h"
#include "ComponentRegistry.h"
#include "PulseMonitor.h"
#include "RequestReceiver.h"
#include "ClientGarbageCollector.h"
#include "UrmSettings.h"
#include "SignalRegistry.h"
#include "RestuneParser.h"

#define MAX_EXTENSION_LIB_HANDLES 3
static void** extensionLibHandles = nullptr;

// Request Listener and Handler Threads
static std::thread restuneHandlerThread;
static std::thread resourceTunerListener;

static void restoreToSafeState() {
    if(AuxRoutines::fileExists(UrmSettings::mPersistenceFile)) {
        AuxRoutines::writeSysFsDefaults();

        // Delete the Node Persistence File
        AuxRoutines::deleteFile(UrmSettings::mPersistenceFile);
    }
}

// Load the Extensions Plugin lib if it is available
// If the lib is not present, we simply return Success. Since this lib is optional
static ErrCode loadExtensionsLib() {
    if(extensionLibHandles == nullptr) {
        extensionLibHandles = (void**) calloc(MAX_EXTENSION_LIB_HANDLES, sizeof(void*));
        if(extensionLibHandles == nullptr) {
            return RC_MODULE_INIT_FAILURE;
        }
    }

    std::string libDirPath = std::string(LIBDIR_PATH) + "/urm/";
    DIR* dir = opendir(libDirPath.c_str());
    if(dir == nullptr) {
        return RC_SUCCESS; // Return success regardless, since this is an extension.
    }

    uint32_t extLibHandleIndex = 0;
    int32_t libsLoaded = 0;
    struct dirent* entry;
    while(((entry = readdir(dir)) != nullptr) &&
          (extLibHandleIndex < UrmSettings::metaConfigs.mPluginCount)) {
        std::string fileName = std::string(entry->d_name);
        if(fileName == "." || fileName == "..") {
            continue;
        }
        std::string libPath = libDirPath + "/" + fileName;

        // Check if the library file exists
        extensionLibHandles[extLibHandleIndex] = dlopen(libPath.c_str(), RTLD_NOW);
        if(extensionLibHandles[extLibHandleIndex] != nullptr) {
            libsLoaded++;
        } else {
            TYPELOGV(NOTIFY_EXTENSIONS_LOAD_FAILED, libPath.c_str(), dlerror());
        }
        extLibHandleIndex++;
    }
    closedir(dir);

    if(libsLoaded > 0) {
        TYPELOGV(NOTIFY_EXTENSIONS_LIB_LOADED_SUCCESS, libsLoaded);
    }
    return RC_SUCCESS;
}

static void preAllocateMemory() {
    // Preallocate Memory for certain frequently used types.
    uint32_t concurrentRequestsUB = UrmSettings::metaConfigs.mMaxConcurrentRequests;
    uint32_t resourcesPerRequestUB = UrmSettings::metaConfigs.mMaxResourcesPerRequest;

    uint32_t maxBlockCount = concurrentRequestsUB * resourcesPerRequestUB;

    MakeAlloc<Message> (concurrentRequestsUB);
    MakeAlloc<Request> (concurrentRequestsUB);
    MakeAlloc<DLManager> (concurrentRequestsUB);
    MakeAlloc<Timer> (concurrentRequestsUB);
    MakeAlloc<Resource> (maxBlockCount);
    MakeAlloc<ClientInfo> (maxBlockCount);
    MakeAlloc<ClientTidData> (maxBlockCount);
    MakeAlloc<std::unordered_set<int64_t>> (maxBlockCount);
    MakeAlloc<MsgForwardInfo> (maxBlockCount);
    MakeAlloc<ResIterable> (maxBlockCount);
    MakeAlloc<char[REQ_BUFFER_SIZE]> (maxBlockCount);
    MakeAlloc<Signal> (concurrentRequestsUB);
    MakeAlloc<std::vector<Resource*>> (concurrentRequestsUB * resourcesPerRequestUB);
    MakeAlloc<std::vector<uint32_t>> (concurrentRequestsUB * resourcesPerRequestUB);
}

static void initLogger() {
    std::string resultBuffer;

    int32_t logLevel = LOG_DEBUG;
    submitPropGetRequest(LOGGER_LOGGING_LEVEL, resultBuffer, "DEBUG");
    std::string level = std::string(resultBuffer);

    if(level == "DEBUG") logLevel = LOG_DEBUG;
    if(level == "INFO") logLevel = LOG_INFO;
    if(level == "ERROR") logLevel = LOG_ERR;
    if(level == "WARN") logLevel = LOG_WARNING;

    int8_t levelSpecificLogging = false;
    submitPropGetRequest(LOGGER_LOGGING_LEVEL_TYPE, resultBuffer, "false");
    if(resultBuffer == "true") {
        levelSpecificLogging = true;
    }

    RedirectOptions redirectOutputTo = RedirectOptions::LOG_TOSYSLOG;
    submitPropGetRequest(LOGGER_LOGGING_OUTPUT_REDIRECT, resultBuffer, "SYSLOG");
    std::string target = std::string(resultBuffer);

    if(target == "FILE") redirectOutputTo = RedirectOptions::LOG_TOFILE;
    if(target == "SYSLOG") redirectOutputTo = RedirectOptions::LOG_TOSYSLOG;
    if(target == "FTRACE") redirectOutputTo = RedirectOptions::LOG_TOFTRACE;
    if(target == "LOGCAT") redirectOutputTo = RedirectOptions::LOG_TOLOGCAT;

    // Configure
    Logger::configure(logLevel, levelSpecificLogging, redirectOutputTo);
}

static ErrCode fetchMetaConfigs() {
    std::string resultBuffer;

    try {
        // Fetch target Name
        UrmSettings::targetConfigs.targetName = AuxRoutines::getMachineName();
        TYPELOGV(NOTIFY_CURRENT_TARGET_NAME, UrmSettings::targetConfigs.targetName.c_str());

        submitPropGetRequest(MAX_CONCURRENT_REQUESTS, resultBuffer, "50");
        UrmSettings::metaConfigs.mMaxConcurrentRequests = (uint32_t)std::stol(resultBuffer);

        submitPropGetRequest(MAX_RESOURCES_PER_REQUEST, resultBuffer, "5");
        UrmSettings::metaConfigs.mMaxResourcesPerRequest = (uint32_t)std::stol(resultBuffer);

        submitPropGetRequest(PULSE_MONITOR_DURATION, resultBuffer, "60000");
        UrmSettings::metaConfigs.mPulseDuration = (uint32_t)std::stol(resultBuffer);

        submitPropGetRequest(GARBAGE_COLLECTOR_DURATION, resultBuffer, "83000");
        UrmSettings::metaConfigs.mClientGarbageCollectorDuration = (uint32_t)std::stol(resultBuffer);

        submitPropGetRequest(GARBAGE_COLLECTOR_BATCH_SIZE, resultBuffer, "20");
        UrmSettings::metaConfigs.mCleanupBatchSize = (uint32_t)std::stol(resultBuffer);

        submitPropGetRequest(RATE_LIMITER_DELTA, resultBuffer, "5");
        UrmSettings::metaConfigs.mDelta = (uint32_t)std::stol(resultBuffer);

        submitPropGetRequest(RATE_LIMITER_PENALTY_FACTOR, resultBuffer, "2.0");
        UrmSettings::metaConfigs.mPenaltyFactor = std::stod(resultBuffer);

        submitPropGetRequest(RATE_LIMITER_REWARD_FACTOR, resultBuffer, "0.4");
        UrmSettings::metaConfigs.mRewardFactor = std::stod(resultBuffer);

        submitPropGetRequest(URM_MAX_PLUGIN_COUNT, resultBuffer, "3");
        UrmSettings::metaConfigs.mPluginCount = (uint32_t)std::stol(resultBuffer);

    } catch(const std::invalid_argument& e) {
        TYPELOGV(META_CONFIG_PARSE_FAILURE, e.what());
        return RC_PROP_PARSING_ERROR;

    } catch(const std::out_of_range& e) {
        TYPELOGV(META_CONFIG_PARSE_FAILURE, e.what());
        return RC_PROP_PARSING_ERROR;
    }

    return RC_SUCCESS;
}

static ErrCode parseUtil(const std::string& filePath,
                         const std::string& desc,
                         ConfigType configType) {

    if(filePath.length() == 0) return RC_FILE_NOT_FOUND;
    ErrCode opStatus = RC_SUCCESS;
    RestuneParser configProcessor;

    TYPELOGV(NOTIFY_PARSING_START, desc.c_str(), filePath.c_str());
    opStatus = configProcessor.parse(configType, filePath);

    if(RC_IS_NOTOK(opStatus)) {
        TYPELOGV(NOTIFY_PARSING_FAILURE, desc.c_str());
        return opStatus;
    }

    TYPELOGV(NOTIFY_PARSING_SUCCESS, desc.c_str());
    return opStatus;
}

static std::string getFullTargetBasedConfPath(const std::string& configFileName) {
    if(UrmSettings::targetConfigs.targetName.length() == 0) {
        return "";
    }

    std::string filePath = "";
    filePath.append(UrmSettings::mTargetConfDir);
    filePath.append(UrmSettings::targetConfigs.targetName);
    filePath.append("/");
    filePath.append(configFileName);

    return filePath;
}

static ErrCode fetchCommonProperties() {
    std::string filePath = UrmSettings::mCommonPropertiesPath;
    return parseUtil(filePath, "prop-common", ConfigType::PROPERTIES_CONFIG);
}

static ErrCode fetchCustomProperties() {
    ErrCode opStatus = RC_SUCCESS;

    std::string customConfPaths[4] = {
        UrmSettings::mDevIndexedPropertiesPath,
        getFullTargetBasedConfPath("PropertiesConfig.yaml"),
        UrmSettings::mCustomPropertiesPath,
        Extensions::getPropertiesConfigFilePath(),
    };

    for(int32_t i = 0; i < 4; i++) {
        std::string filePath = customConfPaths[i];
        if(AuxRoutines::fileExists(filePath)) {
            opStatus = parseUtil(filePath, "prop-custom", ConfigType::PROPERTIES_CONFIG);
            if(RC_IS_NOTOK(opStatus)) {
                return opStatus;
            }
        }
    }

    return opStatus;
}

static ErrCode fetchResources() {
    ErrCode opStatus = RC_SUCCESS;

    // Parse Common Resource Configs
    std::string filePath = UrmSettings::mCommonResourcesPath;
    opStatus = parseUtil(filePath, "resource-common", ConfigType::RESOURCE_CONFIG);
    if(RC_IS_NOTOK(opStatus)) {
        return opStatus;
    }

    std::string customConfPaths[4] = {
        UrmSettings::mDevIndexedResourcesPath,
        getFullTargetBasedConfPath("ResourcesConfig.yaml"),
        UrmSettings::mCustomResourcesPath,
        Extensions::getResourceConfigFilePath(),
    };

    for(int32_t i = 0; i < 4; i++) {
        filePath = customConfPaths[i];
        if(AuxRoutines::fileExists(filePath)) {
            opStatus = parseUtil(filePath, "resource-custom", ConfigType::RESOURCE_CONFIG);
            if(RC_IS_NOTOK(opStatus)) {
                return opStatus;
            }
        }
    }

    return opStatus;
}

static ErrCode fetchTargetInfo() {
    ErrCode opStatus = RC_SUCCESS;
    // Perform Logical To Physical (Core / Cluster) Mapping
    // Note we don't perform error-checking here since the behaviour of this
    // routine is target / architecture specific, and the initialization flow
    // needs to be generic enough to accomodate them.
    TargetRegistry::getInstance()->readTargetInfo();

    std::string customConfPaths[4] = {
        UrmSettings::mDevIndexedTargetPath,
        getFullTargetBasedConfPath("TargetConfig.yaml"),
        UrmSettings::mCustomTargetPath,
        Extensions::getTargetConfigFilePath(),
    };

    for(int32_t i = 0; i < 4; i++) {
        std::string filePath = customConfPaths[i];
        if(AuxRoutines::fileExists(filePath)) {
            opStatus = parseUtil(filePath, "target-custom", ConfigType::RESOURCE_CONFIG);
            if(RC_IS_NOTOK(opStatus)) {
                return opStatus;
            }
        }
    }

    return opStatus;
}

static ErrCode fetchInitInfo() {
    ErrCode opStatus = RC_SUCCESS;
    std::string filePath = UrmSettings::mCommonInitPath;

    opStatus = parseUtil(filePath, "init-common", ConfigType::INIT_CONFIG);
    if(RC_IS_NOTOK(opStatus)) {
        return opStatus;
    }

    std::string customConfPaths[4] = {
        UrmSettings::mDevIndexedInitPath,
        getFullTargetBasedConfPath("InitConfig.yaml"),
        UrmSettings::mCustomInitPath,
        Extensions::getInitConfigFilePath(),
    };

    for(int32_t i = 0; i < 4; i++) {
        filePath = customConfPaths[i];
        if(AuxRoutines::fileExists(filePath)) {
            opStatus = parseUtil(filePath, "init-custom", ConfigType::INIT_CONFIG);
            if(RC_IS_NOTOK(opStatus)) {
                return opStatus;
            }
        }
    }

    return opStatus;
}

static ErrCode fetchSignals() {
    ErrCode opStatus = RC_SUCCESS;

    // Parse Common Signal Configs
    std::string filePath = UrmSettings::mCommonSignalsPath;
    opStatus = parseUtil(filePath, "signal-common", ConfigType::SIGNALS_CONFIG);
    if(RC_IS_NOTOK(opStatus)) {
        return opStatus;
    }

    std::string customConfPaths[4] = {
        UrmSettings::mDevIndexedSignalsPath,
        getFullTargetBasedConfPath("SignalsConfig.yaml"),
        UrmSettings::mCustomSignalsPath,
        Extensions::getSignalsConfigFilePath(),
    };

    for(int32_t i = 0; i < 4; i++) {
        filePath = customConfPaths[i];
        if(AuxRoutines::fileExists(filePath)) {
            opStatus = parseUtil(filePath, "signal-custom", ConfigType::SIGNALS_CONFIG);
            if(RC_IS_NOTOK(opStatus)) {
                return opStatus;
            }
        }
    }

    return opStatus;
}

// Since this is a Custom (Optional) Config, hence if the expected Config file is
// not found, we simply return Success.
static ErrCode fetchExtFeatureConfigs() {
    ErrCode opStatus = RC_SUCCESS;

    std::string customConfPaths[4] = {
        UrmSettings::mDevIndexedExtFeatPath,
        getFullTargetBasedConfPath("ExtFeaturesConfig.yaml"),
        UrmSettings::mCustomExtFeaturesPath,
        Extensions::getExtFeaturesConfigFilePath(),
    };

    for(int32_t i = 0; i < 4; i++) {
        std::string filePath = customConfPaths[i];
        if(AuxRoutines::fileExists(filePath)) {
            opStatus = parseUtil(filePath, "ext-features-custom", ConfigType::EXT_FEATURES_CONFIG);
            if(RC_IS_NOTOK(opStatus)) {
                return opStatus;
            }
        }
    }

    return opStatus;
}

static ErrCode fetchPerAppConfigs() {
    ErrCode opStatus = RC_SUCCESS;

    std::string customConfPaths[4] = {
        UrmSettings::mDevIndexedAppPath,
        getFullTargetBasedConfPath("PerApp.yaml"),
        UrmSettings::mCustomAppConfigPath,
        Extensions::getAppConfigFilePath(),
    };

    for(int32_t i = 0; i < 4; i++) {
        std::string filePath = customConfPaths[i];
        if(filePath.length() > 0 && AuxRoutines::fileExists(filePath)) {
            opStatus = parseUtil(filePath, "app-config-custom", ConfigType::APP_CONFIG);
            if(RC_IS_NOTOK(opStatus)) {
                return opStatus;
            }
        }
    }

    return opStatus;
}

// Initialize Request and Timer ThreadPools
static ErrCode preAllocateWorkers() {
    int32_t desiredThreadCapacity = UrmSettings::desiredThreadCount;
    int32_t maxScalingCapacity = UrmSettings::maxScalingCapacity;

    try {
        RequestReceiver::mRequestsThreadPool = new ThreadPool(desiredThreadCapacity,
                                                              maxScalingCapacity);

        // Allocate 2 extra threads for Pulse Monitor and Garbage Collector
        Timer::mTimerThreadPool = new ThreadPool(desiredThreadCapacity + 2,
                                                 maxScalingCapacity);

    } catch(const std::bad_alloc& e) {
        TYPELOGV(THREAD_POOL_CREATION_FAILURE, e.what());
        return RC_MODULE_INIT_FAILURE;
    }

    return RC_SUCCESS;
}

static void* restuneThreadStart() {
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();

    // Initialize CocoTable
    CocoTable::getInstance();
    while(UrmSettings::isServerOnline()) {
        requestQueue->wait();
    }

    return nullptr;
}

static ErrCode setCgroupParam(const std::string& slice,
                              const std::string& name,
                              const std::string& value) {
    // 1) Build the absolute cGroupPath: /sys/fs/cgroup/<slice>/<name>
    std::string cGroupPath = UrmSettings::mBaseCGroupPath;
    cGroupPath += slice;
    cGroupPath += "/";
    cGroupPath += name;

    // 2) existence/permission check
    if (!AuxRoutines::fileExists(cGroupPath)) {
        // Compose a detailed log line with cGroupPath + strerror
        std::string detail = "cGroupPath=" + cGroupPath + ", err=" + std::string(strerror(errno));
        TYPELOGV(ERRNO_LOG, "access", detail.c_str());
        return RC_SOCKET_FD_READ_FAILURE;
    }

    // 3) Write the value with newline
    AuxRoutines::writeToFile(cGroupPath, value);
    return RC_SUCCESS;
}

static void configureFocusedSlice() {
    // 2D array of const char* pairs: {key, value}
    const char* cgroupParam[][2] = {
        {"cgroup.max.depth", "3"},
        {"cgroup.max.descendants", "10"},
    };

    for(size_t i = 0; i < sizeof(cgroupParam) / sizeof(cgroupParam[0]); i++) {
        setCgroupParam(UrmSettings::focusedCgroup.c_str(), cgroupParam[i][0], cgroupParam[i][1]);
    }
}

static ErrCode init(void* arg) {
    (void)arg;
    // Server might have been restarted by systemd
    // Ensure that Resource Nodes are reset to sane state
    restoreToSafeState();

    // Start Resource Tuner Server Initialization
    // As part of Server Initialization the Configs (Resource / Signals etc.) will be parsed
    // If any of mandatory Configs cannot be parsed then initialization will fail.
    // Mandatory Configs include: Properties Configs, Resource Configs and Signal Configs (if Signal
    // module is plugged in)

    // Fetch common properties
    if(RC_IS_NOTOK(fetchCommonProperties())) {
        TYPELOGD(PROPERTY_RETRIEVAL_FAILED);
        return RC_MODULE_INIT_FAILURE;
    }

    if(RC_IS_NOTOK(fetchMetaConfigs())) {
        TYPELOGD(META_CONF_FETCH_FAILED);
        return RC_MODULE_INIT_FAILURE;
    }

    uint32_t pluginCount = UrmSettings::metaConfigs.mPluginCount;
    if(pluginCount > MAX_EXTENSION_LIB_HANDLES) {
        extensionLibHandles = (void**) realloc(extensionLibHandles, pluginCount * sizeof(void*));
        if(extensionLibHandles == nullptr) {
            return RC_MODULE_INIT_FAILURE;
        }
    }

    // Check if Extensions Plugin lib is available
    if(RC_IS_NOTOK(loadExtensionsLib())) {
        return RC_MODULE_INIT_FAILURE;
    }

    // Fetch custom Properties
    if(RC_IS_NOTOK(fetchCustomProperties())) {
        TYPELOGD(PROPERTY_RETRIEVAL_FAILED);
        return RC_MODULE_INIT_FAILURE;
    }

    if(RC_IS_NOTOK(fetchMetaConfigs())) {
        TYPELOGD(META_CONF_FETCH_FAILED);
        return RC_MODULE_INIT_FAILURE;
    }

    // Pre-Allocate Memory for Commonly used Types via Memory Pool
    preAllocateMemory();

    // Setup the logger, according to configuration urm can log to either:
    // syslog, ftrace or regular text file.
    initLogger();

    // Pre Allocate some Worker Threads in the Thread Pool for handling requests
    if(RC_IS_NOTOK(preAllocateWorkers())) {
        return RC_MODULE_INIT_FAILURE;
    }

    // Fetch and Parse: Custom Target Configs
    if(RC_IS_NOTOK(fetchTargetInfo())) {
        return RC_MODULE_INIT_FAILURE;
    }

    // Fetch and Parse: Init Configs
    // Init Configs which will be considered:
    // - Common Init Configs
    // - Custom Init Configs (if present)
    if(RC_IS_NOTOK(fetchInitInfo())) {
        return RC_MODULE_INIT_FAILURE;
    }

    // Fetch and Parse Resource Configs
    // Resource Configs which will be considered:
    // - Common Resource Configs
    // - Target Specific Resource Configs (if present)
    // - Custom Resource Configs (if present)
    // Note by this point, we will know the Target Info, i.e. number of Core, Clusters etc.
    if(RC_IS_NOTOK(fetchResources())) {
        return RC_MODULE_INIT_FAILURE;
    }

    // Fetch and Parse Signal Configs
    // Signal Configs which will be considered:
    // - Common Signal Configs
    // - Target Specific Signal Configs (if present)
    // - Custom Signal Configs (if present)
    if(RC_IS_NOTOK(fetchSignals())) {
        return RC_MODULE_INIT_FAILURE;
    }

    // Fetch and Parse: Custom Per-App Configs
    if(RC_IS_NOTOK(fetchPerAppConfigs())) {
        return RC_MODULE_INIT_FAILURE;
    }

    // Fetch and Parse: Custom ExtFeature Configs
    if(RC_IS_NOTOK(fetchExtFeatureConfigs())) {
        return RC_MODULE_INIT_FAILURE;
    }

    // By this point, all the Extension Appliers / Resources would have been registered.
    ResourceRegistry::getInstance()->pluginModifications();

    // Initialize external features
    ExtFeaturesRegistry::getInstance()->initializeFeatures();

    // Configure focused.slice parameters
    configureFocusedSlice();

    // Create the Processor thread:
    try {
        restuneHandlerThread = std::thread(restuneThreadStart);
    } catch(const std::system_error& e) {
        TYPELOGV(SYSTEM_THREAD_CREATION_FAILURE, "resource-tuner", e.what());
        return RC_MODULE_INIT_FAILURE;
    }

    // Wait for the thread to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

     // Start the Pulse Monitor and Garbage Collector Daemon Threads
    if(RC_IS_NOTOK(startPulseMonitorDaemon())) {
        TYPELOGD(PULSE_MONITOR_INIT_FAILED);
        return RC_MODULE_INIT_FAILURE;
    }

    if(RC_IS_NOTOK(startClientGarbageCollectorDaemon())) {
        TYPELOGD(GARBAGE_COLLECTOR_INIT_FAILED);
        return RC_MODULE_INIT_FAILURE;
    }

    // Create the listener thread
    try {
        resourceTunerListener = std::thread(listenerThreadStartRoutine);
        TYPELOGD(LISTENER_THREAD_CREATION_SUCCESS);

    } catch(const std::system_error& e) {
        TYPELOGV(SYSTEM_THREAD_CREATION_FAILURE, "resource-tuner-listener", e.what());
        return RC_MODULE_INIT_FAILURE;
    }

    // Wait for the thread to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    return RC_SUCCESS;
}

static ErrCode tear(void* arg) {
    (void)arg;
    // Check if the thread is joinable, to prevent undefined behaviour
    if(resourceTunerListener.joinable()) {
        resourceTunerListener.join();
    } else {
        TYPELOGV(SYSTEM_THREAD_NOT_JOINABLE, "resource-tuner-listener");
    }

    // Check if the thread is joinable, to prevent undefined behaviour
    if(restuneHandlerThread.joinable()) {
        RequestQueue::getInstance()->forcefulAwake();
        restuneHandlerThread.join();
    } else {
        TYPELOGV(SYSTEM_THREAD_NOT_JOINABLE, "resource-tuner");
    }

    // Restore all the Resources to Original Values
    ResourceRegistry::getInstance()->restoreResourcesToDefaultValues();

    stopPulseMonitorDaemon();
    stopClientGarbageCollectorDaemon();

    if(RequestReceiver::mRequestsThreadPool != nullptr) {
        delete RequestReceiver::mRequestsThreadPool;
    }

    if(Timer::mTimerThreadPool != nullptr) {
        delete Timer::mTimerThreadPool;
    }

    // Delete the Sysfs Persistent File
    AuxRoutines::deleteFile(UrmSettings::mPersistenceFile);

    if(extensionLibHandles != nullptr) {
        for(uint32_t i = 0; i < UrmSettings::metaConfigs.mPluginCount; i++) {
            if(extensionLibHandles[i] != nullptr) {
                dlclose(extensionLibHandles[i]);
                extensionLibHandles[i] = nullptr;
            }
        }

        free(extensionLibHandles);
    }

    return RC_SUCCESS;
}

URM_REGISTER_MODULE(MOD_RESTUNE, init, tear);
