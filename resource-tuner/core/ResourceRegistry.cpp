// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ResourceRegistry.h"
#include "AuxRoutines.h"
#include "UrmSettings.h"
#include "TargetRegistry.h"

static const int32_t unsupportedResoure = -2;

std::shared_ptr<ResourceRegistry> ResourceRegistry::resourceRegistryInstance = nullptr;

ResourceRegistry::ResourceRegistry() {
    this->mTotalResources = 0;
}

int8_t ResourceRegistry::isResourceConfigMalformed(ResConfInfo* rConf) {
    if(rConf == nullptr) return true;
    if(rConf->mResourceResType == 0) return true;
    return false;
}

void ResourceRegistry::setLifeCycleCallbacks(ResConfInfo* resourceConfigInfo) {
    switch(resourceConfigInfo->mApplyType) {
        case APPLY_CLUSTER:
            resourceConfigInfo->mResourceApplierCallback = defaultClusterLevelApplierCb;
            resourceConfigInfo->mResourceTearCallback = defaultClusterLevelTearCb;
            break;
        case APPLY_CORE:
            resourceConfigInfo->mResourceApplierCallback = defaultCoreLevelApplierCb;
            resourceConfigInfo->mResourceTearCallback = defaultCoreLevelTearCb;
            break;
        case APPLY_CGROUP:
            resourceConfigInfo->mResourceApplierCallback = defaultCGroupLevelApplierCb;
            resourceConfigInfo->mResourceTearCallback = defaultCGroupLevelTearCb;
            break;
        case APPLY_GLOBAL:
            resourceConfigInfo->mResourceApplierCallback = defaultGlobalLevelApplierCb;
            resourceConfigInfo->mResourceTearCallback = defaultGlobalLevelTearCb;
            break;
    }
}

void ResourceRegistry::addDefaultValue(const std::string& filePath, const std::string& value) {
    this->mDefaultValueStore[filePath] = value;

    // std::fstream persistenceFile(UrmSettings::mPersistenceFile, std::ios::out | std::ios::app);
    // std::string resourceData = filePath;
    // resourceData.push_back(',');
    // resourceData.append(value);
    // resourceData.push_back('\n');
    // persistenceFile << resourceData;
}

void ResourceRegistry::fetchAndStoreDefaults(ResConfInfo* resourceConfigInfo) {
    if(resourceConfigInfo == nullptr) return;
    switch(resourceConfigInfo->mApplyType) {
        case APPLY_CLUSTER: {
            std::vector<int32_t> clusterIDs;
            TargetRegistry::getInstance()->getClusterIDs(clusterIDs);
            for(int32_t clusterID : clusterIDs) {
                char filePath[128];
                snprintf(filePath, sizeof(filePath), resourceConfigInfo->mResourcePath.c_str(), (int32_t)clusterID);
                this->addDefaultValue(std::string(filePath), AuxRoutines::readFromFile(filePath));
            }
            break;
        }
        case APPLY_CGROUP: {
            std::vector<std::string> cGroupNames;
            TargetRegistry::getInstance()->getCGroupNames(cGroupNames);
            for(std::string cGroupName : cGroupNames) {
                char filePath[128];
                snprintf(filePath, sizeof(filePath), resourceConfigInfo->mResourcePath.c_str(), cGroupName.c_str());
                this->addDefaultValue(std::string(filePath), AuxRoutines::readFromFile(filePath));
            }
            break;
        }
        case APPLY_CORE: {
            int32_t count = UrmSettings::targetConfigs.mTotalCoreCount;
            for(int32_t coreID = 0; coreID < count; coreID++) {
                char filePath[128];
                snprintf(filePath, sizeof(filePath), resourceConfigInfo->mResourcePath.c_str(), (int32_t)coreID);
                this->addDefaultValue(std::string(filePath), AuxRoutines::readFromFile(filePath));
            }
            break;
        }
        case APPLY_GLOBAL: {
            this->addDefaultValue(resourceConfigInfo->mResourcePath,
                                  AuxRoutines::readFromFile(resourceConfigInfo->mResourcePath));
            break;
        }
    }
}

void ResourceRegistry::registerResource(ResConfInfo* resourceConfigInfo) {
    // Invalid Resource, skip.
    if(this->isResourceConfigMalformed(resourceConfigInfo)) {
        if(resourceConfigInfo != nullptr) {
            delete resourceConfigInfo;
            resourceConfigInfo = nullptr;
        }
        return;
    }

    // Create the OpID Bitmap, this will serve as the key for the entry in mSILMap.
    uint32_t resourceBitmap = 0;
    resourceBitmap |= ((uint32_t)resourceConfigInfo->mResourceResID);
    resourceBitmap |= ((uint32_t)resourceConfigInfo->mResourceResType << 16);

    // Check for any conflict
    if(this->mSILMap.find(resourceBitmap) != this->mSILMap.end()) {
        // Resource with the specified ResType and ResCode already exists
        // Overwrite it.
        int32_t resourceTableIndex = getResourceTableIndex(resourceBitmap);
        this->mResourceConfigs[resourceTableIndex] = resourceConfigInfo;

        this->mSILMap.erase(resourceBitmap);
        this->mSILMap[resourceBitmap] = resourceTableIndex;

    } else {
        this->mSILMap[resourceBitmap] = this->mTotalResources;
        this->mResourceConfigs.push_back(resourceConfigInfo);

        this->mTotalResources++;
    }

    this->setLifeCycleCallbacks(resourceConfigInfo);
    this->fetchAndStoreDefaults(resourceConfigInfo);
}

void ResourceRegistry::displayResources() {
    for(int32_t i = 0; i < this->mTotalResources; i++) {
        auto& res = mResourceConfigs[i];

        LOGI("RESTUNE_RESOURCE_PROCESSOR", "Resource Name: " + res->mResourceName);
        LOGI("RESTUNE_RESOURCE_PROCESSOR", "Resource Path: " + res->mResourcePath);
        LOGI("RESTUNE_RESOURCE_PROCESSOR", "ResType: " + std::to_string(res->mResourceResType));
        LOGI("RESTUNE_RESOURCE_PROCESSOR", "ResID: " + std::to_string(res->mResourceResID));
        LOGI("RESTUNE_RESOURCE_PROCESSOR", "High Threshold: " + std::to_string(res->mHighThreshold));
        LOGI("RESTUNE_RESOURCE_PROCESSOR", "Low Threshold: " + std::to_string(res->mLowThreshold));

        LOGI("RESTUNE_RESOURCE_PROCESSOR", "====================================");
    }
}

std::vector<ResConfInfo*> ResourceRegistry::getRegisteredResources() {
    return mResourceConfigs;
}

ResConfInfo* ResourceRegistry::getResConf(uint32_t resourceId) {
    if(this->mSILMap.find(resourceId) == this->mSILMap.end()) {
        TYPELOGV(RESOURCE_REGISTRY_RESOURCE_NOT_FOUND, resourceId);
        return nullptr;
    }

    int32_t resourceTableIndex = this->mSILMap[resourceId];
    if(resourceTableIndex == -1) {
        TYPELOGV(RESOURCE_REGISTRY_RESOURCE_NOT_FOUND, resourceId);
        return nullptr;
    }
    return this->mResourceConfigs[resourceTableIndex];
}

int32_t ResourceRegistry::getResourceTableIndex(uint32_t resourceId) {
    if(this->mSILMap.find(resourceId) == this->mSILMap.end()) {
        return -1;
    }

    return this->mSILMap[resourceId];
}

int32_t ResourceRegistry::getTotalResourcesCount() {
    return this->mTotalResources;
}

std::string ResourceRegistry::getDefaultValue(const std::string& filePath) {
    return this->mDefaultValueStore[filePath];
}

void ResourceRegistry::deleteDefaultValue(const std::string& filePath) {
    this->mDefaultValueStore.erase(filePath);
}

void ResourceRegistry::pluginModifications() {
    std::vector<std::pair<uint32_t, ResourceLifecycleCallback>> applierCallbacks = Extensions::getResourceApplierCallbacks();
    std::vector<std::pair<uint32_t, ResourceLifecycleCallback>> tearCallbacks = Extensions::getResourceTearCallbacks();

    for(std::pair<uint32_t, ResourceLifecycleCallback> resource: applierCallbacks) {
        int32_t resourceTableIndex = this->getResourceTableIndex(resource.first);
        if(resourceTableIndex != -1) {
            this->mResourceConfigs[resourceTableIndex]->mResourceApplierCallback = resource.second;
        }
    }

    for(std::pair<uint32_t, ResourceLifecycleCallback> resource: tearCallbacks) {
        int32_t resourceTableIndex = this->getResourceTableIndex(resource.first);
        if(resourceTableIndex != -1) {
            this->mResourceConfigs[resourceTableIndex]->mResourceTearCallback = resource.second;
        }
    }
}

void ResourceRegistry::restoreResourcesToDefaultValues() {
    for(std::pair<std::string, std::string> defaultConfig: this->mDefaultValueStore) {
        std::string filePath = defaultConfig.first;
        std::string value = defaultConfig.second;

        AuxRoutines::writeToFile(filePath, value);
    }
}

ResourceRegistry::~ResourceRegistry() {
    for(size_t i = 0; i < this->mResourceConfigs.size(); i++) {
        if(this->mResourceConfigs[i] != nullptr) {
            delete this->mResourceConfigs[i];
            this->mResourceConfigs[i] = nullptr;
        }
    }
}

ResourceConfigInfoBuilder::ResourceConfigInfoBuilder() {
    this->mTargetRefCount = 0;
    this->mResourceConfigInfo = new (std::nothrow) ResConfInfo;
    if(this->mResourceConfigInfo == nullptr) {
        return;
    }

    // Initialize Default Values
    this->mResourceConfigInfo->mResourceResType = 0;
    this->mResourceConfigInfo->mResourceResID = 0;
    this->mResourceConfigInfo->mResourceApplierCallback = nullptr;
    this->mResourceConfigInfo->mResourceTearCallback = nullptr;
    this->mResourceConfigInfo->mModes = 0;
    this->mResourceConfigInfo->mHighThreshold = this->mResourceConfigInfo->mLowThreshold = -1;
    this->mResourceConfigInfo->mPermissions = PERMISSION_THIRD_PARTY;
    this->mResourceConfigInfo->mApplyType = ResourceApplyType::APPLY_GLOBAL;
    this->mResourceConfigInfo->mPolicy = Policy::LAZY_APPLY;
    this->mResourceConfigInfo->mUnit = TranslationUnit::U_NA;
    this->mResourceConfigInfo->mResourcePath = "";
    this->mResourceConfigInfo->mResourceName = "";
}

ErrCode ResourceConfigInfoBuilder::setName(const std::string& name) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    this->mResourceConfigInfo->mResourceName = name;
    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setPath(const std::string& path) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    this->mResourceConfigInfo->mResourcePath = path;

    std::string curToken;
    std::vector<std::string> tokens;
    std::istringstream resPathStream(path);
    while(std::getline(resPathStream, curToken, '*')) {
        tokens.push_back(curToken);
    }

    if(tokens.size() < 2) {
        return RC_SUCCESS;
    }

    // Further split to get the pattern to match to:
    std::string keyMatch = "";
    std::string remFilePath = "";
    int8_t foundSplit = false;
    for(char ch: tokens[1]) {
        if(ch == '/') {
            if(foundSplit) {
                remFilePath.push_back(ch);
            }
            foundSplit = true;
        } else {
            if(foundSplit) {
                remFilePath.push_back(ch);
            } else {
                keyMatch.push_back(ch);
            }
        }
    }

    DIR* dir = opendir(tokens[0].c_str());
    if(dir != nullptr) {
        struct dirent* entry;
        while((entry = readdir(dir)) != nullptr) {
            std::string dirName = std::string(entry->d_name);
            if(keyMatch.size() < dirName.size()) {
                // calculate starting index
                int32_t start = dirName.size() - keyMatch.size();
                if(start >= 0) {
                    if(dirName.compare(start, keyMatch.size(), keyMatch) == 0) {
                        // Generate the resulting path:
                        std::string filePath =
                            tokens[0] + "/" + dirName + "/" + remFilePath;
                        if(AuxRoutines::fileExists(filePath)) {
                            this->mResourceConfigInfo->mResourcePath = filePath;
                            break;
                        }
                    }
                }
            }
        }
        closedir(dir);
    }

    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setResType(const std::string& resTypeString) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    this->mResourceConfigInfo->mResourceResType = -1;
    try {
        this->mResourceConfigInfo->mResourceResType = (uint8_t)stoi(resTypeString, nullptr, 0);
    } catch(const std::invalid_argument& e) {
        TYPELOGV(RESOURCE_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;

    } catch(const std::out_of_range& e) {
        TYPELOGV(RESOURCE_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setResID(const std::string& resIDString) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    this->mResourceConfigInfo->mResourceResID = 0;
    try {
        this->mResourceConfigInfo->mResourceResID = (uint16_t)stoi(resIDString, nullptr, 0);
    } catch(const std::invalid_argument& e) {
        TYPELOGV(RESOURCE_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;

    } catch(const std::out_of_range& e) {
        TYPELOGV(RESOURCE_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setHighThreshold(const std::string& highThresholdString) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    this->mResourceConfigInfo->mHighThreshold = 0;
    try {
        this->mResourceConfigInfo->mHighThreshold = (int32_t)stoi(highThresholdString, nullptr, 0);
    } catch(const std::invalid_argument& e) {
        TYPELOGV(RESOURCE_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;

    } catch(const std::out_of_range& e) {
        TYPELOGV(RESOURCE_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setLowThreshold(const std::string& lowThresholdString) {
        if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    this->mResourceConfigInfo->mLowThreshold = 0;
    try {
        this->mResourceConfigInfo->mLowThreshold = (int32_t)stoi(lowThresholdString, nullptr, 0);
    } catch(const std::invalid_argument& e) {
        TYPELOGV(RESOURCE_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;

    } catch(const std::out_of_range& e) {
        TYPELOGV(RESOURCE_REGISTRY_PARSING_FAILURE, e.what());
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setPermissions(const std::string& permissionString) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    enum Permissions permissions = PERMISSION_THIRD_PARTY;
    if(permissionString == "system") {
        permissions = PERMISSION_SYSTEM;
    } else if(permissionString == "third_party") {
        permissions = PERMISSION_THIRD_PARTY;
    } else {
        if(permissionString.length() != 0) {
            return RC_INVALID_VALUE;
        }
    }

    this->mResourceConfigInfo->mPermissions = permissions;
    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setModes(const std::string& modeString) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    if(modeString == "display_on") {
        this->mResourceConfigInfo->mModes = this->mResourceConfigInfo->mModes | MODE_RESUME;
    } else if(modeString == "display_off") {
        this->mResourceConfigInfo->mModes = this->mResourceConfigInfo->mModes | MODE_SUSPEND;
    } else if(modeString == "doze") {
        this->mResourceConfigInfo->mModes = this->mResourceConfigInfo->mModes | MODE_DOZE;
    } else {
        if(modeString.length() != 0) {
            return RC_INVALID_VALUE;
        }
    }
    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setSupported(const std::string& supportedString) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    if(supportedString != "true") {
        this->mTargetRefCount = unsupportedResoure;
    }
    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setPolicy(const std::string& policyString) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    enum Policy policy = LAZY_APPLY;
    if(policyString == "higher_is_better") {
        policy = HIGHER_BETTER;
    } else if(policyString == "lower_is_better") {
        policy = LOWER_BETTER;
    } else if(policyString == "lazy_apply") {
        policy = LAZY_APPLY;
    } else if(policyString == "instant_apply") {
        policy = INSTANT_APPLY;
    } else if(policyString == "pass_through") {
        policy = PASS_THROUGH;
    } else {
        if(policyString.length() != 0) {
            return RC_INVALID_VALUE;
        }
    }

    this->mResourceConfigInfo->mPolicy = policy;
    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setTranslationUnit(const std::string& unitString) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    enum TranslationUnit unit = U_NA;
    if(unitString == "KB" || unitString == "kb") {
        unit = U_KB;
    } else if(unitString == "MB" || unitString == "mb") {
        unit = U_MB;
    } else if(unitString == "GB" || unitString == "gb") {
        unit = U_GB;
    } else if(unitString == "KHz" || unitString == "khz") {
        unit = U_KHz;
    } else if(unitString == "MHz" || unitString == "mhz") {
        unit = U_MHz;
    } else if(unitString == "GHz" || unitString == "ghz") {
        unit = U_GHz;
    } else if(unitString == "Hz" || unitString == "hz") {
        unit = U_Hz;
    } else if(unitString == "byte") {
        unit = U_BYTE;
    } else if(unitString == "NA" || unitString == "na") {
        unit = U_NA;
    } else {
        if(unitString.length() != 0) {
            return RC_INVALID_VALUE;
        }
    }

    this->mResourceConfigInfo->mUnit = unit;
    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::setApplyType(const std::string& applyTypeString) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_INVALID_VALUE;
    }

    enum ResourceApplyType applyType = APPLY_GLOBAL;
    if(applyTypeString == "global") {
        applyType = APPLY_GLOBAL;
    } else if(applyTypeString == "core") {
        applyType = APPLY_CORE;
    } else if(applyTypeString == "cluster") {
        applyType = APPLY_CLUSTER;
    } else if(applyTypeString == "cgroup") {
        applyType = APPLY_CGROUP;
    } else {
        if(applyTypeString.length() != 0) {
            return RC_INVALID_VALUE;
        }
    }

    this->mResourceConfigInfo->mApplyType = applyType;
    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::addTargetEnabled(const std::string& target) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    if(this->mTargetRefCount == unsupportedResoure) {
        return RC_RESOURCE_NOT_SUPPORTED;
    }

    // first entry
    if(this->mTargetRefCount == 0) {
        this->mTargetRefCount = -1;
    }

    if(target == UrmSettings::targetConfigs.targetName) {
        this->mTargetRefCount = 1;
    }
    return RC_SUCCESS;
}

ErrCode ResourceConfigInfoBuilder::addTargetDisabled(const std::string& target) {
    if(this->mResourceConfigInfo == nullptr) {
        return RC_MEMORY_ALLOCATION_FAILURE;
    }

    if(this->mTargetRefCount == unsupportedResoure) {
        return RC_RESOURCE_NOT_SUPPORTED;
    }

    // first entry
    if(this->mTargetRefCount == 0) {
        this->mTargetRefCount = 1;
    }

    if(target == UrmSettings::targetConfigs.targetName) {
        this->mTargetRefCount = -1;
    }
    return RC_SUCCESS;
}

ResConfInfo* ResourceConfigInfoBuilder::build() {
    return this->mResourceConfigInfo;
}

ResourceConfigInfoBuilder::~ResourceConfigInfoBuilder() {}
