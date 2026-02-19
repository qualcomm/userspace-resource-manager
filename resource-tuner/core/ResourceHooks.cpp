// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <unistd.h>
#include <sched.h>

#include "Utils.h"
#include "Logger.h"
#include "Extensions.h"
#include "TargetRegistry.h"
#include "ResourceRegistry.h"

static std::string getFullResourceNodePath(ResConfInfo* rConf, int32_t id) {
    if(rConf == nullptr) return "";
    std::string filePath = rConf->mResourcePath;

    // Replace %d in above file path with the actual cluster id
    char pathBuffer[128];
    std::snprintf(pathBuffer, sizeof(pathBuffer), filePath.c_str(), id);
    filePath = std::string(pathBuffer);

    return filePath;
}

static std::string getCGroupTypeResourceNodePath(Resource* resource, const std::string& cGroupName) {
    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    if(rConf == nullptr) return "";
    std::string filePath = rConf->mResourcePath;

    // Replace %s in above file path with the actual cgroup name
    char pathBuffer[128];
    std::snprintf(pathBuffer, sizeof(pathBuffer), filePath.c_str(), cGroupName.c_str());
    filePath = std::string(pathBuffer);

    return filePath;
}

// Default Applier Callback for Resources with ApplyType = "cluster"
void defaultClusterLevelApplierCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    if(rConf == nullptr) return;

    // Get the Cluster ID
    int32_t clusterID = resource->getClusterValue();
    std::string resourceNodePath = getFullResourceNodePath(rConf, clusterID);

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

    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    if(rConf == nullptr) return;

    // Get the Cluster ID
    int32_t clusterID = resource->getClusterValue();
    std::string resourceNodePath = getFullResourceNodePath(rConf, clusterID);
    std::string defVal = ResourceRegistry::getInstance()->getDefaultValue(resourceNodePath);

    TYPELOGV(NOTIFY_NODE_RESET, resourceNodePath.c_str(), defVal.c_str());
    std::ofstream resourceFileStream(resourceNodePath);
    if(!resourceFileStream.is_open()) {
        TYPELOGV(ERRNO_LOG, "open", strerror(errno));
        return;
    }

    resourceFileStream<<defVal<<std::endl;

    if(resourceFileStream.fail()) {
        TYPELOGV(ERRNO_LOG, "write", strerror(errno));
    }
    resourceFileStream.close();
}

static void defaultCoreLevelApplierHelper(Resource* resource, int32_t coreID) {
    if(resource == nullptr) return;
    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    if(rConf == nullptr) return;

    std::string resourceNodePath = getFullResourceNodePath(rConf, coreID);

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

    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    if(rConf == nullptr) return;

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
    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    if(rConf == nullptr) return;

    std::string resourceNodePath = getFullResourceNodePath(rConf, coreID);
    std::string defVal = ResourceRegistry::getInstance()->getDefaultValue(resourceNodePath);

    TYPELOGV(NOTIFY_NODE_RESET, resourceNodePath.c_str(), defVal.c_str());
    std::ofstream controllerFile(resourceNodePath);
    if(!controllerFile.is_open()) {
        TYPELOGV(ERRNO_LOG, "open", strerror(errno));
        return;
    }

    controllerFile<<defVal<<std::endl;

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

    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    if(rConf == nullptr) return;

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
        std::string defVal = ResourceRegistry::getInstance()->getDefaultValue(controllerFilePath);

        TYPELOGV(NOTIFY_NODE_RESET, controllerFilePath.c_str(), defVal.c_str());
        std::ofstream controllerFile(controllerFilePath);
        if(!controllerFile.is_open()) {
            TYPELOGV(ERRNO_LOG, "open", strerror(errno));
            return;
        }

        controllerFile<<defVal<<std::endl;

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

    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    if(rConf == nullptr) return;

    int32_t valueToWrite = resource->getValueAt(0);
    std::string resourceNodePath = rConf->mResourcePath;

    if(resource->getValuesCount() == 2) {
        int32_t id = resource->getValueAt(0);
        valueToWrite = resource->getValueAt(1);
        resourceNodePath = getFullResourceNodePath(rConf, id);
    }

    TYPELOGV(NOTIFY_NODE_WRITE, resourceNodePath.c_str(), valueToWrite);
    AuxRoutines::writeToFile(resourceNodePath, std::to_string(valueToWrite));
}

// Default Tear Callback for Resources with ApplyType = "global"
void defaultGlobalLevelTearCb(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);

    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    if(rConf == nullptr) return;

    std::string resourceNodePath = rConf->mResourcePath;

    if(resource->getValuesCount() == 2) {
        int32_t id = resource->getValueAt(0);
        resourceNodePath = getFullResourceNodePath(rConf, id);
    }

    std::string defVal = ResourceRegistry::getInstance()->getDefaultValue(resourceNodePath);
    TYPELOGV(NOTIFY_NODE_RESET, resourceNodePath.c_str(), defVal.c_str());
    AuxRoutines::writeToFile(resourceNodePath, defVal);
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

    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());

    if(cGroupConfig == nullptr) {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
        return;
    }

    // mCgroupName has the entire name from InitConfig.yaml. Ex- system.slice/camera-cgroup
    const std::string cGroupName = cGroupConfig->mCgroupName;
    if(cGroupName.length() == 0) {
        return;
    }

    std::string filePath = rConf->mResourcePath;

    // Replace %s in above file path with the actual cgroup name
    char pathBuffer[128] = {0};
    std::snprintf(pathBuffer, sizeof(pathBuffer), filePath.c_str(), cGroupName.c_str());

    for(int32_t i = 1; i < resource->getValuesCount(); i++) {
        int32_t pid = resource->getValueAt(i);
        std::string currentCGroupFilePath = "/proc/" + std::to_string(pid) + "/cgroup";
        std::string currentCGroup = AuxRoutines::readFromFile(currentCGroupFilePath);

        if(currentCGroup.length() > 4) {
            currentCGroup = currentCGroup.substr(4);
            ResourceRegistry::getInstance()->addDefaultValue(currentCGroupFilePath, currentCGroup);
        }

        TYPELOGV(NOTIFY_NODE_WRITE, pathBuffer, pid);
        std::ofstream controllerFile(pathBuffer);
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

static void setTaskAffinity(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() < 2) return;

    pid_t pid = resource->getValueAt(0);

    // Get current mask
    cpu_set_t mask;
    if(sched_getaffinity(pid, sizeof(cpu_set_t), &mask) == -1) {
        TYPELOGV(ERRNO_LOG, "sched_getaffinity", strerror(errno));
    } else {
        std::string procDesc = "task_affinity_" + std::to_string(pid);
        std::string maskStr = "";

        // Serialize the mask as a string
        for(int32_t i = 0; i < CPU_SETSIZE; i++) {
            if(CPU_ISSET(i, &mask)) {
                if(maskStr.length() > 0) {
                    maskStr.append(",");
                }
                maskStr.append(std::to_string(i));
            }
        }

        ResourceRegistry::getInstance()->addDefaultValue(procDesc, maskStr);
    }

    CPU_ZERO(&mask);
    for(int32_t i = 1; i < resource->getValuesCount(); i++) {
        int32_t cpuId = resource->getValueAt(i);
        CPU_SET(cpuId, &mask);
    }

    if(sched_setaffinity(pid, sizeof(mask), &mask) == -1) {
        TYPELOGV(ERRNO_LOG, "sched_setaffinity", strerror(errno));
    }
}

static void removeProcessFromCGroup(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() < 2) return;

    for(int32_t i = 1; i < resource->getValuesCount(); i++) {
        int32_t pid = resource->getValueAt(i);

        std::string originalCGroupKey = "/proc/" + std::to_string(pid) + "/cgroup";
        std::string cGroupPath =
            ResourceRegistry::getInstance()->getDefaultValue(originalCGroupKey);

        if(cGroupPath.length() == 0) {
            cGroupPath = UrmSettings::mBaseCGroupPath + "cgroup.procs";
        } else {
            cGroupPath =  UrmSettings::mBaseCGroupPath + cGroupPath + "/cgroup.procs";
        }

        ResourceRegistry::getInstance()->deleteDefaultValue(originalCGroupKey);

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

            std::string defVal = ResourceRegistry::getInstance()->getDefaultValue(cGroupCpuSetFilePath);

            TYPELOGV(NOTIFY_NODE_RESET, cGroupCpuSetFilePath.c_str(), defVal.c_str());
            std::ofstream controllerFile(cGroupCpuSetFilePath);
            if(!controllerFile.is_open()) {
                TYPELOGV(ERRNO_LOG, "open", strerror(errno));
                return;
            }

            controllerFile<<defVal<<std::endl;

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

            defVal = ResourceRegistry::getInstance()->getDefaultValue(cGroupCpusetPartitionFilePath);

            partitionFile<<defVal<<std::endl;

            if(partitionFile.fail()) {
                TYPELOGV(ERRNO_LOG, "write", strerror(errno));
            }
            partitionFile.close();
        }
    } else {
        TYPELOGV(VERIFIER_CGROUP_NOT_FOUND, cGroupIdentifier);
    }
}

static void resetTaskAffinity(void* context) {
    if(context == nullptr) return;
    Resource* resource = static_cast<Resource*>(context);
    if(resource->getValuesCount() < 2) return;

    pid_t pid = resource->getValueAt(0);

    std::string procDesc = "task_affinity_" + std::to_string(pid);
    std::string defVal = ResourceRegistry::getInstance()->getDefaultValue(procDesc);

    if(defVal.length() == 0) {
        return;
    }

    // Reconstruct Original Mask
    cpu_set_t originalMask;
    CPU_ZERO(&originalMask);

    std::string value;
    std::istringstream iss(defVal);
    while(std::getline(iss, value, ',')) {
        try {
            int32_t cpuId = std::stoi(value);
            CPU_SET(cpuId, &originalMask);

        } catch (const std::exception&) {
            return;
        }
    }

    if(sched_setaffinity(pid, sizeof(originalMask), &originalMask) == -1) {
        TYPELOGV(ERRNO_LOG, "sched_setaffinity", strerror(errno));
    }
}

// Register the specific Callbacks
URM_REGISTER_RES_APPLIER_CB(0x00090000, moveProcessToCGroup);
URM_REGISTER_RES_APPLIER_CB(0x00090001, moveThreadToCGroup);
URM_REGISTER_RES_APPLIER_CB(0x00090002, setRunOnCores);
URM_REGISTER_RES_APPLIER_CB(0x00090003, setRunOnCoresExclusively);
URM_REGISTER_RES_APPLIER_CB(0x00090005, limitCpuTime);
URM_REGISTER_RES_APPLIER_CB(0x000c0000, setTaskAffinity);
URM_REGISTER_RES_TEAR_CB(0x00090000, removeProcessFromCGroup);
URM_REGISTER_RES_TEAR_CB(0x00090001, removeThreadFromCGroup);
URM_REGISTER_RES_TEAR_CB(0x00090003, resetRunOnCoresExclusively);
URM_REGISTER_RES_TEAR_CB(0x000c0000, resetTaskAffinity)
