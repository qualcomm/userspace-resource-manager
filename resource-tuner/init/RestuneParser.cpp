// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "RestuneParser.h"

#define ADD_TO_RESOURCE_BUILDER(KEY, METHOD)                    \
    if(topKey == KEY && resourceConfigInfoBuilder != nullptr) { \
        if(RC_IS_OK(rc)) {                                      \
            rc = resourceConfigInfoBuilder->METHOD(value);      \
            if(RC_IS_NOTOK(rc)) {                               \
                TYPELOGV(INV_ATTR_VAL, KEY, value.c_str());     \
            }                                                   \
        }                                                       \
        break;                                                  \
    }

#define ADD_TO_CGROUP_BUILDER(KEY, METHOD)                      \
    if(topKey == KEY && cGroupConfigBuilder != nullptr) {       \
        if(RC_IS_OK(rc)) {                                      \
            rc = cGroupConfigBuilder->METHOD(value);            \
        }                                                       \
        break;                                                  \
    }

#define ADD_TO_MPAM_GROUP_BUILDER(KEY, METHOD)                  \
    if(topKey == KEY && mpamGroupConfigBuilder != nullptr) {    \
        if(RC_IS_OK(rc)) {                                      \
            rc = mpamGroupConfigBuilder->METHOD(value);         \
        }                                                       \
        break;                                                  \
    }

#define ADD_TO_CACHE_INFO_BUILDER(KEY, METHOD)                  \
    if(topKey == KEY && cacheInfoBuilder != nullptr) {          \
        if(RC_IS_OK(rc)) {                                      \
            rc = cacheInfoBuilder->METHOD(value);               \
        }                                                       \
        break;                                                  \
    }


#define ADD_TO_SIGNAL_BUILDER(KEY, METHOD)                      \
    if(topKey == KEY && signalInfoBuilder != nullptr) {         \
        if(RC_IS_OK(rc)) {                                      \
            rc = signalInfoBuilder->METHOD(value);              \
        }                                                       \
        break;                                                  \
    }

#define ADD_RES_TO_SIGNAL_BUILDER(KEY, METHOD)                  \
    if(topKey == KEY && resourceBuilder != nullptr) {           \
        if(RC_IS_OK(rc)) {                                      \
            rc = resourceBuilder->METHOD(value);                \
        }                                                       \
        break;                                                  \
    }

#define ADD_TO_EXT_FEATURE_BUILDER(KEY, METHOD)                 \
    if(topKey == KEY && extFeatureInfoBuilder != nullptr) {     \
        if(RC_IS_OK(rc)) {                                      \
            rc = extFeatureInfoBuilder->METHOD(value);          \
        }                                                       \
        break;                                                  \
    }

static int8_t isKey(const std::string& keyName) {
    std::vector<std::string> keys = {
        TARGET_CONFIGS_ROOT,
        TARGET_NAME_LIST,
        TARGET_CLUSTER_INFO,
        TARGET_CLUSTER_INFO_LOGICAL_ID,
        TARGET_CLUSTER_INFO_PHYSICAL_ID,
        TARGET_CLUSTER_SPREAD,
        TARGET_PER_CLUSTER_CORE_COUNT,
        RESOURCE_CONFIGS_ROOT,
        RESOURCE_CONFIGS_ELEM_RESOURCE_TYPE,
        RESOURCE_CONFIGS_ELEM_RESOURCE_ID,
        RESOURCE_CONFIGS_ELEM_RESOURCENAME,
        RESOURCE_CONFIGS_ELEM_RESOURCEPATH,
        RESOURCE_CONFIGS_ELEM_SUPPORTED,
        RESOURCE_CONFIGS_ELEM_HIGHTHRESHOLD,
        RESOURCE_CONFIGS_ELEM_LOWTHRESHOLD,
        RESOURCE_CONFIGS_ELEM_PERMISSIONS,
        RESOURCE_CONFIGS_ELEM_MODES,
        RESOURCE_CONFIGS_ELEM_POLICY,
        RESOURCE_CONFIGS_ELEM_T_UNIT,
        RESOURCE_CONFIGS_ELEM_APPLY_TYPE,
        RESOURCE_CONFIGS_ELEM_TARGETS_ENABLED,
        RESOURCE_CONFIGS_ELEM_TARGETS_DISABLED,
        INIT_CONFIGS_ROOT,
        INIT_CONFIGS_ELEM_CGROUPS_LIST,
        INIT_CONFIGS_ELEM_CGROUP_NAME,
        INIT_CONFIGS_ELEM_CGROUP_IDENTIFIER,
        INIT_CONFIGS_ELEM_CGROUP_CREATION,
        INIT_CONFIGS_ELEM_CGROUP_THREADED,
        INIT_CONFIGS_ELEM_CLUSTER_MAP,
        INIT_CONFIGS_ELEM_CLUSTER_MAP_CLUSTER_ID,
        INIT_CONFIGS_ELEM_CLUSTER_MAP_CLUSTER_TYPE,
        INIT_CONFIGS_ELEM_MPAM_GROUPS_LIST,
        INIT_CONFIGS_ELEM_MPAM_GROUP_NAME,
        INIT_CONFIGS_ELEM_MPAM_GROUP_ID,
        INIT_CONFIGS_ELEM_MPAM_GROUP_PRIORITY,
        INIT_CONFIGS_ELEM_CACHE_INFO_LIST,
        INIT_CONFIGS_ELEM_CACHE_INFO_TYPE,
        INIT_CONFIGS_ELEM_CACHE_INFO_BLK_CNT,
        INIT_CONFIGS_ELEM_CACHE_INFO_PRIO_AWARE,
        SIGNAL_CONFIGS_ROOT,
        SIGNAL_CONFIGS_ELEM_SIGID,
        SIGNAL_CONFIGS_ELEM_CATEGORY,
        SIGNAL_CONFIGS_ELEM_NAME,
        SIGNAL_CONFIGS_ELEM_ENABLE,
        SIGNAL_CONFIGS_ELEM_TIMEOUT,
        SIGNAL_CONFIGS_ELEM_TARGETS_ENABLED,
        SIGNAL_CONFIGS_ELEM_PERMISSIONS,
        SIGNAL_CONFIGS_ELEM_TARGETS_DISABLED,
        SIGNAL_CONFIGS_ELEM_RESOURCES,
        SIGNAL_CONFIGS_ELEM_DERIVATIVES,
        SIGNAL_CONFIGS_ELEM_RESOURCE_CODE,
        SIGNAL_CONFIGS_ELEM_RESOURCE_RESINFO,
        SIGNAL_CONFIGS_ELEM_RESOURCE_VALUES,
        EXT_FEATURE_CONFIGS_ROOT,
        EXT_FEATURE_CONFIGS_ELEM_ID,
        EXT_FEATURE_CONFIGS_ELEM_LIB,
        EXT_FEATURE_CONFIGS_ELEM_NAME,
        EXT_FEATURE_CONFIGS_ELEM_DESCRIPTION,
        EXT_FEATURE_CONFIGS_ELEM_SUBSCRIBER_LIST,
        APP_CONFIGS_ROOT,
        APP_CONFIGS_APP_NAME,
        APP_CONFIGS_THREAD_LIST,
        APP_CONFIGS_CONFIGURATION_LIST,
    };

    for(const std::string& key: keys) {
        if(key == keyName) return true;
    }

    return false;
}

static int8_t isKeyTypeList(const std::string& keyName) {
    if(keyName == TARGET_NAME_LIST) return true;
    if(keyName == TARGET_CLUSTER_INFO) return true;
    if(keyName == TARGET_CLUSTER_SPREAD) return true;

    if(keyName == INIT_CONFIGS_ELEM_CLUSTER_MAP) return true;
    if(keyName == INIT_CONFIGS_ELEM_CGROUPS_LIST) return true;
    if(keyName == INIT_CONFIGS_ELEM_MPAM_GROUPS_LIST) return true;
    if(keyName == INIT_CONFIGS_ELEM_CACHE_INFO_LIST) return true;

    if(keyName == RESOURCE_CONFIGS_ELEM_MODES) return true;
    if(keyName == RESOURCE_CONFIGS_ELEM_TARGETS_ENABLED) return true;
    if(keyName == RESOURCE_CONFIGS_ELEM_TARGETS_DISABLED) return true;

    if(keyName == SIGNAL_CONFIGS_ELEM_TARGETS_ENABLED) return true;
    if(keyName == SIGNAL_CONFIGS_ELEM_TARGETS_DISABLED) return true;
    if(keyName == SIGNAL_CONFIGS_ELEM_PERMISSIONS) return true;
    if(keyName == SIGNAL_CONFIGS_ELEM_DERIVATIVES) return true;
    if(keyName == SIGNAL_CONFIGS_ELEM_RESOURCES) return true;
    if(keyName == SIGNAL_CONFIGS_ELEM_RESOURCE_VALUES) return true;

    if(keyName == EXT_FEATURE_CONFIGS_ELEM_SUBSCRIBER_LIST) return true;

    return false;
}

ErrCode RestuneParser::parseResourceConfigYamlNode(const std::string& filePath, int8_t isBuSpecified) {
    SETUP_LIBYAML_PARSING(filePath);

    ErrCode rc = RC_SUCCESS;

    int8_t parsingDone = false;
    int8_t docMarker = false;
    int8_t parsingResource = false;

    std::string value;
    std::string topKey;
    std::stack<std::string> keyTracker;

    ResourceConfigInfoBuilder* resourceConfigInfoBuilder = nullptr;

    while(!parsingDone) {
        if(!yaml_parser_parse(&parser, &event)) {
            return RC_YAML_PARSING_ERROR;
        }

        switch(event.type) {
            case YAML_STREAM_END_EVENT:
                parsingDone = true;
                break;

            case YAML_MAPPING_START_EVENT:
                if(!docMarker) {
                    docMarker = true;
                } else {
                    // Individual Resource Config
                    resourceConfigInfoBuilder = new(std::nothrow) ResourceConfigInfoBuilder();
                    if(resourceConfigInfoBuilder == nullptr) {
                        return RC_YAML_PARSING_ERROR;
                    }
                    parsingResource = true;
                }
                break;

            case YAML_MAPPING_END_EVENT:
                if(parsingResource) {
                    if(RC_IS_NOTOK(rc) || resourceConfigInfoBuilder->mTargetRefCount < 0) {
                        // Invalid Resource
                        resourceConfigInfoBuilder->setResType("0");
                    }

                    ResourceRegistry::getInstance()->
                        registerResource(resourceConfigInfoBuilder->build(), isBuSpecified);

                    delete resourceConfigInfoBuilder;
                    resourceConfigInfoBuilder = nullptr;
                    parsingResource = false;
                }

                break;

            case YAML_SEQUENCE_END_EVENT:
                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                keyTracker.pop();
                break;

            case YAML_SCALAR_EVENT:
                if(event.data.scalar.value != nullptr) {
                    value = reinterpret_cast<char*>(event.data.scalar.value);
                }

                if(isKey(value)) {
                    keyTracker.push(value);
                    break;
                }

                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                topKey = keyTracker.top();
                if(!isKeyTypeList(topKey)) {
                    keyTracker.pop();
                }

                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_RESOURCE_TYPE, setResType);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_RESOURCE_ID, setResID);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_RESOURCENAME, setName);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_RESOURCEPATH, setPath);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_SUPPORTED, setSupported);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_HIGHTHRESHOLD, setHighThreshold);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_LOWTHRESHOLD, setLowThreshold);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_PERMISSIONS, setPermissions);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_POLICY, setPolicy);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_T_UNIT, setTranslationUnit);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_APPLY_TYPE, setApplyType);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_MODES, setModes);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_TARGETS_ENABLED, addTargetEnabled);
                ADD_TO_RESOURCE_BUILDER(RESOURCE_CONFIGS_ELEM_TARGETS_DISABLED, addTargetDisabled);

                break;

            default:
                break;
        }

        yaml_event_delete(&event);
    }

    TEARDOWN_LIBYAML_PARSING
    return rc;
}

ErrCode RestuneParser::parsePropertiesConfigYamlNode(const std::string& filePath) {
    SETUP_LIBYAML_PARSING(filePath);

    int8_t parsingDone = false;
    int8_t isPropName = false;

    std::string currentKey = "";
    std::string currentValue = "";
    std::string value;

    while(!parsingDone) {
        if(!yaml_parser_parse(&parser, &event)) {
            return RC_YAML_PARSING_ERROR;
        }

        switch(event.type) {
            case YAML_STREAM_END_EVENT:
                parsingDone = true;
                break;

            case YAML_MAPPING_END_EVENT:
                if(currentKey.length() > 0 && currentValue.length() > 0) {
                    PropertiesRegistry::getInstance()->createProperty(currentKey, currentValue);
                }

                currentKey.clear();
                currentValue.clear();
                break;

            case YAML_SCALAR_EVENT:
                if(event.data.scalar.value != nullptr) {
                    value = reinterpret_cast<char*>(event.data.scalar.value);
                }

                if(value == PROPERTY_CONFIGS_ROOT) {
                    break;
                } else if(value == PROPERTY_CONFIGS_ELEM_NAME) {
                    isPropName = true;
                    break;
                } else if(value == PROPERTY_CONFIGS_ELEM_VALUE) {
                    isPropName = false;
                    break;
                }

                if(isPropName) {
                    currentKey = value;
                } else {
                    currentValue = value;
                }
                break;

            default:
                break;
        }

        yaml_event_delete(&event);
    }

    TEARDOWN_LIBYAML_PARSING
    return RC_SUCCESS;
}

ErrCode RestuneParser::parseTargetConfigYamlNode(const std::string& filePath) {
    SETUP_LIBYAML_PARSING(filePath);

    int8_t parsingDone = false;
    int8_t docMarker = false;
    int8_t parsingItem = false;
    int8_t isConfigForCurrentTarget = false;
    int8_t deviceParsingDone = false;

    std::string value;
    std::string topKey;
    std::stack<std::string> keyTracker;

    std::string lgcClusterID;
    std::string phyClusterID;
    std::string numCoresString;

    while(!parsingDone) {
        if(!yaml_parser_parse(&parser, &event)) {
            return RC_YAML_PARSING_ERROR;
        }

        switch(event.type) {
            case YAML_STREAM_END_EVENT:
                parsingDone = true;
                break;

            case YAML_SEQUENCE_END_EVENT:
                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                keyTracker.pop();
                break;

            case YAML_MAPPING_START_EVENT:
                if(!docMarker) {
                    docMarker = true;
                } else {
                    parsingItem = true;
                }
                break;

            case YAML_MAPPING_END_EVENT:
                if(!keyTracker.empty() && parsingItem) {
                    parsingItem = false;

                    topKey = keyTracker.top();
                    if(topKey == TARGET_CLUSTER_INFO) {
                        if(isConfigForCurrentTarget) {
                            TargetRegistry::getInstance()->addClusterMapping(lgcClusterID, phyClusterID);
                            lgcClusterID.clear();
                            phyClusterID.clear();
                        }
                    } else if(isConfigForCurrentTarget) {
                        if(isConfigForCurrentTarget) {
                            TargetRegistry::getInstance()->addClusterSpreadInfo(phyClusterID, numCoresString);
                            phyClusterID.clear();
                            numCoresString.clear();
                        }
                    }
                }

                break;

            case YAML_SCALAR_EVENT:
                if(event.data.scalar.value != nullptr) {
                    value = reinterpret_cast<char*>(event.data.scalar.value);
                }

                if(isKey(value)) {
                    if(value == TARGET_NAME_LIST) {
                        isConfigForCurrentTarget = false;
                    }
                    keyTracker.push(value);
                    break;
                }

                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                topKey = keyTracker.top();
                if(!isKeyTypeList(topKey)) {
                    keyTracker.pop();
                }

                if(topKey == TARGET_NAME_LIST) {
                    if(value == "*" || value == UrmSettings::targetConfigs.targetName) {
                        if(!deviceParsingDone) {
                            isConfigForCurrentTarget = true;
                            deviceParsingDone = true;
                        }
                    }
                } else if(topKey == TARGET_CLUSTER_INFO_LOGICAL_ID) {
                    lgcClusterID = value;
                } else if(topKey == TARGET_CLUSTER_INFO_PHYSICAL_ID) {
                    phyClusterID = value;
                } else if(topKey == TARGET_PER_CLUSTER_CORE_COUNT) {
                    numCoresString = value;
                }

                break;

            default:
                break;
        }
        yaml_event_delete(&event);
    }

    TEARDOWN_LIBYAML_PARSING
    return RC_SUCCESS;
}

ErrCode RestuneParser::parseInitConfigYamlNode(const std::string& filePath) {
    SETUP_LIBYAML_PARSING(filePath);

    ErrCode rc = RC_SUCCESS;

    int8_t parsingDone = false;
    int8_t docMarker = false;

    std::string value;
    std::string topKey;
    std::stack<std::string> keyTracker;

    CGroupConfigInfoBuilder* cGroupConfigBuilder = nullptr;
    MpamGroupConfigInfoBuilder* mpamGroupConfigBuilder = nullptr;
    CacheInfoBuilder* cacheInfoBuilder = nullptr;

    while(!parsingDone) {
        if(!yaml_parser_parse(&parser, &event)) {
            return RC_YAML_PARSING_ERROR;
        }

        switch(event.type) {
            case YAML_STREAM_END_EVENT:
                parsingDone = true;
                break;

            case YAML_SEQUENCE_END_EVENT:
                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                keyTracker.pop();
                break;

            case YAML_MAPPING_START_EVENT:
                if(!docMarker) {
                    docMarker = true;
                } else {
                    if(keyTracker.empty()) {
                        return RC_YAML_INVALID_SYNTAX;
                    }

                    topKey = keyTracker.top();
                    if(topKey == INIT_CONFIGS_ELEM_CGROUPS_LIST) {
                        cGroupConfigBuilder = new(std::nothrow) CGroupConfigInfoBuilder;
                        if(cGroupConfigBuilder == nullptr) {
                            return RC_MEMORY_ALLOCATION_FAILURE;
                        }

                    } else if(topKey == INIT_CONFIGS_ELEM_MPAM_GROUPS_LIST) {
                        mpamGroupConfigBuilder = new(std::nothrow) MpamGroupConfigInfoBuilder;
                        if(mpamGroupConfigBuilder == nullptr) {
                            return RC_MEMORY_ALLOCATION_FAILURE;
                        }

                    } else if(topKey == INIT_CONFIGS_ELEM_CACHE_INFO_LIST) {
                        cacheInfoBuilder = new(std::nothrow) CacheInfoBuilder;
                        if(cacheInfoBuilder == nullptr) {
                            return RC_MEMORY_ALLOCATION_FAILURE;
                        }
                    }
                }

                break;

            case YAML_MAPPING_END_EVENT:
                if(keyTracker.empty()) {
                    break;
                }

                topKey = keyTracker.top();
                if(topKey == INIT_CONFIGS_ELEM_CGROUPS_LIST) {
                    if(RC_IS_NOTOK(rc)) {
                        // Set the ID to -1, so that the Cgroup is not added and is cleaned up
                        cGroupConfigBuilder->setCGroupID("-1");
                    }

                    TargetRegistry::getInstance()->addCGroupMapping(cGroupConfigBuilder->build());

                    delete cGroupConfigBuilder;
                    cGroupConfigBuilder = nullptr;

                } else if(topKey == INIT_CONFIGS_ELEM_MPAM_GROUPS_LIST) {
                    if(RC_IS_NOTOK(rc)) {
                        // Set the ID to -1, so that the Cgroup is not added and is cleaned up
                        mpamGroupConfigBuilder->setLgcID("-1");
                    }

                    TargetRegistry::getInstance()->addMpamGroupMapping(mpamGroupConfigBuilder->build());

                    delete mpamGroupConfigBuilder;
                    mpamGroupConfigBuilder = nullptr;

                } else if(topKey == INIT_CONFIGS_ELEM_CACHE_INFO_LIST) {
                    if(RC_IS_NOTOK(rc)) {
                        cacheInfoBuilder->setType("");
                        cacheInfoBuilder->setNumBlocks("-1");
                    }

                    TargetRegistry::getInstance()->addCacheInfoMapping(cacheInfoBuilder->build());

                    delete cacheInfoBuilder;
                    cacheInfoBuilder = nullptr;
                }

                break;

            case YAML_SCALAR_EVENT:
                if(event.data.scalar.value != nullptr) {
                    value = reinterpret_cast<char*>(event.data.scalar.value);
                }

                if(isKey(value)) {
                    keyTracker.push(value);
                    break;
                }

                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                topKey = keyTracker.top();
                if(!isKeyTypeList(topKey)) {
                    keyTracker.pop();
                }

                ADD_TO_CGROUP_BUILDER(INIT_CONFIGS_ELEM_CGROUP_NAME, setCGroupName);
                ADD_TO_CGROUP_BUILDER(INIT_CONFIGS_ELEM_CGROUP_IDENTIFIER, setCGroupID);
                ADD_TO_CGROUP_BUILDER(INIT_CONFIGS_ELEM_CGROUP_CREATION, setCreationNeeded);
                ADD_TO_CGROUP_BUILDER(INIT_CONFIGS_ELEM_CGROUP_THREADED, setThreaded);

                ADD_TO_MPAM_GROUP_BUILDER(INIT_CONFIGS_ELEM_MPAM_GROUP_NAME, setName);
                ADD_TO_MPAM_GROUP_BUILDER(INIT_CONFIGS_ELEM_MPAM_GROUP_ID, setLgcID);
                ADD_TO_MPAM_GROUP_BUILDER(INIT_CONFIGS_ELEM_MPAM_GROUP_PRIORITY, setPriority);

                ADD_TO_CACHE_INFO_BUILDER(INIT_CONFIGS_ELEM_CACHE_INFO_TYPE, setType);
                ADD_TO_CACHE_INFO_BUILDER(INIT_CONFIGS_ELEM_CACHE_INFO_BLK_CNT, setNumBlocks);
                ADD_TO_CACHE_INFO_BUILDER(INIT_CONFIGS_ELEM_CACHE_INFO_PRIO_AWARE, setPriorityAware);

                break;

            default:
                break;
        }
        yaml_event_delete(&event);
    }

    TEARDOWN_LIBYAML_PARSING
    return rc;
}

ErrCode RestuneParser::parseSignalConfigYamlNode(const std::string& filePath, int8_t isBuSpecified) {
    SETUP_LIBYAML_PARSING(filePath);

    ErrCode rc = RC_SUCCESS;

    int8_t parsingDone = false;
    int8_t docMarker = false;
    int8_t parsingSignal = false;
    int8_t inResourcesMap = false;

    std::string value;
    std::string topKey;
    std::stack<std::string> keyTracker;
    std::vector<std::string> resValues;

    SignalInfoBuilder* signalInfoBuilder = nullptr;
    ResourceBuilder* resourceBuilder = nullptr;

    while(!parsingDone) {
        if(!yaml_parser_parse(&parser, &event)) {
            return RC_YAML_PARSING_ERROR;
        }

        switch(event.type) {
            case YAML_STREAM_END_EVENT:
                parsingDone = true;
                break;

            case YAML_MAPPING_START_EVENT:
                if(!docMarker) {
                    docMarker = true;
                } else {
                    if(keyTracker.empty()) {
                        return RC_YAML_INVALID_SYNTAX;
                    }

                    topKey = keyTracker.top();
                    if(topKey == SIGNAL_CONFIGS_ELEM_RESOURCES) {
                        inResourcesMap = true;
                        resourceBuilder = new(std::nothrow) ResourceBuilder;
                        if(resourceBuilder == nullptr) {
                            if(signalInfoBuilder != nullptr) {
                                delete(signalInfoBuilder);
                            }
                            return RC_YAML_PARSING_ERROR;
                        }

                    } else {
                        parsingSignal = true;
                        signalInfoBuilder = new(std::nothrow) SignalInfoBuilder;
                        if(signalInfoBuilder == nullptr) {
                            return RC_YAML_PARSING_ERROR;
                        }
                    }
                }

                break;

            case YAML_MAPPING_END_EVENT:
                if(inResourcesMap) {
                    inResourcesMap = false;
                    if(RC_IS_OK(rc)) {
                        rc = signalInfoBuilder->addResource(resourceBuilder->build());
                        delete(resourceBuilder);
                        resourceBuilder = nullptr;
                    }

                } else if(parsingSignal) {
                    parsingSignal = false;

                    if(RC_IS_NOTOK(rc)) {
                        // Set SigCategory so that the Signal gets discarded by Signal Regsitry
                        rc = signalInfoBuilder->setSignalCategory("0");
                    }

                    SignalRegistry::getInstance()->
                        registerSignal(signalInfoBuilder->build(), isBuSpecified);
                    delete(signalInfoBuilder);
                    signalInfoBuilder = nullptr;
                }

                break;

            case YAML_SEQUENCE_END_EVENT:
                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                topKey = keyTracker.top();
                keyTracker.pop();

                if(topKey == SIGNAL_CONFIGS_ELEM_RESOURCE_VALUES) {
                    if(RC_IS_OK(rc)) {
                        rc = resourceBuilder->setNumValues(resValues.size());
                    }

                    for(size_t idx = 0; idx < resValues.size(); idx++) {
                        if(RC_IS_OK(rc)) {
                            rc = resourceBuilder->addValue(idx, resValues[idx]);
                        }
                    }

                    resValues.clear();
                }
                break;

            case YAML_SCALAR_EVENT:
                if(event.data.scalar.value != nullptr) {
                    value = reinterpret_cast<char*>(event.data.scalar.value);
                }

                if(isKey(value)) {
                    keyTracker.push(value);
                    break;
                }

                if(keyTracker.empty()) {
                    if(signalInfoBuilder != nullptr) {
                        delete signalInfoBuilder;
                    }
                    if(resourceBuilder != nullptr) {
                        delete resourceBuilder;
                    }
                    return RC_YAML_INVALID_SYNTAX;
                }

                topKey = keyTracker.top();
                if(!isKeyTypeList(topKey)) {
                    keyTracker.pop();
                }

                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_TARGETS_ENABLED, addTargetEnabled);
                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_TARGETS_DISABLED, addTargetDisabled);
                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_PERMISSIONS, addPermission);
                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_DERIVATIVES, addDerivative);
                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_SIGID, setSignalID);
                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_CATEGORY, setSignalCategory);
                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_SIGTYPE, setSignalType);
                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_NAME, setName);
                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_TIMEOUT, setTimeout);
                ADD_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_ENABLE, setIsEnabled);

                ADD_RES_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_RESOURCE_CODE, setResCode);
                ADD_RES_TO_SIGNAL_BUILDER(SIGNAL_CONFIGS_ELEM_RESOURCE_RESINFO, setResInfo);

                if(topKey == SIGNAL_CONFIGS_ELEM_RESOURCE_VALUES) {
                    resValues.push_back(value);
                }

                break;

            default:
                break;
        }

        yaml_event_delete(&event);
    }

    if(signalInfoBuilder != nullptr) {
        delete(signalInfoBuilder);
    }

    if(resourceBuilder != nullptr) {
        delete(resourceBuilder);
    }

    TEARDOWN_LIBYAML_PARSING
    return rc;
}

ErrCode RestuneParser::parseExtFeatureConfigYamlNode(const std::string& filePath) {
    SETUP_LIBYAML_PARSING(filePath);

    ErrCode rc = RC_SUCCESS;

    int8_t parsingDone = false;
    int8_t docMarker = false;
    int8_t parsingFeature = false;

    std::string value;
    std::string topKey;
    std::stack<std::string> keyTracker;

    ExtFeatureInfoBuilder* extFeatureInfoBuilder = nullptr;

    while(!parsingDone) {
        if(!yaml_parser_parse(&parser, &event)) {
            return RC_YAML_PARSING_ERROR;
        }

        switch(event.type) {
            case YAML_STREAM_END_EVENT:
                parsingDone = true;
                break;

            case YAML_MAPPING_START_EVENT:
                if(!docMarker) {
                    docMarker = true;
                } else {
                    extFeatureInfoBuilder = new(std::nothrow) ExtFeatureInfoBuilder;
                    if(extFeatureInfoBuilder == nullptr) {
                        return RC_YAML_PARSING_ERROR;
                    }

                    parsingFeature = true;
                }
                break;

            case YAML_MAPPING_END_EVENT:
                if(parsingFeature) {
                    parsingFeature = false;
                    if(RC_IS_NOTOK(rc)) {
                        extFeatureInfoBuilder->setLib("");
                    }

                    ExtFeaturesRegistry::getInstance()->
                        registerExtFeature(extFeatureInfoBuilder->build());

                    delete extFeatureInfoBuilder;
                    extFeatureInfoBuilder = nullptr;
                }

                break;

            case YAML_SEQUENCE_END_EVENT:
                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                keyTracker.pop();
                break;

            case YAML_SCALAR_EVENT:
                if(event.data.scalar.value != nullptr) {
                    value = reinterpret_cast<char*>(event.data.scalar.value);
                }

                if(isKey(value)) {
                    keyTracker.push(value);
                    break;
                }

                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                topKey = keyTracker.top();
                if(!isKeyTypeList(topKey)) {
                    keyTracker.pop();
                }

                ADD_TO_EXT_FEATURE_BUILDER(EXT_FEATURE_CONFIGS_ELEM_ID, setId);
                ADD_TO_EXT_FEATURE_BUILDER(EXT_FEATURE_CONFIGS_ELEM_LIB, setLib);
                ADD_TO_EXT_FEATURE_BUILDER(EXT_FEATURE_CONFIGS_ELEM_NAME, setName);
                ADD_TO_EXT_FEATURE_BUILDER(EXT_FEATURE_CONFIGS_ELEM_SUBSCRIBER_LIST, addSignalSubscribedTo);

                break;

            default:
                break;
        }

        yaml_event_delete(&event);
    }

    TEARDOWN_LIBYAML_PARSING
    return rc;
}

ErrCode RestuneParser::parsePerAppConfigYamlNode(const std::string& filePath) {
    SETUP_LIBYAML_PARSING(filePath);

    ErrCode rc = RC_SUCCESS;

    int8_t parsingDone = false;
    int8_t docMarker = false;
    int8_t inThreadList = false;
    int8_t inConfigList = false;

    std::string value;
    std::string topKey;
    std::vector<std::string> itemArray;
    std::stack<std::string> keyTracker;

    AppConfigBuilder* appConfigBuider = nullptr;

    while(!parsingDone) {
        if(!yaml_parser_parse(&parser, &event)) {
            return RC_YAML_PARSING_ERROR;
        }

        switch(event.type) {
            case YAML_STREAM_END_EVENT:
                parsingDone = true;
                break;

            case YAML_MAPPING_START_EVENT:
                if(!docMarker) {
                    docMarker = true;
                } else {
                    topKey = keyTracker.top();
                    if(topKey == APP_CONFIGS_ROOT && appConfigBuider == nullptr) {
                        appConfigBuider = new AppConfigBuilder;
                    }
                }

                break;

            case YAML_MAPPING_END_EVENT:
                if(keyTracker.empty()) {
                    break;
                }

                topKey = keyTracker.top();
                if(topKey == APP_CONFIGS_ROOT) {
                    // Add to registry
                    AppConfigs::getInstance()->registerAppConfig(appConfigBuider->build());
                    appConfigBuider = nullptr;
                }
                break;

            case YAML_SEQUENCE_START_EVENT:
                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                topKey = keyTracker.top();

                if(topKey == APP_CONFIGS_THREAD_LIST) {
                    inThreadList = true;
                } else if(topKey == APP_CONFIGS_CONFIGURATION_LIST) {
                    inConfigList = true;
                }

                break;

            case YAML_SEQUENCE_END_EVENT:
                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                if(inThreadList) {
                    // Add threads to builder
                    if(RC_IS_OK(rc)) {
                        rc = appConfigBuider->setNumThreads(itemArray.size() / 2);
                        if(RC_IS_NOTOK(rc)) {
                            return RC_YAML_INVALID_SYNTAX;
                        }
                    }

                    int32_t listIndex = 0;
                    for(int32_t i = 0; i < (int32_t)itemArray.size(); i += 2) {
                        if(RC_IS_OK(rc)) {
                            rc = appConfigBuider->addThreadMapping(listIndex, itemArray[i], itemArray[i + 1]);
                            listIndex++;
                            if(RC_IS_NOTOK(rc)) {
                                return RC_YAML_INVALID_SYNTAX;
                            }
                        }
                    }

                    itemArray.clear();
                    inThreadList = !inThreadList;

                } else if(inConfigList) {
                    if(RC_IS_OK(rc)) {
                        rc = appConfigBuider->setNumSigCodes(itemArray.size());
                        if(RC_IS_NOTOK(rc)) {
                            return RC_YAML_INVALID_SYNTAX;
                        }
                    }
                    for(size_t i = 0; i < itemArray.size(); i++) {
                        if(RC_IS_OK(rc)) {
                            rc = appConfigBuider->addSigCode(i, itemArray[i]);
                            if(RC_IS_NOTOK(rc)) {
                                return RC_YAML_INVALID_SYNTAX;
                            }
                        }
                    }

                    itemArray.clear();
                    inConfigList = !inConfigList;
                }

                keyTracker.pop();
                break;

            case YAML_SCALAR_EVENT:
                if(event.data.scalar.value != nullptr) {
                    value = reinterpret_cast<char*>(event.data.scalar.value);
                }

                if(isKey(value)) {
                    keyTracker.push(value);
                    break;
                }

                if(keyTracker.empty()) {
                    return RC_YAML_INVALID_SYNTAX;
                }

                topKey =  keyTracker.top();
                if(!inConfigList && !inThreadList) {
                    // Not in any list, pop out the key
                    keyTracker.pop();
                }

                if(topKey == APP_CONFIGS_APP_NAME) {
                    if(RC_IS_OK(rc)) {
                        rc = appConfigBuider->setAppName(value);
                        if(RC_IS_NOTOK(rc)) {
                            return RC_YAML_INVALID_SYNTAX;
                        }
                    }
                } else if(topKey == APP_CONFIGS_THREAD_LIST || topKey == APP_CONFIGS_CONFIGURATION_LIST) {
                    itemArray.push_back(value);
                }

                break;

            default:
                break;
        }

        yaml_event_delete(&event);
    }

    TEARDOWN_LIBYAML_PARSING
    return rc;
}

ErrCode RestuneParser::parseResourceConfigs(const std::string& filePath, int8_t isBuSpecified) {
    return parseResourceConfigYamlNode(filePath, isBuSpecified);
}

ErrCode RestuneParser::parsePropertiesConfigs(const std::string& filePath) {
    return parsePropertiesConfigYamlNode(filePath);
}

ErrCode RestuneParser::parseTargetConfigs(const std::string& filePath) {
    return parseTargetConfigYamlNode(filePath);
}

ErrCode RestuneParser::parseInitConfigs(const std::string& filePath) {
    return parseInitConfigYamlNode(filePath);
}

ErrCode RestuneParser::parseSignalConfigs(const std::string& filePath, int8_t isBuSpecified) {
    return parseSignalConfigYamlNode(filePath, isBuSpecified);
}

ErrCode RestuneParser::parseExtFeaturesConfigs(const std::string& filePath) {
    return parseExtFeatureConfigYamlNode(filePath);
}

ErrCode RestuneParser::parsePerAppConfigs(const std::string& filePath) {
    return parsePerAppConfigYamlNode(filePath);
}

ErrCode RestuneParser::parse(ConfigType configType, const std::string& filePath, int8_t isBuSpecified) {
    ErrCode rc = RC_SUCCESS;

    switch(configType) {
        case ConfigType::RESOURCE_CONFIG: {
            rc = this->parseResourceConfigs(filePath, isBuSpecified);
            break;
        }
        case ConfigType::PROPERTIES_CONFIG: {
            rc = this->parsePropertiesConfigs(filePath);
            break;
        }
        case ConfigType::TARGET_CONFIG: {
            rc = this->parseTargetConfigs(filePath);
            break;
        }
        case ConfigType::INIT_CONFIG: {
            rc = this->parseInitConfigs(filePath);
            break;
        }
        case ConfigType::SIGNALS_CONFIG: {
            rc = this->parseSignalConfigs(filePath, isBuSpecified);
            break;
        }
        case ConfigType::EXT_FEATURES_CONFIG: {
            rc = this->parseExtFeaturesConfigs(filePath);
            break;
        }
        case ConfigType::APP_CONFIG: {
            rc = this->parsePerAppConfigYamlNode(filePath);
            break;
        }
        default: {
            rc = RC_BAD_ARG;
            break;
        }
    }
    return rc;
}
