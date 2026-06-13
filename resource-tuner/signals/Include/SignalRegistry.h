// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef SIGNAL_REGISTRY_H
#define SIGNAL_REGISTRY_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "Utils.h"
#include "Logger.h"
#include "Resource.h"
#include "UrmSettings.h"
#include "UrmPlatformAL.h"
#include "SignalInternal.h"

#define NSIG_PLACEHOLDER std::numeric_limits<int>::min()

/**
 * @struct SignalInfo
 * @brief Representation of a single Signal Configuration
 * @details This information is read from the Config files.\n
 *          Note this (SignalInfo) struct is separate from the Signal struct.
 */
typedef struct _signalInfo {
    /**
     * @brief Category of the Signal
     */
    uint8_t mSignalCategory;

    /**
     * @brief 16-bit Signal ID
     */
    uint16_t mSignalID;

    /**
     * @brief Signal Sub-Type
     */
    uint32_t mSigType;

    /**
     * @brief Signal Name, for ex: EARLY_WAKEUP
     */
    std::string mSignalName;

    /**
     * @brief Default Signal Timeout, to be used if Client specifies a duration
     *        of 0 in the tuneSignal API call.
     */
    int32_t mTimeout;
    /**
     * @brief Pointer to a list of Permissions, i.e. only Clients with one of
     *        these permissions can provision the signal.
     */
    std::vector<enum Permissions>* mPermissions;

    std::vector<std::string>* mDerivatives;

    /**
     * @brief Array of optional extra attributes associated with the Signal.
     *        Each index corresponds to a named attribute (see SIGNAL_EXTRA_ATTR_* constants).
     */
    uint32_t mExtraAttrs[SIGNAL_EXTRA_ATTRS_COUNT];

    /**
     * @brief Indicates whether any ExtraAttrs are present
     */
    int8_t mHasExtraAttrs;

    /**
     * @brief List of Actual Resource which will be Provisioned and the
     *        Values to be configured for the Resources.
     */
    std::vector<Resource*>* mSignalResources;

    _signalInfo* next;
} SignalInfo;

struct SignalConf {
    SignalInfo* mSignalInfo;
    SignalInfo* mFeatureSignals;
    SignalConf() : mSignalInfo(nullptr), mFeatureSignals(nullptr) {}
};

/**
 * @brief SignalRegistry
 * @details Stores information Relating to all the Signals available for Tuning.
 *          Note: This information is extracted from Config YAML files.
 */
class SignalRegistry {
private:
    static std::shared_ptr<SignalRegistry> signalRegistryInstance;

    int32_t mTotalSignals;
    std::unordered_map<uint64_t, SignalConf> mSignalsConfigs;

    SignalRegistry();

    SignalInfo* findBestExtraAttrsMatch(SignalInfo* featureSignalHead,
                                        int32_t numArgs,
                                        const uint32_t* extraAttrs);
    int8_t isDuplicateConfig(SignalInfo* src, SignalInfo* dest);
    void addToFeaturedSigList(SignalInfo** mFeatureSignals,
                              SignalInfo* newSignal,
                              int8_t& isOverriden);

public:
    ~SignalRegistry();

    /**
     * @brief Used to register a Config specified (through YAML) Signal with Resource Tuner
     * @details The Signal Info is parsed from YAML files. If the SignalInfo provided is
     *          Malformed, then it will be freed as part of this routine, else it will
     *          be added to the "mSignalsConfigs" vector.
     */
    void registerSignal(SignalInfo* signalInfo);

    int8_t isSignalConfigMalformed(SignalInfo* sConf);

   /**
    * @brief Get the SignalInfo object corresponding to the given Signal code.
    * @param sigID      A 64-bit signal code.
    * @param extraAttrs Optional caller-provided array of size SIGNAL_EXTRA_ATTRS_COUNT,
    *                   indexed via SignalExtraAttrIndex, carrying Fps/Height/Width values.
    *                   Pass nullptr (default) for signals without extra attributes.
    *                   A non-null pointer selects the extra-attrs storage path.
    * @return SignalInfo* or nullptr if not found.
    */
    SignalInfo* getSignalConfigById(uint64_t sigID,
                                    int32_t numArgs = 0,
                                    const uint32_t* extraAttrs = nullptr);

   /**
    * @brief Get the SignalInfo object corresponding to the given Signal ID and type.
    * @param sigCode    An unsigned 32-bit signal code.
    * @param sigType    An unsigned 32-bit signal sub-type.
    * @param extraAttrs Optional caller-provided array of size SIGNAL_EXTRA_ATTRS_COUNT,
    *                   indexed via SignalExtraAttrIndex, carrying Fps/Height/Width values.
    *                   Pass nullptr (default) for signals without extra attributes.
    *                   A non-null pointer selects the extra-attrs storage path.
    * @return SignalInfo* or nullptr if not found.
    */
    SignalInfo* getSignalConfigByIdAndType(uint32_t sigCode,
                                           uint32_t sigType,
                                           int32_t numArgs = 0,
                                           const uint32_t* extraAttrs = nullptr);

    int32_t getSignalsConfigCount();

    static std::shared_ptr<SignalRegistry> getInstance() {
        if(signalRegistryInstance == nullptr) {
            try {
                signalRegistryInstance = std::shared_ptr<SignalRegistry>(new SignalRegistry());
            } catch(const std::bad_alloc& e) {
                LOGE("RESTUNE_SIGNAL_REGISTRY",
                     "Failed to allocate memory for SignalRegistry: "
                     + std::string(e.what()));
                return nullptr;
            }
        }
        return signalRegistryInstance;
    }
};

class SignalInfoBuilder {
private:
    SignalInfo* mSignalInfo;

public:
    int32_t mTargetRefCount;

    SignalInfoBuilder();
    ~SignalInfoBuilder();

    ErrCode setSignalID(const std::string& signalOpIdString);
    ErrCode setSignalCategory(const std::string& categoryString);
    ErrCode setSignalType(const std::string& typeString);
    ErrCode setName(const std::string& signalName);
    ErrCode setTimeout(const std::string& timeoutString);
    ErrCode setIsEnabled(const std::string& isEnabledString);
    ErrCode addTargetEnabled(const std::string& target);
    ErrCode addTargetDisabled(const std::string& target);
    ErrCode addPermission(const std::string& permissionString);
    ErrCode addDerivative(const std::string& derivative);
    ErrCode addResource(Resource* resource);
    ErrCode setFps(const std::string& fpsString);
    ErrCode setHeight(const std::string& heightString);
    ErrCode setWidth(const std::string& widthString);
    void markExtraAttrsPresent();

    SignalInfo* build();
};

class ResourceBuilder {
private:
    Resource* mResource;

public:
    ResourceBuilder();

    ErrCode setResCode(const std::string& resCodeString);
    ErrCode setResInfo(const std::string& resInfoString);
    ErrCode setNumValues(int32_t valuesCount);
    ErrCode addValue(int32_t index, const std::string& value);

    Resource* build();
};

#endif
