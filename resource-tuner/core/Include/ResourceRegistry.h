// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef RESOURCE_REGISTRY_H
#define RESOURCE_REGISTRY_H

#include <vector>
#include <memory>
#include <unordered_map>

#include "UrmPlatformAL.h"
#include "Utils.h"
#include "TargetRegistry.h"
#include "Resource.h"
#include "UrmSettings.h"
#include "AuxRoutines.h"
#include "Extensions.h"
#include "Logger.h"

/**
 * @struct ResourceApplyType
 * @brief Application type or level for a Resource
 */
enum ResourceApplyType {
    APPLY_CORE, //!< Resource can have different configured values across different cores
    APPLY_CLUSTER, //!< Resource can have different configured values across different clusters
    APPLY_GLOBAL,  //!< Resource is global, i.e. the same configured value across the system.
    APPLY_CGROUP  //!< Resource can have different configured values across different cgroups
};

/**
 * @struct ResConfInfo
 * @brief Representation of a single Resource Configuration
 * @details This information is read from the Config files.\n
 *          Note this (ResConfInfo) struct is separate from the Resource struct.
 */
typedef struct {
    /**
     * @brief Name of the Resource (Placeholder).
     */
    std::string mResourceName;
    /**
     * @brief Path to the Sysfs node, CGroup controller file or as applicable.
     */
    std::string mResourcePath;
    /**
     * @brief Type of the Resource, for example: LPM, CPU_DCVS, GPU etc.
     */
    uint8_t mResourceResType;
    /**
     * @brief Unique Resource ID associated with the resource.
     */
    uint16_t mResourceResID;
    /**
     * @brief Max Possible Value which can be configured for this Resource.
     */
    int32_t mHighThreshold;
    /**
     * @brief Min Possible Value which can be configured for this Resource.
     */
    int32_t mLowThreshold;
    /**
     * @brief The Permission Level needed by a client in order to tune this Resource
     */
    enum Permissions mPermissions;
    /**
     * @brief Specify the mode (ex: Display on, display off, doze) under which the Resource can be provisioned
     */
    uint8_t mModes;
    /**
     * @brief Application Type Enum, indicating whether the specified value for the Resource
     *        by a Request, needs to be applied at a per-core, per-cluster or global value.
     */
    enum ResourceApplyType mApplyType;
    /**
     * @brief Policy by which the resource is governed, for example Higher is Better.
     */
    enum Policy mPolicy;
    /**
     * @brief Optional Custom Resource Applier Callback, it needs to be supplied by
     *        the BU via the Extension Interface.
     */
    ResourceLifecycleCallback mResourceApplierCallback;
    /**
     * @brief Translation Unit for the Resource, for example KB, MB, MHz etc.
     */
    enum TranslationUnit mUnit;
    /**
     * @brief Optional Custom Resource Tear Callback, it needs to be supplied by
     *        the BU via the Extension Interface.
     */
    ResourceLifecycleCallback mResourceTearCallback;
} ResConfInfo;

/**
 * @brief ResourceRegistry
 * @details Stores information Relating to all the Resources available for Tuning.
 *          Note: This information is extracted from Config YAML files.
 */
class __attribute__((visibility("default"))) ResourceRegistry {
private:
    static std::shared_ptr<ResourceRegistry> resourceRegistryInstance;
    int32_t mTotalResources;

    std::vector<ResConfInfo*> mResourceConfigs;
    std::unordered_map<uint32_t, int32_t> mSILMap;
    std::unordered_map<std::string, std::string> mDefaultValueStore;

    ResourceRegistry();

    int8_t isResourceConfigMalformed(ResConfInfo* resourceConfigInfo);
    void setLifeCycleCallbacks(ResConfInfo* resourceConfigInfo);
    void fetchAndStoreDefaults(ResConfInfo* resourceConfigInfo);

public:
    ~ResourceRegistry();

    /**
     * @brief Used to register a Config specified (through YAML) Resource with Resource Tuner
     * @details The Resource Info is parsed from YAML files. If the ResourceConfig provided is
     *          Malformed, then it will be freed as part of this routine, else it will
     *          be added to the "mResourceConfigs" vector.
     */
    void registerResource(ResConfInfo* resourceConfigInfo);

    std::vector<ResConfInfo*> getRegisteredResources();

    /**
     * @brief Get the ResConfInfo object corresponding to the given Resource ID.
     * @param resourceId An unsigned 32 bit integer, representing the Resource ID.
     * @return ResConfInfo*:\n
     *          - A pointer to the ResConfInfo object
     *          - nullptr, if no ResConfInfo object with the given Resource ID exists.
     */
    ResConfInfo* getResConf(uint32_t resourceId);

    int32_t getResourceTableIndex(uint32_t resourceId);
    int32_t getTotalResourcesCount();
    std::string getDefaultValue(const std::string& fileName);

    void addDefaultValue(const std::string& key, const std::string& value);
    void deleteDefaultValue(const std::string& filePath);
    void restoreResourcesToDefaultValues();
    void displayResources();

    // Merge the Changes provided by the BU with the existing ResourceTable.
    void pluginModifications();

    static std::shared_ptr<ResourceRegistry> getInstance() {
        if(resourceRegistryInstance == nullptr) {
            resourceRegistryInstance = std::shared_ptr<ResourceRegistry>(new ResourceRegistry());
        }
        return resourceRegistryInstance;
    }
};

class ResourceConfigInfoBuilder {
private:
    ResConfInfo* mResourceConfigInfo;

public:
    int32_t mTargetRefCount;

    ResourceConfigInfoBuilder();
    ~ResourceConfigInfoBuilder();

    ErrCode setName(const std::string& resourceName);
    ErrCode setPath(const std::string& resourcePath);
    ErrCode setResType(const std::string& resTypeString);
    ErrCode setResID(const std::string& resIDString);
    ErrCode setHighThreshold(const std::string& highThreshold);
    ErrCode setLowThreshold(const std::string& lowThreshold);
    ErrCode setPermissions(const std::string& permissionString);
    ErrCode setModes(const std::string& modeString);
    ErrCode setSupported(const std::string& supported);
    ErrCode setPolicy(const std::string& policyString);
    ErrCode setTranslationUnit(const std::string& unitString);
    ErrCode setApplyType(const std::string& applyTypeString);
    ErrCode addTargetEnabled(const std::string& target);
    ErrCode addTargetDisabled(const std::string& target);

    ResConfInfo* build();
};

void defaultClusterLevelApplierCb(void* context);
void defaultClusterLevelTearCb(void* context);
void defaultCoreLevelApplierCb(void* context);
void defaultCoreLevelTearCb(void* context);
void defaultCGroupLevelApplierCb(void* context);
void defaultCGroupLevelTearCb(void* context);
void defaultGlobalLevelApplierCb(void* context);
void defaultGlobalLevelTearCb(void* context);

#endif
