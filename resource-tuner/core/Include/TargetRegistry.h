// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef TARGET_REGISTRY_H
#define TARGET_REGISTRY_H

/*!
 * \file  TargetRegistry.h
 */

#include <memory>
#include <unordered_map>
#include <vector>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <regex>
#include <memory>

#include "Logger.h"
#include "ErrCodes.h"
#include "UrmSettings.h"
#include "AuxRoutines.h"
#include "RestuneDBus.h"

#define POLICY_DIR_PATH "/sys/devices/system/cpu/cpufreq/"
#define ONLINE_CPU_FILE_PATH "/sys/devices/system/cpu/online"
#define CPU_CAPACITY_FILE_PATH "/sys/devices/system/cpu/cpu%d/cpu_capacity"

/**
 * @struct CGroupConfigInfo
 * @brief Representation of a single CGroup Configuration Info
 */
typedef struct {
    std::string mCgroupName; //!< Cgroup Name
    int32_t mCgroupID;  //!< 32-bit identifier for the Cgroup (to be used as part of tuneResources API)
    int8_t mCreationNeeded; //!< Flag indicating if Cgroup needs to be created by Resource Tuner, or if it already exists
    int8_t mIsThreaded; //!< Flag indicating if the Cgroup is threaded.
} CGroupConfigInfo;

/**
 * @struct ClusterInfo
 * @brief Representation for various Clusters detected on the device.
 */
typedef struct {
    int32_t mPhysicalID; //!< Physical Cluster ID corresponding to the logical ID.
    int32_t mCapacity; //!< Cluster Capacity
    int32_t mStartCpu; //!< Starting CPU index in this cluster
    int32_t mNumCpus; //!< Number of CPUs part of the Cluster
} ClusterInfo;

/**
 * @struct MpamGroupConfigInfo
 * @brief Representation of a single Mpam Group Configuration Info
 */
typedef struct {
    int32_t mMpamGroupInfoID; //!< 32-bit identifier for the Mpam Group (to be used as part of tuneResources API)
    std::string mMpamGroupName; //!< Mpam group Name
    int32_t mPriority; //!< Mpam group Priority
} MpamGroupConfigInfo;

/**
 * @struct CacheInfo
 * @brief Representation for a single Cluster Type Info.
 */
typedef struct {
    std::string mCacheType; //!< Cache Type, for example: L2 or L3
    int32_t mNumCacheBlocks; //!< Number of cache blocks for this type
    int8_t mPriorityAware; //!< Flag indicating if the Cache type is priority aware.
} CacheInfo;

/**
 * @brief TargetRegistry
 * @details Stores all the target related info, fetched dynamically or provided
 *          statically via Target and Init Config files.
 */
class __attribute__((visibility("default"))) TargetRegistry {
private:
    static std::shared_ptr<TargetRegistry> targetRegistryInstance;

    std::unordered_map<int32_t, int32_t> mLogicalToPhysicalClusterMapping;
    std::unordered_map<int32_t, ClusterInfo*> mPhysicalClusters;
    std::unordered_map<int32_t, CGroupConfigInfo*> mCGroupMapping;
    std::unordered_map<int32_t, MpamGroupConfigInfo*> mMpamGroupMapping;
    std::unordered_map<std::string, CacheInfo*> mCacheInfoMapping;

    TargetRegistry();

    void generatePolicyBasedMapping(std::vector<std::string>& policyDirs);
    void getClusterIdBasedMapping();

public:
    ~TargetRegistry();

    // Methods for adding Target Info via TargetConfig.yaml
    void addClusterSpreadInfo(const std::string& physicalID, const std::string& coreCount);
    void addClusterMapping(const std::string& logicalID, const std::string& physicalID);

    // Method for adding CGroup configs from InitConfig.yaml
    void addCGroupMapping(CGroupConfigInfo* cGroupConfigInfo);

    // Method for adding Mpam Group configs from InitConfig.yaml
    void addMpamGroupMapping(MpamGroupConfigInfo* mpamGroupConfigInfo);

    void addCacheInfoMapping(CacheInfo* cacheInfo);

    void getClusterIDs(std::vector<int32_t>& clusterIDs);

    /**
     * @brief Called by the Verifier to get the physical core ID corresponding to the Logical Core ID value.
     * @details This routine performs Logical to Physical Core Translation.
     * @param logicalClusterId The Logical Cluster ID, passed via the mResInfo field (part of the Resource struct)
     *                         when issuing a tuneResources API call.
     * @param logicalCoreId The Logical Core ID, passed via the mResInfo field (part of the Resource struct)
     *                      when issuing a tuneResources API call.
     * @return int32_t:\n
     *            - A Non-Negative Integer, representing the corresponding physical Core ID.
     *            - -1: otherwise
     */
    int32_t getPhysicalCoreId(int32_t logicalClusterId, int32_t logicalCoreId);

    /**
     * @brief Called by the Verifier to get the physical Cluster ID corresponding to the Logical Cluster ID value.
     * @details This routine performs Logical to Physical Cluster Translation.
     * @param logicalClusterId The Logical Cluster ID, passed via the mResInfo field (part of the Resource struct)
     *                         when issuing a tuneResources API call.
     * @return int32_t:\n
     *            - A Non-Negative Integer, representing the corresponding physical Cluster ID.
     *            - -1: otherwise
     */
    int32_t getPhysicalClusterId(int32_t logicalClusterId);

    ClusterInfo* getClusterInfo(int32_t physicalClusterID);

    /**
     * @brief Called during Server Init, to read and Parse the Logical To Physical Core / Cluster Mappings.
     * @details This routine will extract the physical Core IDs and the list of CPU cores part of each Physical Cluster
     *          This data will be used to perform Logical to Physical Translation for each incoming tuneResources Request
     *          later on, if it contains any Resource which has ApplyType set to Core.
     *
     *          Note: This function tries to use different strategies to get the Core / Cluster Enumeration and Mapping data:
     *          - If the BU has provided a mapping file, it will be used. This mapping file should contain data specifying
     *            the total Number of Cores / Clusters, mapping of Cluster Type to Physical IDs and Number of Cores
     *            in each Cluster.
     *          - If no such file is provided, we try to generate the mappings dynamically by reading the policy directories
     *            located in /sys/devices/system/cpu/cpufreq/
     *          - If even the above strategy fails, we default to using Simple Heuristic based approach, where we divided
     *            the number of Online CPU cores as evenly as possible among the Cluster Types.
     *          - If even the above strategy fails, the function will return an Error.
     */
    void readTargetInfo();

    CGroupConfigInfo* getCGroupConfig(int32_t cGroupID);
    void getCGroupNames(std::vector<std::string>& cGroupNames);
    void getCGroupConfigs(std::vector<CGroupConfigInfo*>& cGroupConfigs);
    int32_t getCreatedCGroupsCount();

    MpamGroupConfigInfo* getMpamGroupConfig(int32_t mpamGroupID);
    void getMpamGroupNames(std::vector<std::string>& cGroupNames);
    int32_t getCreatedMpamGroupsCount();

    void displayTargetInfo();

    ErrCode addIrqAffine(std::vector<std::string>& values,
                         int8_t areClusterValues = false);
    ErrCode addLogLimit(std::vector<std::string>& values);

    static std::shared_ptr<TargetRegistry> getInstance() {
        if(targetRegistryInstance == nullptr) {
            targetRegistryInstance = std::shared_ptr<TargetRegistry>(new TargetRegistry());
        }
        return targetRegistryInstance;
    }
};

class CGroupConfigInfoBuilder {
private:
    CGroupConfigInfo* mCGroupConfigInfo;

public:
    CGroupConfigInfoBuilder();

    ErrCode setCGroupName(const std::string& cGroupName);
    ErrCode setCGroupID(const std::string& cGroupIdentifier);
    ErrCode setCreationNeeded(const std::string& creationNeeded);
    ErrCode setThreaded(const std::string& isThreaded);

    CGroupConfigInfo* build();
};

class MpamGroupConfigInfoBuilder {
private:
    MpamGroupConfigInfo* mMpamGroupInfo;

public:
    MpamGroupConfigInfoBuilder();

    ErrCode setName(const std::string& name);
    ErrCode setLgcID(const std::string& logicalID);
    ErrCode setPriority(const std::string& priority);

    MpamGroupConfigInfo* build();
};

class CacheInfoBuilder {
private:
    CacheInfo* mCacheInfo;

public:
    CacheInfoBuilder();

    ErrCode setType(const std::string& type);
    ErrCode setNumBlocks(const std::string& numBlocks);
    ErrCode setPriorityAware(const std::string& isPriorityAware);

    CacheInfo* build();
};

#endif
