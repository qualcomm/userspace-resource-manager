// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef URM_SETTINGS_H
#define URM_SETTINGS_H

#include <unordered_map>

#include "ErrCodes.h"
#include "MemoryPool.h"
#include "SafeOps.h"
#include "Utils.h"

#define URM_IDENTIFIER "urm"
#define REQ_BUFFER_SIZE 580

// Operational Tunable Parameters for Resource Tuner
typedef struct {
    uint32_t mMaxConcurrentRequests;
    uint32_t mMaxResourcesPerRequest;
    uint32_t mListeningPort;
    uint32_t mPulseDuration;
    uint32_t mClientGarbageCollectorDuration;
    uint32_t mDelta;
    uint32_t mCleanupBatchSize;
    double mPenaltyFactor;
    double mRewardFactor;
    uint32_t mPluginCount;
} MetaConfigs;

typedef struct {
    std::string targetName;
    int32_t mTotalCoreCount;
    int32_t mTotalClusterCount;
    // Determine whether the system is in Display On or Off / Doze Mode
    // This needs to be tracked, so that only those Requests for which background Processing
    // is Enabled can be processed during Display Off / Doze.
    int8_t currMode;
} TargetConfigs;

class UrmSettings {
private:
    static int32_t serverOnlineStatus;

public:
    static const int32_t desiredThreadCount = 5;
    static const int32_t maxScalingCapacity = 10;

    static const std::string mTargetConfDir;

    // Support both versions: Common and Custom
    static const std::string mCommonResourcesPath;
    static const std::string mCustomResourcesPath;
    static const std::string mDevIndexedResourcesPath;

    static const std::string mCommonSignalsPath;
    static const std::string mCustomSignalsPath;
    static const std::string mDevIndexedSignalsPath;

    static const std::string mCommonPropertiesPath;
    static const std::string mCustomPropertiesPath;
    static const std::string mDevIndexedPropertiesPath;

    static const std::string mCommonInitPath;
    static const std::string mCustomInitPath;
    static const std::string mDevIndexedInitPath;

    // Only Custom Config is supported for Target, Ext Features and App Configs
    static const std::string mCustomTargetPath;
    static const std::string mDevIndexedTargetPath;

    static const std::string mCustomExtFeaturesPath;
    static const std::string mDevIndexedExtFeatPath;

    static const std::string mCustomAppConfigPath;
    static const std::string mDevIndexedAppPath;

    static const std::string focusedCgroup;
    static const std::string mDeviceNamePath;
    static const std::string mBaseCGroupPath;
    static const std::string mPersistenceFile;

    // Target Information Stores
    static MetaConfigs metaConfigs;
    static TargetConfigs targetConfigs;

    static int32_t isServerOnline();
    static void setServerOnlineStatus(int32_t isOnline);
};

#endif
