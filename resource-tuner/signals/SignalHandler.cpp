// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SignalInternal.h"

static int8_t getRequestPriority(int8_t clientPermissions, int8_t reqSpecifiedPriority) {
    if(clientPermissions == PERMISSION_SYSTEM) {
        switch(reqSpecifiedPriority) {
            case RequestPriority::REQ_PRIORITY_HIGH:
                return SYSTEM_HIGH;
            case RequestPriority::REQ_PRIORITY_LOW:
                return SYSTEM_LOW;
            default:
                return -1;
        }

    } else if(clientPermissions == PERMISSION_THIRD_PARTY) {
        switch(reqSpecifiedPriority) {
            case RequestPriority::REQ_PRIORITY_HIGH:
                return THIRD_PARTY_HIGH;
            case RequestPriority::REQ_PRIORITY_LOW:
                return THIRD_PARTY_LOW;
            default:
                return -1;
        }
    }

    // Note: Since Client Permissions and Request Specified Priority have already been individually
    // validated, hence control should not reach here.
    return -1;
}

/**
 * @brief Verifies the validity of an incoming Signal.
 *
 * @details This function checks the incoming Signal request against the Signal Registry.
 *          It ensures that the Signal exists in the registry and that the request is valid.
 *
 * @param req Pointer to the Request object.
 * @return int8_t:\n
 *            - 1: if the request is valid.
 *            - 0: otherwise.
 */
static int8_t VerifyIncomingRequest(Signal* signal) {
    std::shared_ptr<SignalRegistry> sigRegistry = SignalRegistry::getInstance();

    // Check if a Signal with the given ID exists in the Registry
    SignalInfo* signalInfo =
        sigRegistry->getSignalConfigById(signal->getSignalCode(), signal->getSignalType());

    // Basic sanity: Invalid ResCode
    if(signalInfo == nullptr) {
        TYPELOGV(VERIFIER_INVALID_OPCODE, signal->getSignalCode());
        return false;
    }

    // Client Permission Checks
    int8_t clientPermissions = ClientDataManager::getInstance()->getClientLevelByID(signal->getClientPID());
    if(clientPermissions == -1) {
        TYPELOGV(VERIFIER_INVALID_PERMISSION, signal->getClientPID(), signal->getClientTID());
        return false;
    }

    // Priority Checks
    int8_t reqSpecifiedPriority = signal->getPriority();
    if(reqSpecifiedPriority > RequestPriority::REQ_PRIORITY_LOW ||
       reqSpecifiedPriority < RequestPriority::REQ_PRIORITY_HIGH) {
        TYPELOGV(VERIFIER_INVALID_PRIORITY, reqSpecifiedPriority);
        return false;
    }

    int8_t allowedPriority = getRequestPriority(clientPermissions, reqSpecifiedPriority);
    if(allowedPriority == -1) return false;
    signal->setPriority(allowedPriority);

    // Does Client have sufficient permissions to acquire this signal
    int8_t permissionCheck = false;
    for(enum Permissions signalPermission: *signalInfo->mPermissions) {
        if(clientPermissions == signalPermission) {
            permissionCheck = true;
            break;
        }
    }

    // Client does not have the necessary permissions to tune this Resource.
    if(!permissionCheck) {
        TYPELOGV(VERIFIER_NOT_SUFFICIENT_SIGNAL_ACQ_PERMISSION, signal->getSignalCode());
        return false;
    }

    // If duration (timeout) is not specified, derive it from Signal Configs
    if(signal->getDuration() == 0) {
        // If the Client has not specified a duration to tune the Signal for,
        // We use the default duration for the Signal, specified in the Signal
        // Configs (YAML) file.
        signal->setDuration(signalInfo->mTimeout);
    }

    return true;
}

static Request* createResourceTuningRequest(Signal* signal) {
    try {
        std::shared_ptr<SignalRegistry> sigRegistry = SignalRegistry::getInstance();

        // Check if a Signal with the given ID exists in the Registry
        SignalInfo* signalInfo =
            sigRegistry->getSignalConfigById(signal->getSignalCode(), signal->getSignalType());

        if(signalInfo == nullptr) return nullptr;

        Request* request = MPLACED(Request);

        request->setRequestType(REQ_RESOURCE_TUNING);
        request->setHandle(signal->getHandle());
        request->setDuration(signal->getDuration());
        request->setProperties(signal->getProperties());
        request->setClientPID(signal->getClientPID());
        request->setClientTID(signal->getClientTID());

        std::vector<Resource*>* signalLocks = signalInfo->mSignalResources;

        int32_t listIndex = 0;
        for(int32_t i = 0; i < (int32_t)signalLocks->size(); i++) {
            if((*signalLocks)[i] == nullptr) {
                continue;
            }

            // Copy
            Resource* resource = MPLACEV(Resource, (*((*signalLocks)[i])));

            // fill placeholders if any
            for(int32_t j = 0; j < resource->getValuesCount(); j++) {
                if(resource->getValueAt(j) == -1) {
                    if(signal->getListArgs() == nullptr) return nullptr;
                    if(listIndex >= 0 && listIndex < signal->getNumArgs()) {
                        resource->setValueAt(j, signal->getListArgAt(listIndex));
                        listIndex++;
                    } else {
                        return nullptr;
                    }
                }
            }

            ResIterable* resIterable = MPLACED(ResIterable);
            resIterable->mData = resource;
            request->addResource(resIterable);
        }

        return request;

    } catch(const std::bad_alloc& e) {
        TYPELOGV(REQUEST_MEMORY_ALLOCATION_FAILURE_HANDLE, signal->getHandle(), e.what());
        return nullptr;
    }

    return nullptr;
}

static Request* createResourceUntuneRequest(Signal* signal) {
    Request* request = nullptr;

    try {
        request = MPLACED(Request);

    } catch(const std::bad_alloc& e) {
        TYPELOGV(REQUEST_MEMORY_ALLOCATION_FAILURE_HANDLE, signal->getHandle(), e.what());
        return nullptr;
    }

    request->setRequestType(REQ_RESOURCE_UNTUNING);
    request->setHandle(signal->getHandle());
    request->setDuration(-1);
    request->setProperties(signal->getProperties());
    request->setClientPID(signal->getClientPID());
    request->setClientTID(signal->getClientTID());

    return request;
}

static void processIncomingRequest(Signal* signal) {
    std::shared_ptr<RateLimiter> rateLimiter = RateLimiter::getInstance();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    if(!rateLimiter->isGlobalRateLimitHonored()) {
        TYPELOGV(RATE_LIMITER_GLOBAL_RATE_LIMIT_HIT, signal->getHandle());
        // Free the Signal Memory Block
        Signal::cleanUpSignal(signal);
        return;
    }

    if(signal->getRequestType() == REQ_SIGNAL_RELAY || signal->getRequestType() == REQ_SIGNAL_TUNING) {
        // Check if the client exists, if not create a new client tracking entry
        if(!clientDataManager->clientExists(signal->getClientPID(), signal->getClientTID())) {
            if(!clientDataManager->createNewClient(signal->getClientPID(), signal->getClientTID())) {
                // Failed to create a tracking entry, drop the Request.
                TYPELOGV(CLIENT_ENTRY_CREATION_FAILURE, signal->getHandle());

                // Free the Signal Memory Block
                Signal::cleanUpSignal(signal);
                return;
            }
        }
    } else {
        // In case of untune Requests, the Client should already exist
        if(!clientDataManager->clientExists(signal->getClientPID(), signal->getClientTID())) {
            // Client does not exist, drop the request
            Signal::cleanUpSignal(signal);
            return;
        }
    }

    // Rate Check the client
    if(!rateLimiter->isRateLimitHonored(signal->getClientTID())) {
        TYPELOGV(RATE_LIMITER_RATE_LIMITED, signal->getClientTID(), signal->getHandle());
        Signal::cleanUpSignal(signal);
        return;
    }

    switch(signal->getRequestType()) {
        case REQ_SIGNAL_TUNING: {
            // Verify
            if(!VerifyIncomingRequest(signal)) {
                TYPELOGV(VERIFIER_STATUS_FAILURE, signal->getHandle());
                Signal::cleanUpSignal(signal);
                return;
            }

            // Translate to Request and send to RequestQueue for application
            Request* request = createResourceTuningRequest(signal);
            FreeBlock<Signal>(static_cast<void*>(signal));

            // Submit the Resource Provisioning request for processing
            if(request != nullptr) {
                submitResProvisionRequest(request, false);
            } else {
                LOGE("RESTUNE_SIGNAL_QUEUE", "Malformed Signal Request");
            }
            break;
        }

        case REQ_SIGNAL_UNTUNING: {
            Request* request = createResourceUntuneRequest(signal);
            FreeBlock<Signal>(static_cast<void*>(signal));

            // Submit the Resource De-Provisioning request for processing
            if(request != nullptr) {
                submitResProvisionRequest(request, true);
            }
            break;
        }

        case REQ_SIGNAL_RELAY: {
            // Get all the subscribed Features
            std::vector<uint32_t> subscribedFeatures;
            int8_t featureExist = SignalExtFeatureMapper::getInstance()->getFeatures(
                signal->getSignalCode(),
                subscribedFeatures
            );

            if(featureExist) {
                // Relay
                for(uint32_t featureId: subscribedFeatures) {
                    ExtFeaturesRegistry::getInstance()->relayToFeature(featureId, signal);
                }
            }

            Signal::cleanUpSignal(signal);
            break;
        }

        default:
            break;
    }
}

ErrCode submitSignalRequest(void* msg) {
    if(msg == nullptr) return RC_BAD_ARG;

    ErrCode opStatus = RC_SUCCESS;
    MsgForwardInfo* info = (MsgForwardInfo*) msg;
    Signal* signal = nullptr;

    if(RC_IS_OK(opStatus)) {
        if(info == nullptr) {
            opStatus = RC_BAD_ARG;
        }
    }

    if(RC_IS_OK(opStatus)) {
        try {
            signal = MPLACED(Signal);
            opStatus = signal->deserialize(info->mBuffer);
            if(RC_IS_NOTOK(opStatus)) {
                Signal::cleanUpSignal(signal);
            }

        } catch(const std::bad_alloc& e) {
            TYPELOGV(REQUEST_MEMORY_ALLOCATION_FAILURE, e.what());
            opStatus = RC_MEMORY_ALLOCATION_FAILURE;
        }
    }

    if(RC_IS_OK(opStatus)) {
        if(signal->getRequestType() == REQ_SIGNAL_TUNING) {
            signal->setHandle(info->mHandle);
        }

        processIncomingRequest(signal);
    }

    if(info != nullptr) {
        FreeBlock<char[REQ_BUFFER_SIZE]>(info->mBuffer);
        FreeBlock<MsgForwardInfo>(info);
    }

    return RC_SUCCESS;
}
