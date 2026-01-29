// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#include "TargetRegistry.h"

// Create all the CGroups specified via InitConfig.yaml during the init phase.
static ErrCode createCGroup(CGroupConfigInfo* cGroupConfig) {
    if(cGroupConfig == nullptr) return RC_BAD_ARG;
    if(!cGroupConfig->mCreationNeeded) return RC_SUCCESS;

    std::string cGroupPath = UrmSettings::mBaseCGroupPath + cGroupConfig->mCgroupName;
    errno = 0;
    if(mkdir(cGroupPath.c_str(), 0755) == 0) {
        if(cGroupConfig->mIsThreaded) {
            AuxRoutines::writeToFile(cGroupPath + "/cgroup.type", "threaded");
        }
    } else {
        if(errno == EEXIST) {
            return RC_SUCCESS; // Already exists, treat as success
        } else {
            TYPELOGV(ERRNO_LOG, "mkdir", strerror(errno));
            return RC_CGROUP_CREATION_FAILURE;
        }
    }

    return RC_SUCCESS;
}

static ErrCode createMpamGroup(MpamGroupConfigInfo* mpamGroupConfig) {
    if(mpamGroupConfig == nullptr) return RC_BAD_ARG;

    return RC_SUCCESS;
}

// Dynamic Utilities to fetch Target Info
// Number of CPUs currently online
static int32_t getOnlineCpuCount() {
    std::ifstream file;
    try {
        file.open(ONLINE_CPU_FILE_PATH);
        if(!file.is_open()) {
            LOGE("RESTUNE_SERVER_INIT", "Failed to Open file: " ONLINE_CPU_FILE_PATH);
            return 0;
        }

        std::string line;
        std::getline(file, line);

        std::stringstream cpuRangeStream(line);
        std::string token;
        int32_t cpuIndex = 0;

        while(std::getline(cpuRangeStream, token, '-')) {
            cpuIndex = std::max(cpuIndex, std::stoi(token));
        }

        return cpuIndex + 1;

    } catch(const std::exception& e) {
        TYPELOGV(CORE_COUNT_EXTRACTION_FAILED, e.what());
    }

    return 0;
}

// This routine gets the list of cpus corresponding to a particular Cluster ID.
// For example: C0 -> cpu0, cpu1, cpu2
static ErrCode readRelatedCpus(const std::string& policyPath,
                               std::vector<int32_t>& cpus) {
    try {
        std::ifstream file;

        file.open(policyPath + "/related_cpus");
        if(!file.is_open()) {
            LOGE("RESTUNE_SERVER_INIT", "Failed to Open file: " + policyPath + "/related_cpus");
            return RC_FILE_NOT_FOUND;
        }

        std::string line;
        if(getline(file, line)) {
            size_t start = 0;
            while(start < line.size()) {
                size_t end = line.find(' ', start);
                std::string token = line.substr(start, end - start);
                if(!token.empty()) {
                    cpus.push_back(static_cast<int8_t>(stoi(token)));
                }
                if(end == std::string::npos) {
                    break;
                }
                start = end + 1;
            }
        }
    } catch(const std::exception& e) {
        TYPELOGV(CLUSTER_CPU_LIST_EXTRACTION_FAILED, policyPath.c_str(), e.what());
    }

    return RC_SUCCESS;
}

// Get the capacity of a cpu, indexed by it's integer ID.
static int32_t readCpuCapacity(int32_t cpuID) {
    char filePath[128];
    snprintf(filePath, sizeof(filePath), CPU_CAPACITY_FILE_PATH, cpuID);
    std::string capacityString = AuxRoutines::readFromFile(filePath);
    int32_t capacity = 0;

    try {
        capacity = std::stoi(capacityString);
    } catch(std::exception& e) {
        TYPELOGV(CLUSTER_CPU_CAPACITY_EXTRACTION_FAILED, cpuID, e.what());
    }

    return capacity;
}

void TargetRegistry::generatePolicyBasedMapping(std::vector<std::string>& policyDirs) {
    // Sort the directories, to ensure processing always starts with policy0
    std::sort(policyDirs.begin(), policyDirs.end());

    // Number of policy directories, is equivalent to number of clusters
    UrmSettings::targetConfigs.mTotalClusterCount = policyDirs.size();

    // Next, get the list of cpus corresponding to each cluster
    std::vector<std::pair<int32_t, std::pair<int32_t, ClusterInfo*>>> clusterConfigs;

    int8_t physicalClusterId = 0;
    for(const std::string& dirName : policyDirs) {
        std::string fullPath = std::string(POLICY_DIR_PATH) + dirName;
        std::vector<int32_t> cpuList;

        if(RC_IS_OK(readRelatedCpus(fullPath, cpuList))) {
            int32_t clusterCapacity = 0;
            ClusterInfo* clusterInfo = new ClusterInfo;
            clusterInfo->mPhysicalID = physicalClusterId;
            clusterInfo->mNumCpus = cpuList.size();

            if(!cpuList.empty()) {
                // If this cluster has a non-zero number of cores, then
                // proceed with determining the Cluster Capcity
                int32_t cpuID = cpuList[0];
                clusterInfo->mStartCpu = cpuID;
                for(int32_t i = 0; i < (int32_t)cpuList.size(); i++) {
                    clusterInfo->mStartCpu = std::min(clusterInfo->mStartCpu, cpuList[i]);
                }

                clusterCapacity = readCpuCapacity(cpuID);
            }

            clusterInfo->mCapacity = clusterCapacity;
            clusterConfigs.push_back({clusterCapacity, {clusterInfo->mPhysicalID, clusterInfo}});
            this->mPhysicalClusters[clusterInfo->mPhysicalID] = clusterInfo;
            physicalClusterId += cpuList.size();
        }
    }

    std::sort(clusterConfigs.begin(), clusterConfigs.end());

    // Now, Create the Logical to Physical Mappings
    // Note the Clusters are arranged in increasing order of Capacities
    for(size_t i = 0; i < clusterConfigs.size(); i++) {
        this->mLogicalToPhysicalClusterMapping[i] = clusterConfigs[i].second.second->mPhysicalID;
    }
}

void TargetRegistry::getClusterIdBasedMapping() {
    const std::string cpuDir = "/sys/devices/system/cpu";
    const std::regex cpuRegex("^cpu([0-9]+)$");

    DIR* dir = opendir(cpuDir.c_str());
    if(dir == nullptr) {
        return;
    }

    std::unordered_map<int32_t, std::vector<int32_t>> clusterToCoreMap;

    struct dirent* entry;
    while((entry = readdir(dir)) != nullptr) {
        if(entry->d_type == DT_DIR) {
            std::cmatch match;
            if(std::regex_match(entry->d_name, match, cpuRegex)) {
                int32_t cpuNum = -1;
                try {
                    cpuNum = std::stoi(match[1]);
                } catch(const std::exception& e) {
                    break;
                }

                std::string clusterIdPath = cpuDir + "/" + entry->d_name + "/topology/cluster_id";
                std::ifstream clusterIdFile(clusterIdPath);
                if(clusterIdFile) {
                    int32_t clusterID;
                    clusterIdFile>>clusterID;
                    if(clusterIdFile) {
                        clusterToCoreMap[clusterID].push_back(cpuNum);
                    }
                }
            }
        }
    }

    closedir(dir);

    UrmSettings::targetConfigs.mTotalClusterCount = clusterToCoreMap.size();

    for(std::pair<int32_t, std::vector<int32_t>> entry: clusterToCoreMap) {
        int32_t clusterID = entry.first;
        ClusterInfo* clusterInfo = new ClusterInfo;
        clusterInfo->mPhysicalID = clusterID;
        clusterInfo->mNumCpus = entry.second.size();

        if(entry.second.size() > 0) {
            clusterInfo->mStartCpu = entry.second[0];

            for(int32_t cpu: entry.second) {
                clusterInfo->mStartCpu = std::min(clusterInfo->mStartCpu, cpu);
            }
        }
        this->mPhysicalClusters[clusterID] = clusterInfo;
    }

    // Next, get the list of cpus corresponding to each cluster
    std::vector<std::pair<int32_t, ClusterInfo*>> clusterConfigs;

    for(std::pair<int32_t, ClusterInfo*> entry: this->mPhysicalClusters) {
        int32_t clusterCapacity = 0;
        if(entry.second != nullptr) {
            // If this cluster has a non-zero number of cores, then
            // proceed with determining the Cluster Capcity
            clusterCapacity = readCpuCapacity(entry.second->mStartCpu);
        }
        clusterConfigs.push_back({clusterCapacity, entry.second});
    }

    std::sort(clusterConfigs.begin(), clusterConfigs.end());

    // Now, Create the Logical to Physical Mappings
    // Note the Clusters are arranged in increasing order of Capacities
    for(size_t i = 0; i < clusterConfigs.size(); i++) {
        this->mLogicalToPhysicalClusterMapping[i] = clusterConfigs[i].second->mPhysicalID;
    }
}

std::shared_ptr<TargetRegistry> TargetRegistry::targetRegistryInstance = nullptr;
TargetRegistry::TargetRegistry() {}

void TargetRegistry::addClusterMapping(const std::string& logicalIDString, const std::string& physicalIDString) {
    try {
        int32_t logicalID = std::stoi(logicalIDString);
        int32_t physicalID = std::stoi(physicalIDString);

        if(this->mPhysicalClusters.find(physicalID) == this->mPhysicalClusters.end()) {
            ClusterInfo* clusterInfo = new ClusterInfo;

            clusterInfo->mPhysicalID = physicalID;
            clusterInfo->mNumCpus = 0;
            clusterInfo->mCapacity = 0;
            clusterInfo->mStartCpu = physicalID;

            this->mPhysicalClusters[physicalID] = clusterInfo;
        }

        this->mLogicalToPhysicalClusterMapping[logicalID] = physicalID;
        UrmSettings::targetConfigs.mTotalClusterCount = this->mPhysicalClusters.size();

    } catch(const std::exception& e) {

    }
}

void TargetRegistry::addClusterSpreadInfo(const std::string& physicalIDString, const std::string& numCoresString) {
    try {
        int32_t physicalID = std::stoi(physicalIDString);
        int32_t numCores = std::stoi(numCoresString);

        if(this->mPhysicalClusters.find(physicalID) == this->mPhysicalClusters.end()) {
            return;
        }

        this->mPhysicalClusters[physicalID]->mNumCpus = numCores;
    } catch(const std::exception& e) {

    }
}

void TargetRegistry::readTargetInfo() {
    // Get the Online Core Count
    UrmSettings::targetConfigs.mTotalCoreCount = getOnlineCpuCount();

    // Check if cpufreq/policy directories are available,
    // If yes, we'll use them to generate the mapping info.

    // Get the list of all the policy/ directories
    std::vector<std::string> policyDirs;

    DIR* dir = opendir(POLICY_DIR_PATH);
    if(dir == nullptr) {
        return;
    }

    struct dirent* entry;
    while((entry = readdir(dir)) != nullptr) {
        if(strncmp(entry->d_name, "policy", 6) == 0) {
            policyDirs.push_back(entry->d_name);
        }
    }
    closedir(dir);

    if(policyDirs.size() > 0) {
        // cpufreq/policy directories are available, generate mapping.
        this->generatePolicyBasedMapping(policyDirs);
        return;
    }

    // Fallback:
    // Generate mapping using topology/cluster_id node
    this->getClusterIdBasedMapping();
}

void TargetRegistry::getClusterIDs(std::vector<int32_t>& clusterIDs) {
    for(std::pair<int32_t, int32_t> info: this->mLogicalToPhysicalClusterMapping) {
        clusterIDs.push_back(info.second);
    }
}

// Get the Physical Cluster corresponding to a Logical Cluster Id.
int32_t TargetRegistry::getPhysicalClusterId(int32_t logicalClusterId) {
    if(this->mLogicalToPhysicalClusterMapping.find(logicalClusterId) ==
       this->mLogicalToPhysicalClusterMapping.end()) {
        return -1;
    }

    return this->mLogicalToPhysicalClusterMapping[logicalClusterId];
}

// Get the nth Physical Core ID in a particular cluster
int32_t TargetRegistry::getPhysicalCoreId(int32_t logicalClusterId, int32_t logicalCoreCount) {
    if(this->mPhysicalClusters.size() == 0) {
        // In case there are no clusters, i.e. the system is homogeneous then
        // physical core count will be the same as logical core count.
        return logicalCoreCount;
    }

    if(this->mLogicalToPhysicalClusterMapping.find(logicalClusterId) ==
       this->mLogicalToPhysicalClusterMapping.end()) {
        return -1;
    }

    int32_t physicalClusterId = this->mLogicalToPhysicalClusterMapping[logicalClusterId];
    ClusterInfo* clusterInfo = this->mPhysicalClusters[physicalClusterId];

    if(clusterInfo == nullptr || logicalCoreCount <= 0 || logicalCoreCount > clusterInfo->mNumCpus) {
        return -1;
    }

    return clusterInfo->mStartCpu + logicalCoreCount - 1;
}

ClusterInfo* TargetRegistry::getClusterInfo(int32_t physicalClusterID) {
    return this->mPhysicalClusters[physicalClusterID];
}

void TargetRegistry::getCGroupNames(std::vector<std::string>& cGroupNames) {
    for(std::pair<int32_t, CGroupConfigInfo*> cGroup: this->mCGroupMapping) {
        cGroupNames.push_back(cGroup.second->mCgroupName);
    }
}

CGroupConfigInfo* TargetRegistry::getCGroupConfig(int32_t cGroupID) {
    return this->mCGroupMapping[cGroupID];
}

void TargetRegistry::getCGroupConfigs(std::vector<CGroupConfigInfo*>& cGroupConfigs) {
    for(std::pair<int32_t, CGroupConfigInfo*> cGroup: this->mCGroupMapping) {
        cGroupConfigs.push_back(cGroup.second);
    }
}

int32_t TargetRegistry::getCreatedCGroupsCount() {
    return this->mCGroupMapping.size();
}

void TargetRegistry::getMpamGroupNames(std::vector<std::string>& mpamGroupNames) {
    for(std::pair<int32_t, MpamGroupConfigInfo*> mpamGroup: this->mMpamGroupMapping) {
        mpamGroupNames.push_back(mpamGroup.second->mMpamGroupName);
    }
}

MpamGroupConfigInfo* TargetRegistry::getMpamGroupConfig(int32_t mpamGroupID) {
    return this->mMpamGroupMapping[mpamGroupID];
}

int32_t TargetRegistry::getCreatedMpamGroupsCount() {
    return this->mMpamGroupMapping.size();
}

void TargetRegistry::displayTargetInfo() {
    LOGI("RESTUNE_SERVER_INIT", "Displaying Target Info");
    LOGI("RESTUNE_SERVER_INIT", "Number of Cores = " + std::to_string(UrmSettings::targetConfigs.mTotalCoreCount));
    LOGI("RESTUNE_SERVER_INIT", "Number of Clusters = " + std::to_string(UrmSettings::targetConfigs.mTotalClusterCount));

    for(std::pair<int32_t, ClusterInfo*> cluster: this->mPhysicalClusters) {
        LOGI("RESTUNE_SERVER_INIT", "Physical ID of cluster: " + std::to_string(cluster.first));
        LOGI("RESTUNE_SERVER_INIT", "Number of Cores in cluster: " + std::to_string(cluster.second->mNumCpus));
        LOGI("RESTUNE_SERVER_INIT", "Starting CPU in this cluster: " + std::to_string(cluster.second->mStartCpu));
        LOGI("RESTUNE_SERVER_INIT", "Cluster Capacity: " + std::to_string(cluster.second->mCapacity));
    }
}

TargetRegistry::~TargetRegistry() {
    for(std::pair<int32_t, CGroupConfigInfo*> cGroupInfo: this->mCGroupMapping) {
        if(cGroupInfo.second == nullptr) return;

        if(cGroupInfo.second != nullptr) {
            delete cGroupInfo.second;
            cGroupInfo.second = nullptr;
        }
    }

    for(std::pair<int32_t, ClusterInfo*> clusterInfo: this->mPhysicalClusters) {
        if(clusterInfo.second == nullptr) return;

        if(clusterInfo.second != nullptr) {
            delete clusterInfo.second;
            clusterInfo.second = nullptr;
        }
    }

    for(std::pair<int32_t, MpamGroupConfigInfo*> mpamInfo: this->mMpamGroupMapping) {
        if(mpamInfo.second == nullptr) return;

        if(mpamInfo.second != nullptr) {
            delete mpamInfo.second;
            mpamInfo.second = nullptr;
        }
    }

    for(std::pair<std::string, CacheInfo*> cacheInfo: this->mCacheInfoMapping) {
        if(cacheInfo.second == nullptr) return;

        if(cacheInfo.second != nullptr) {
            delete cacheInfo.second;
            cacheInfo.second = nullptr;
        }
    }
}

void TargetRegistry::addCGroupMapping(CGroupConfigInfo* cGroupConfigInfo) {
    if(cGroupConfigInfo == nullptr) return;

    if(cGroupConfigInfo->mCgroupID == -1 || cGroupConfigInfo->mCgroupName.length() == 0) {
        delete cGroupConfigInfo;
        return;
    }

    if(RC_IS_OK(createCGroup(cGroupConfigInfo))) {
        this->mCGroupMapping[cGroupConfigInfo->mCgroupID] = cGroupConfigInfo;
    } else {
        delete(cGroupConfigInfo);
    }
}

void TargetRegistry::addMpamGroupMapping(MpamGroupConfigInfo* mpamGroupConfigInfo) {
    if(mpamGroupConfigInfo == nullptr) return;

    if(mpamGroupConfigInfo->mMpamGroupInfoID == -1 || mpamGroupConfigInfo->mMpamGroupName.length() == 0) {
        delete mpamGroupConfigInfo;
        return;
    }

    if(RC_IS_OK(createMpamGroup(mpamGroupConfigInfo))) {
        this->mMpamGroupMapping[mpamGroupConfigInfo->mMpamGroupInfoID] = mpamGroupConfigInfo;
    }
}

void TargetRegistry::addCacheInfoMapping(CacheInfo* cacheInfo) {
    if(cacheInfo == nullptr) return;

    if(cacheInfo->mCacheType.length() == 0 || cacheInfo->mNumCacheBlocks == -1) {
        delete cacheInfo;
        return;
    }

    this->mCacheInfoMapping[cacheInfo->mCacheType] = cacheInfo;
}

// Methods for Building CGroup Config from InitConfigs.yaml
CGroupConfigInfoBuilder::CGroupConfigInfoBuilder() {
    this->mCGroupConfigInfo = new(std::nothrow) CGroupConfigInfo;

    if(this->mCGroupConfigInfo == nullptr) {
        return;
    }

    this->mCGroupConfigInfo->mCgroupName = "";
    this->mCGroupConfigInfo->mCgroupID = -1;
    this->mCGroupConfigInfo->mCreationNeeded = false;
    this->mCGroupConfigInfo->mIsThreaded = false;
}

ErrCode CGroupConfigInfoBuilder::setCGroupName(const std::string& name) {
    if(this->mCGroupConfigInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mCGroupConfigInfo->mCgroupName = name;
    return RC_SUCCESS;
}

ErrCode CGroupConfigInfoBuilder::setCGroupID(const std::string& cGroupIDString) {
    if(this->mCGroupConfigInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mCGroupConfigInfo->mCgroupID = -1;
    try {
        this->mCGroupConfigInfo->mCgroupID = std::stoi(cGroupIDString);
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_INVALID_VALUE;
}

ErrCode CGroupConfigInfoBuilder::setCreationNeeded(const std::string& creationNeededString) {
    if(this->mCGroupConfigInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mCGroupConfigInfo->mCreationNeeded = (creationNeededString == "true");

    return RC_SUCCESS;
}

ErrCode CGroupConfigInfoBuilder::setThreaded(const std::string& isThreadedString) {
    if(this->mCGroupConfigInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mCGroupConfigInfo->mIsThreaded = (isThreadedString == "true");

    return RC_SUCCESS;
}

CGroupConfigInfo* CGroupConfigInfoBuilder::build() {
    return this->mCGroupConfigInfo;
}

MpamGroupConfigInfoBuilder::MpamGroupConfigInfoBuilder() {
    this->mMpamGroupInfo = new(std::nothrow) MpamGroupConfigInfo;
    if(this->mMpamGroupInfo == nullptr) {
        return;
    }

    this->mMpamGroupInfo->mMpamGroupInfoID = -1;
    this->mMpamGroupInfo->mMpamGroupName = "";
    this->mMpamGroupInfo->mPriority = 0;
}

ErrCode MpamGroupConfigInfoBuilder::setName(const std::string& name) {
    if(this->mMpamGroupInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mMpamGroupInfo->mMpamGroupName = name;
    return RC_SUCCESS;
}

ErrCode MpamGroupConfigInfoBuilder::setLgcID(const std::string& logicalIDString) {
    if(this->mMpamGroupInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    try {
        int32_t logicalID = std::stoi(logicalIDString);
        this->mMpamGroupInfo->mMpamGroupInfoID = logicalID;
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_INVALID_VALUE;
}

ErrCode MpamGroupConfigInfoBuilder::setPriority(const std::string& priorityString) {
    if(this->mMpamGroupInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    try {
        this->mMpamGroupInfo->mPriority = std::stoi(priorityString);
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }
}

MpamGroupConfigInfo* MpamGroupConfigInfoBuilder::build() {
    return this->mMpamGroupInfo;
}

CacheInfoBuilder::CacheInfoBuilder() {
    this->mCacheInfo = new(std::nothrow) CacheInfo;
    if(this->mCacheInfo == nullptr) {
        return;
    }

    this->mCacheInfo->mPriorityAware = false;
    this->mCacheInfo->mCacheType = "";
    this->mCacheInfo->mNumCacheBlocks = -1;
}

ErrCode CacheInfoBuilder::setType(const std::string& type) {
    if(this->mCacheInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mCacheInfo->mCacheType = type;
    return RC_SUCCESS;
}

ErrCode CacheInfoBuilder::setNumBlocks(const std::string& numBlocksString) {
    if(this->mCacheInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    try {
        int32_t numBlocks = std::stoi(numBlocksString);
        this->mCacheInfo->mNumCacheBlocks = numBlocks;
        return RC_SUCCESS;

    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_INVALID_VALUE;
}

ErrCode CacheInfoBuilder::setPriorityAware(const std::string& isPriorityAwareString) {
    if(this->mCacheInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    this->mCacheInfo->mPriorityAware = (isPriorityAwareString == "true");

    return RC_SUCCESS;
}

CacheInfo* CacheInfoBuilder::build() {
    return this->mCacheInfo;
}
