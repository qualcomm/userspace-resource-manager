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
#define REQ_BUFFER_SIZE 1024

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
    static const int32_t desiredThreadCount = 15;
    static const int32_t maxScalingCapacity = 30;

    // Support both versions: Common and Custom
    static const std::string mCommonResourceFilePath;
    static const std::string mCustomResourceFilePath;
    static const std::string mCommonSignalFilePath;
    static const std::string mCustomSignalFilePath;
    static const std::string mCommonPropertiesFilePath;
    static const std::string mCustomPropertiesFilePath;

    // Only Custom Config is supported for Target and Ext Features Config
    static const std::string mCustomTargetFilePath;
    static const std::string mCustomExtFeaturesFilePath;

    static const std::string mCustomAppConfigFilePath;

    // Support both versions: Common and Custom
    static const std::string mCommonInitConfigFilePath;
    static const std::string mCustomInitConfigFilePath;

    static const std::string mExtensionsPluginLibPath;

    static const std::string mDeviceNamePath;
    static const std::string mBaseCGroupPath;

    static const std::string mPersistenceFile;

    static MetaConfigs metaConfigs;
    static TargetConfigs targetConfigs;

    static int32_t isServerOnline();
    static void setServerOnlineStatus(int32_t isOnline);
};

#endif
