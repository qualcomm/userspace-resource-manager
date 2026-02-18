// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef EXT_FEATURES_REGISTRY_H
#define EXT_FEATURES_REGISTRY_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <dlfcn.h>

#include "SignalExtFeatureMapper.h"
#include "Utils.h"
#include "Signal.h"
#include "Logger.h"

#define INITIALIZE_FEATURE_ROUTINE "initFeature"
#define TEARDOWN_FEATURE_ROUTINE "tearFeature"
#define RELAY_FEATURE_ROUTINE "relayFeature"

typedef struct {
    uint32_t mFeatureId;
    std::string mFeatureLib;
    std::string mFeatureName;
    std::vector<uint32_t>* mSignalsSubscribedTo;
} ExtFeatureInfo;

typedef void (*ExtFeature)(void);
typedef void (*RelayFeature)(uint32_t, const std::string&, const std::string&, int32_t, std::vector<uint32_t>*);

/**
 * @brief ExtFeaturesRegistry
 * @details Stores information Relating to all the Ext Features registered with resource-tuner.
 *          Note: This information is extracted from Config YAML files.
 */
class __attribute__((visibility("default"))) ExtFeaturesRegistry {
private:
    static std::shared_ptr<ExtFeaturesRegistry> extFeaturesRegistryInstance;
    int32_t mTotalExtFeatures;
    std::vector<ExtFeatureInfo*> mExtFeaturesConfigs;

    std::unordered_map<uint32_t, int32_t> mSILMap;

    ExtFeaturesRegistry();

public:
    ~ExtFeaturesRegistry();

    /**
     * @brief Fetch a Feature Config with the given ID.
     * @param featureId An unsigned 32-bit feature identifier
     * @return ExtFeatureInfo*:\n
     *             - A Pointer to the registered ExtFeatureInfo object, if feature with the given ID exists.\n
     *             - nullptr: Otherwise
     */
    ExtFeatureInfo* getExtFeatureConfigById(uint32_t extFeatureId);

    /**
     * @brief Used to initialize all the registered features.
     * @details This routine will invoke the init callback associated with each of the
     *          registered features. This is done during server initialization.
     */
    void initializeFeatures();

    /**
     * @brief Used to cleanup all the registered features.
     * @details This routine will invoke the tear callback associated with each of the
     *          registered features. This is done during server teardown.
     */
    void teardownFeatures();

    /**
     * @brief Relay a request to a registered feature.
     * @param featureId An unsigned 32-bit feature identifier
     */
    ErrCode relayToFeature(uint32_t featureId, Signal* signal);

    int32_t getExtFeaturesConfigCount();
    std::vector<ExtFeatureInfo*> getExtFeaturesConfigs();

    void registerExtFeature(ExtFeatureInfo* extFeatureInfo);
    void displayExtFeatures();

    static std::shared_ptr<ExtFeaturesRegistry> getInstance() {
        if(extFeaturesRegistryInstance == nullptr) {
            extFeaturesRegistryInstance = std::shared_ptr<ExtFeaturesRegistry> (new ExtFeaturesRegistry());
        }
        return extFeaturesRegistryInstance;
    }
};

class ExtFeatureInfoBuilder {
private:
    ExtFeatureInfo* mFeatureInfo;

public:
    ExtFeatureInfoBuilder();

    ErrCode setId(const std::string& featureIdString);
    ErrCode setName(const std::string& featureName);
    ErrCode setLib(const std::string& featureLib);
    ErrCode addSignalSubscribedTo(const std::string& sigCodeString);

    ExtFeatureInfo* build();
};

#endif
