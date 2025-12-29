// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <unistd.h>

#include "Utils.h"
#include "Logger.h"
#include "Extensions.h"
#include "TargetRegistry.h"
#include "ResourceRegistry.h"

static std::string getClusterTypeResourceNodePath(Resource* resource, int32_t clusterID) {
    ResConfInfo* resourceConfig =
        ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    if(resourceConfig == nullptr) return "";
    std::string filePath = resourceConfig->mResourcePath;

    // Replace %d in above file path with the actual cluster id
    char pathBuffer[128];
    std::snprintf(pathBuffer, sizeof(pathBuffer), filePath.c_str(), clusterID);
    filePath = std::string(pathBuffer);

    return filePath;
}

static std::string getCoreTypeResourceNodePath(Resource* resource, int32_t coreID) {
    ResConfInfo* resourceConfig =
        ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    if(resourceConfig == nullptr) return "";
    std::string filePath = resourceConfig->mResourcePath;

    // Replace %d in above file path with the actual core id
    char pathBuffer[128];
    std::snprintf(pathBuffer, sizeof(pathBuffer), filePath.c_str(), coreID);
    filePath = std::string(pathBuffer);

    return filePath;
}

static std::string getCGroupTypeResourceNodePath(Resource* resource, const std::string& cGroupName) {
    ResConfInfo* resourceConfig =
        ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    if(resourceConfig == nullptr) return "";
    std::string filePath = resourceConfig->mResourcePath;

    // Replace %s in above file path with the actual cgroup name
    char pathBuffer[128];
    std::snprintf(pathBuffer, sizeof(pathBuffer), filePath.c_str(), cGroupName.c_str());
    filePath = std::string(pathBuffer);

    return filePath;
}

static void truncateFile(const std::string& filePath) {
    std::ofstream ofStream(filePath, std::ofstream::out | std::ofstream::trunc);
    ofStream<<""<<std::endl;
    ofStream.close();
}

// Default Applier Callback for Resources with ApplyType = "cluster"
void defaultClusterLevelApplierCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    // Get the Cluster ID
    int32_t clusterID = resource->getClusterValue();
    std::string resourceNodePath = getClusterTypeResourceNodePath(resource, clusterID);

    // 32-bit, unit-dependent value to be written
    int32_t valueToBeWritten = resource->getValueAt(0);

    OperationStatus status = OperationStatus::SUCCESS;
    int64_t translatedValue = Multiply(static_cast<int64_t>(valueToBeWritten),
                                       static_cast<int64_t>(rConf->mUnit),
                                       status);

    if(status != OperationStatus::SUCCESS) {
        // Overflow detected, return to LONG_MAX (64-bit)
        translatedValue = std::numeric_limits<int64_t>::max();
    }

    TYPELOGV(NOTIFY_NODE_WRITE, resourceNodePath.c_str(), valueToBeWritten);
    std::ofstream resourceFileStream(resourceNodePath);
    if(!resourceFileStream.is_open()) {
        TYPELOGV(ERRNO_LOG, "open", strerror(errno));
        return;
    }

    resourceFileStream<<translatedValue<<std::endl;

    if(resourceFileStream.fail()) {
        TYPELOGV(ERRNO_LOG, "write", strerror(errno));
    }
    resourceFileStream.close();
}

// Default Tear Callback for Resources with ApplyType = "cluster"
void defaultClusterLevelTearCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    // Get the Cluster ID
    int32_t clusterID = resource->getClusterValue();
    std::string resourceNodePath = getClusterTypeResourceNodePath(resource, clusterID);
    std::string defaultValue =
        ResourceRegistry::getInstance()->getDefaultValue(resourceNodePath);

    TYPELOGV(NOTIFY_NODE_RESET, resourceNodePath.c_str(), defaultValue.c_str());
    std::ofstream resourceFileStream(resourceNodePath);
    if(!resourceFileStream.is_open()) {
        TYPELOGV(ERRNO_LOG, "open", strerror(errno));
        return;
    }

    resourceFileStream<<defaultValue<<std::endl;

    if(resourceFileStream.fail()) {
        TYPELOGV(ERRNO_LOG, "write", strerror(errno));
    }
    resourceFileStream.close();
}

static void defaultCoreLevelApplierHelper(Resource* resource, int32_t coreID) {
    std::string resourceNodePath = getCoreTypeResourceNodePath(resource, coreID);
    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    // 32-bit, unit-dependent value to be written
    int32_t valueToBeWritten = resource->getValueAt(0);

    OperationStatus status = OperationStatus::SUCCESS;
    int64_t translatedValue = Multiply(static_cast<int64_t>(valueToBeWritten),
                                       static_cast<int64_t>(rConf->mUnit),
                                       status);

    if(status != OperationStatus::SUCCESS) {
        // Overflow detected, return to LONG_MAX (64-bit)
        translatedValue = std::numeric_limits<int64_t>::max();
    }

    TYPELOGV(NOTIFY_NODE_WRITE, resourceNodePath.c_str(), valueToBeWritten);
    std::ofstream resourceFileStream(resourceNodePath);
    if(!resourceFileStream.is_open()) {
        TYPELOGV(ERRNO_LOG, "open", strerror(errno));
        return;
    }

    resourceFileStream<<translatedValue<<std::endl;

    if(resourceFileStream.fail()) {
        TYPELOGV(ERRNO_LOG, "write", strerror(errno));
    }
    resourceFileStream.close();
}

// Default Applier Callback for Resources with ApplyType = "core"
void defaultCoreLevelApplierCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    // Get the Core ID
    int32_t coreID = resource->getCoreValue();
    if(coreID == 0) {
        // Apply to all cores in the specified cluster
        int32_t clusterID = resource->getClusterValue();
        ClusterInfo* cinfo = TargetRegistry::getInstance()->getClusterInfo(clusterID);
        if(cinfo == nullptr) {
            return;
        }

        for(int32_t i = cinfo->mStartCpu; i < cinfo->mStartCpu + cinfo->mNumCpus; i++) {
            defaultCoreLevelApplierHelper(resource, i);
        }
    } else {
        defaultCoreLevelApplierHelper(resource, coreID);
    }
}

static void defaultCoreLevelTearHelper(Resource* resource, int32_t coreID) {
    std::string resourceNodePath = getClusterTypeResourceNodePath(resource, coreID);
    std::string defaultValue =
        ResourceRegistry::getInstance()->getDefaultValue(resourceNodePath);

    TYPELOGV(NOTIFY_NODE_RESET, resourceNodePath.c_str(), defaultValue.c_str());
    std::ofstream controllerFile(resourceNodePath);
    if(!controllerFile.is_open()) {
        TYPELOGV(ERRNO_LOG, "open", strerror(errno));
        return;
    }

    controllerFile<<defaultValue<<std::endl;

    if(controllerFile.fail()) {
        TYPELOGV(ERRNO_LOG, "write", strerror(errno));
    }
    controllerFile.close();
}

// Default Tear Callback for Resources with ApplyType = "core"
void defaultCoreLevelTearCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    // Get the Core ID
    int32_t coreID = resource->getCoreValue();
    if(coreID == 0) {
        // Apply to all cores in the specified cluster
        int32_t clusterID = resource->getClusterValue();
        ClusterInfo* cinfo = TargetRegistry::getInstance()->getClusterInfo(clusterID);
        if(cinfo == nullptr) {
            return;
        }

        for(int32_t i = cinfo->mStartCpu; i < cinfo->mStartCpu + cinfo->mNumCpus; i++) {
            defaultCoreLevelTearHelper(resource, i);
        }
    } else {
        defaultCoreLevelTearHelper(resource, coreID);
    }
}

// Default Applier Callback for Resources with ApplyType = "cgroup"
void defaultCGroupLevelApplierCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() != 2) return;

    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    int32_t cGroupIdentifier = resource->getValueAt(0);
    int32_t valueToBeWritten = resource->getValueAt(1);

    OperationStatus status = OperationStatus::SUCCESS;
    int64_t translatedValue = Multiply(static_cast<int64_t>(valueToBeWritten),
                                       static_cast<int64_t>(rConf->mUnit),
                                       status);

    if(status != OperationStatus::SUCCESS) {
        // Overflow detected, return to LONG_MAX (64-bit)
        translatedValue = std::numeric_limits<int64_t>::max();
    }

    // Get the corresponding cGroupConfig, this is needed to identify the
    // correct CGroup Name.
    CGroupConfigInfo* cGroupConfig =
        TargetRegistry::getInstance()->getCGroupConfig(cGroupIdentifier);

    if(cGroupConfig != nullptr) {
        const std::string cGroupName = cGroupConfig->mCgroupName;

        if(cGroupName.length() > 0) {
            std::string controllerFilePath = getCGroupTypeResourceNodePath(resource, cGroupName);

            TYPELOGV(NOTIFY_NODE_WRITE, controllerFilePath.c_str(), valueToBeWritten);
            LOGD("RESTUNE_COCO_TABLE", "Actual value to be written = " + std::to_string(translatedValue));
            std::ofstream controllerFile(controllerFilePath);
            if(!controllerFile.is_open()) {
                TYPELOGV(ERRNO_LOG, "open", strerror(errno));
                return;
            }

            controllerFile<<translatedValue<<std::endl;

            if(controllerFile.fail()) {
                TYPELOGV(ERRNO_LOG, "write", strerror(errno));
            }
            controllerFile.close();
        }
    } else {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
    }
}

// Default Tear Callback for Resources with ApplyType = "cgroup"
void defaultCGroupLevelTearCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    ResConfInfo* resourceConfigInfo =
        ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    if(resourceConfigInfo == nullptr) return;

    int32_t cGroupIdentifier = resource->getValueAt(0);
    CGroupConfigInfo* cGroupConfig =
        TargetRegistry::getInstance()->getCGroupConfig(cGroupIdentifier);

    if(cGroupConfig == nullptr) {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
        return;
    }

    const std::string cGroupName = cGroupConfig->mCgroupName;

    if(cGroupName.length() > 0) {
        std::string controllerFilePath = getCGroupTypeResourceNodePath(resource, cGroupName);
        std::string defaultValue =
            ResourceRegistry::getInstance()->getDefaultValue(controllerFilePath);

        TYPELOGV(NOTIFY_NODE_RESET, controllerFilePath.c_str(), defaultValue.c_str());
        std::ofstream controllerFile(controllerFilePath);
        if(!controllerFile.is_open()) {
            TYPELOGV(ERRNO_LOG, "open", strerror(errno));
            return;
        }

        controllerFile<<defaultValue<<std::endl;

        if(controllerFile.fail()) {
            TYPELOGV(ERRNO_LOG, "write", strerror(errno));
        }
        controllerFile.close();
    }
}

// Default Applier Callback for Resources with ApplyType = "global"
void defaultGlobalLevelApplierCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    ResConfInfo* resourceConfig =
        ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    if(resourceConfig != nullptr) {
        TYPELOGV(NOTIFY_NODE_WRITE, resourceConfig->mResourcePath.c_str(), resource->getValueAt(0));
        AuxRoutines::writeToFile(resourceConfig->mResourcePath, std::to_string(resource->getValueAt(0)));
    }
}

// Default Tear Callback for Resources with ApplyType = "global"
void defaultGlobalLevelTearCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    ResConfInfo* resourceConfig =
        ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    if(resourceConfig != nullptr) {
        std::string defaultValue =
            ResourceRegistry::getInstance()->getDefaultValue(resourceConfig->mResourcePath);

        TYPELOGV(NOTIFY_NODE_RESET, resourceConfig->mResourcePath.c_str(), defaultValue.c_str());
        AuxRoutines::writeToFile(resourceConfig->mResourcePath, defaultValue);
    }
}

// Specific callbacks for certain special Resources (which cannot be handled via the default versions)
// are listed below:
static void moveProcessToCGroup(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() < 2) return;

    int32_t cGroupIdentifier = resource->getValueAt(0);
    // Get the corresponding cGroupConfig, this is needed to identify the
    // correct CGroup Name.
    CGroupConfigInfo* cGroupConfig =
        TargetRegistry::getInstance()->getCGroupConfig(cGroupIdentifier);

    if(cGroupConfig == nullptr) {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
        return;
    }

    // mCgroupName has the entire name from InitConfig.yaml. Ex- system.slice/camera-cgroup
    const std::string cGroupName = cGroupConfig->mCgroupName;
    if(cGroupName.length() == 0) {
        return;
    }

    std::string controllerFilePath = getCGroupTypeResourceNodePath(resource, cGroupName);
    for(int32_t i = 1; i < resource->getValuesCount(); i++) {
        int32_t pid = resource->getValueAt(i);
        std::string currentCGroupFilePath = "/proc/" + std::to_string(pid) + "/cgroup";
        std::string currentCGroup = AuxRoutines::readFromFile(currentCGroupFilePath);

        if(currentCGroup.length() > 4) {
            currentCGroup = currentCGroup.substr(4);
            ResourceRegistry::getInstance()->addDefaultValue(currentCGroupFilePath, currentCGroup);
        }

        TYPELOGV(NOTIFY_NODE_WRITE, controllerFilePath.c_str(), pid);
        std::ofstream controllerFile(controllerFilePath);
        if(!controllerFile.is_open()) {
            TYPELOGV(ERRNO_LOG, "open", strerror(errno));
            return;
        }

        controllerFile<<pid<<std::endl;

        if(controllerFile.fail()) {
            TYPELOGV(ERRNO_LOG, "write", strerror(errno));
        }
        controllerFile.close();
    }
}

static void moveThreadToCGroup(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() < 2) return;

    int32_t cGroupIdentifier = resource->getValueAt(0);
    // Get the corresponding cGroupConfig, this is needed to identify the
    // correct CGroup Name.
    CGroupConfigInfo* cGroupConfig =
        TargetRegistry::getInstance()->getCGroupConfig(cGroupIdentifier);

    if(cGroupConfig == nullptr) {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
        return;
    }

    const std::string cGroupName = cGroupConfig->mCgroupName;
    if(cGroupName.length() == 0) {
        return;
    }

    std::string controllerFilePath = getCGroupTypeResourceNodePath(resource, cGroupName);
    for(int32_t i = 1; i < resource->getValuesCount(); i++) {
        int32_t tid = resource->getValueAt(i);

        TYPELOGV(NOTIFY_NODE_WRITE, controllerFilePath.c_str(), tid);
        std::ofstream controllerFile(controllerFilePath);
        if(!controllerFile.is_open()) {
            TYPELOGV(ERRNO_LOG, "open", strerror(errno));
            return;
        }

        controllerFile<<tid<<std::endl;

        if(controllerFile.fail()) {
            TYPELOGV(ERRNO_LOG, "write", strerror(errno));
        }
        controllerFile.close();
    }
}

static void setRunOnCores(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() < 2) return;

    int32_t cGroupIdentifier = resource->getValueAt(0);
    CGroupConfigInfo* cGroupConfig = TargetRegistry::getInstance()->getCGroupConfig(cGroupIdentifier);

    if(cGroupConfig != nullptr) {
        const std::string cGroupName = cGroupConfig->mCgroupName;

        if(cGroupName.length() > 0) {
            std::string cpusString = "";
            for(int32_t i = 1; i < resource->getValuesCount(); i++) {
                int32_t curVal = resource->getValueAt(i);
                cpusString += std::to_string(curVal);
                if(resource->getValuesCount() > 2 && i < resource->getValuesCount() - 1) {
                    cpusString.push_back(',');
                }
            }

            std::string controllerFilePath = getCGroupTypeResourceNodePath(resource, cGroupName);

            TYPELOGV(NOTIFY_NODE_WRITE_S, controllerFilePath.c_str(), cpusString.c_str());
            std::ofstream controllerFile(controllerFilePath);
            if(!controllerFile.is_open()) {
                TYPELOGV(ERRNO_LOG, "open", strerror(errno));
                return;
            }

            controllerFile<<cpusString<<std::endl;

            if(controllerFile.fail()) {
                TYPELOGV(ERRNO_LOG, "write", strerror(errno));
            }
            controllerFile.close();
        }
    } else {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
    }
}

static void setRunOnCoresExclusively(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() < 2) return;

    int32_t cGroupIdentifier = resource->getValueAt(0);
    CGroupConfigInfo* cGroupConfig = TargetRegistry::getInstance()->getCGroupConfig(cGroupIdentifier);

    if(cGroupConfig != nullptr) {
        const std::string cGroupName = cGroupConfig->mCgroupName;

        if(cGroupName.length() > 0) {
            const std::string cGroupControllerFilePath =
                UrmSettings::mBaseCGroupPath + cGroupName + "/cpuset.cpus";

            std::string cpusString = "";
            for(int32_t i = 1; i < resource->getValuesCount(); i++) {
                int32_t curVal = resource->getValueAt(i);
                cpusString += std::to_string(curVal);
                if(resource->getValuesCount() > 2 && i < resource->getValuesCount() - 1) {
                    cpusString.push_back(',');
                }
            }

            TYPELOGV(NOTIFY_NODE_WRITE_S, cGroupControllerFilePath.c_str(), cpusString.c_str());
            std::ofstream controllerFile(cGroupControllerFilePath);
            if(!controllerFile.is_open()) {
                TYPELOGV(ERRNO_LOG, "open", strerror(errno));
                return;
            }
            controllerFile<<cpusString<<std::endl;
            controllerFile.close();

            const std::string cGroupCpusetPartitionFilePath =
                UrmSettings::mBaseCGroupPath + cGroupName + "/cpuset.cpus.partition";

            std::ofstream partitionFile(cGroupCpusetPartitionFilePath);
            if(!partitionFile.is_open()) {
                TYPELOGV(ERRNO_LOG, "open", strerror(errno));
                return;
            }

            partitionFile<<"isolated"<<std::endl;
            partitionFile.close();
        }
    } else {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
    }
}

static void limitCpuTime(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() != 3) return;

    int32_t cGroupIdentifier = resource->getValueAt(0);
    int32_t maxUsageMicroseconds = resource->getValueAt(1);
    int32_t periodMicroseconds = resource->getValueAt(2);
    CGroupConfigInfo* cGroupConfig = TargetRegistry::getInstance()->getCGroupConfig(cGroupIdentifier);

    if(cGroupConfig != nullptr) {
        const std::string cGroupName = cGroupConfig->mCgroupName;

        if(cGroupName.length() > 0) {
            std::string controllerFilePath = getCGroupTypeResourceNodePath(resource, cGroupName);
            std::ofstream controllerFile(controllerFilePath);

            if(!controllerFile.is_open()) {
                TYPELOGV(ERRNO_LOG, "open", strerror(errno));
                return;
            }

            controllerFile<<maxUsageMicroseconds<<" "<<periodMicroseconds<<std::endl;

            if(controllerFile.fail()) {
                TYPELOGV(ERRNO_LOG, "write", strerror(errno));
            }
            controllerFile.close();
        }
    } else {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
    }
}

static void removeProcessFromCGroup(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() < 2) return;

    for(int32_t i = 1; i < resource->getValuesCount(); i++) {
        int32_t pid = resource->getValueAt(i);

        std::string cGroupPath =
            ResourceRegistry::getInstance()->getDefaultValue("/proc/" + std::to_string(pid) + "/cgroup");
        if(cGroupPath.length() == 0) {
            cGroupPath = UrmSettings::mBaseCGroupPath + "cgroup.procs";
        } else {
            cGroupPath =  UrmSettings::mBaseCGroupPath + cGroupPath + "/cgroup.procs";
        }

        LOGD("RESTUNE_COCO_TABLE", "Moving PID: " + std::to_string(pid) + " to: " + cGroupPath);
        std::ofstream controllerFile(cGroupPath, std::ios::app);
        if(!controllerFile.is_open()) {
            TYPELOGV(ERRNO_LOG, "open", strerror(errno));
            return;
        }

        controllerFile<<pid<<std::endl;

        if(controllerFile.fail()) {
            TYPELOGV(ERRNO_LOG, "write", strerror(errno));
        }
        controllerFile.close();
    }
}

static void removeThreadFromCGroup(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() < 2) return;

    for(int32_t i = 1; i < resource->getValuesCount(); i++) {
        int32_t tid = resource->getValueAt(i);
        std::string cGroupPath = UrmSettings::mBaseCGroupPath + cGroupPath + "/cgroup.threads";

        LOGD("RESTUNE_COCO_TABLE", "Moving TID: " + std::to_string(tid) + " to: " + cGroupPath);
        std::ofstream controllerFile(cGroupPath, std::ios::app);
        if(!controllerFile.is_open()) {
            TYPELOGV(ERRNO_LOG, "open", strerror(errno));
            return;
        }

        controllerFile<<tid<<std::endl;

        if(controllerFile.fail()) {
            TYPELOGV(ERRNO_LOG, "write", strerror(errno));
        }
        controllerFile.close();
    }
}

static void resetRunOnCoresExclusively(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    if(resource->getValuesCount() < 2) return;

    int32_t cGroupIdentifier = resource->getValueAt(0);
    CGroupConfigInfo* cGroupConfig = TargetRegistry::getInstance()->getCGroupConfig(cGroupIdentifier);

    if(cGroupConfig != nullptr) {
        const std::string cGroupName = cGroupConfig->mCgroupName;

        if(cGroupName.length() > 0) {
            const std::string cGroupCpuSetFilePath =
                UrmSettings::mBaseCGroupPath + cGroupName + "/cpuset.cpus";

            std::string defaultValue =
                ResourceRegistry::getInstance()->getDefaultValue(cGroupCpuSetFilePath);

            TYPELOGV(NOTIFY_NODE_RESET, cGroupCpuSetFilePath.c_str(), defaultValue.c_str());
            std::ofstream controllerFile(cGroupCpuSetFilePath);
            if(!controllerFile.is_open()) {
                TYPELOGV(ERRNO_LOG, "open", strerror(errno));
                return;
            }

            controllerFile<<defaultValue<<std::endl;

            if(controllerFile.fail()) {
                TYPELOGV(ERRNO_LOG, "write", strerror(errno));
            }
            controllerFile.close();

            const std::string cGroupCpusetPartitionFilePath =
                UrmSettings::mBaseCGroupPath + cGroupName + "/cpuset.cpus.partition";

            std::ofstream partitionFile(cGroupCpusetPartitionFilePath);
            if(!partitionFile.is_open()) {
                TYPELOGV(ERRNO_LOG, "open", strerror(errno));
                return;
            }

            defaultValue = ResourceRegistry::getInstance()->getDefaultValue(cGroupCpusetPartitionFilePath);

            partitionFile<<defaultValue<<std::endl;

            if(partitionFile.fail()) {
                TYPELOGV(ERRNO_LOG, "write", strerror(errno));
            }
            partitionFile.close();
        }
    } else {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
    }
}

static void no_op(void* context) {
    return;
}

static void setPmQos(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    if(resource->getValuesCount() < 1) return;

    int32_t value = resource->getValueAt(0);
    int32_t clusterID = resource->getClusterValue();

    ClusterInfo* cinfo = TargetRegistry::getInstance()->getClusterInfo(clusterID);
    if(cinfo == nullptr) {
        return;
    }

    for(int32_t i = cinfo->mStartCpu; i < cinfo->mStartCpu + cinfo->mNumCpus; i++) {
        defaultCoreLevelApplierHelper(resource, i);
    }
}

static void resetPmQos(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    if(resource->getValuesCount() < 1) return;

    int32_t value = resource->getValueAt(0);
    int32_t clusterID = resource->getClusterValue();

    ClusterInfo* cinfo = TargetRegistry::getInstance()->getClusterInfo(clusterID);
    if(cinfo == nullptr) {
        return;
    }

    for(int32_t i = cinfo->mStartCpu; i < cinfo->mStartCpu + cinfo->mNumCpus; i++) {
        defaultCoreLevelTearHelper(resource, i);
    }
}

// Register the specific Callbacks
RESTUNE_REGISTER_APPLIER_CB(0x00010001, setPmQos);
RESTUNE_REGISTER_APPLIER_CB(0x00090000, moveProcessToCGroup);
RESTUNE_REGISTER_APPLIER_CB(0x00090001, moveThreadToCGroup);
RESTUNE_REGISTER_APPLIER_CB(0x00090002, setRunOnCores);
RESTUNE_REGISTER_APPLIER_CB(0x00090003, setRunOnCoresExclusively);
RESTUNE_REGISTER_APPLIER_CB(0x00090005, limitCpuTime);
RESTUNE_REGISTER_TEAR_CB(0x00010001, resetPmQos);
RESTUNE_REGISTER_TEAR_CB(0x00090000, removeProcessFromCGroup);
RESTUNE_REGISTER_TEAR_CB(0x00090001, removeThreadFromCGroup);
RESTUNE_REGISTER_TEAR_CB(0x00090003, resetRunOnCoresExclusively);
