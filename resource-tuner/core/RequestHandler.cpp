// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "RestuneInternal.h"

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

static int8_t performPhysicalMapping(int32_t& coreValue, int32_t& clusterValue) {
    std::shared_ptr<TargetRegistry> targetRegistry = TargetRegistry::getInstance();
    if(targetRegistry == nullptr) return false;

    int32_t physicalClusterValue = targetRegistry->getPhysicalClusterId(clusterValue);
    int32_t physicalCoreValue = 0;

    // For resources with ApplyType == "core":
    // A coreValue of 0, indicates apply the config value to all the cores part of the physical
    // cluster corresponding to the specified logical cluster ID.
    if(coreValue != 0) {
        // if a non-zero coreValue is provided, translate it and apply the config value
        // only to that physical core's resource node.
        physicalCoreValue = targetRegistry->getPhysicalCoreId(clusterValue, coreValue);
    }

    if(physicalCoreValue != -1 && physicalClusterValue != -1) {
        coreValue = physicalCoreValue;
        clusterValue = physicalClusterValue;
    } else {
        return false;
    }

    return true;
}

ErrCode translateToPhysicalIDs(Resource* resource) {
    ResConfInfo* rConf = ResourceRegistry::getInstance()->getResConf(resource->getResCode());
    switch(rConf->mApplyType) {
        case ResourceApplyType::APPLY_CORE: {
            int32_t coreValue = resource->getCoreValue();
            int32_t clusterValue = resource->getClusterValue();

            if(coreValue < 0) {
                TYPELOGV(VERIFIER_INVALID_LOGICAL_CORE, coreValue);
                return RC_INVALID_VALUE;
            }

            if(clusterValue < 0) {
                TYPELOGV(VERIFIER_INVALID_LOGICAL_CLUSTER, clusterValue);
                return RC_INVALID_VALUE;
            }

            // Perform logical to physical mapping here, as part of which verification can happen
            // Replace mResInfo with the Physical values here:
            if(!performPhysicalMapping(coreValue, clusterValue)) {
                TYPELOGV(VERIFIER_LOGICAL_TO_PHYSICAL_MAPPING_FAILED, resource->getResCode());
                return RC_INVALID_VALUE;
            }

            resource->setCoreValue(coreValue);
            resource->setClusterValue(clusterValue);
            return RC_SUCCESS;
        }

        case ResourceApplyType::APPLY_CLUSTER: {
            int32_t clusterValue = resource->getClusterValue();

            if(clusterValue < 0) {
                TYPELOGV(VERIFIER_INVALID_LOGICAL_CLUSTER, clusterValue);
                return RC_INVALID_VALUE;
            }

            // Perform logical to physical mapping here, as part of which verification can happen
            // Replace mResInfo with the Physical values here:
            int32_t physicalClusterID = TargetRegistry::getInstance()->getPhysicalClusterId(clusterValue);
            if(physicalClusterID == -1) {
                TYPELOGV(VERIFIER_LOGICAL_TO_PHYSICAL_MAPPING_FAILED, resource->getResCode());
                return RC_INVALID_VALUE;
            }

            resource->setClusterValue(physicalClusterID);
            return RC_SUCCESS;
        }

        default:
            // Other Apply Types don't require translation.
            return RC_SUCCESS;
    }

    return RC_INVALID_VALUE;
}

/**
 * @brief Verifies the validity of an incoming request.
 * @details This function checks the request's resources against configuration constraints,
 *          permissions, and performs logical-to-physical mapping.
 *
 * @param req Pointer to the Request object.
 * @return int8_t:\n
 *            - 1: if the request is valid.\n
 *            - 0: otherwise.
 */
static int8_t VerifyIncomingRequest(Request* req) {
    if(req->getDuration() < -1 || req->getDuration() == 0) return false;

    // Default: All Requests are supported in Display On Mode
    req->addProcessingMode(MODE_RESUME);

    // If the Device is in Display Off or Doze Mode, then no new Requests
    // shall be accepted.
    if(UrmSettings::targetConfigs.currMode != MODE_RESUME) {
        // Request cannot be accepted in the current device mode
        TYPELOGV(VERIFIER_INVALID_DEVICE_MODE, req->getHandle());
        return false;
    }
    int8_t clientPermissions =
        ClientDataManager::getInstance()->getClientLevelByID(req->getClientPID());
    // If the client permissions could not be determined, reject this request.
    // This could happen if the /proc/<pid>/status file for the Process could not be opened.
    if(clientPermissions == -1) {
        TYPELOGV(VERIFIER_INVALID_PERMISSION, req->getClientPID(), req->getClientTID());
        return false;
    }

    // Check Request Priority
    int8_t reqSpecifiedPriority = req->getPriority();
    if(reqSpecifiedPriority > RequestPriority::REQ_PRIORITY_LOW ||
       reqSpecifiedPriority < RequestPriority::REQ_PRIORITY_HIGH) {
        TYPELOGV(VERIFIER_INVALID_PRIORITY, reqSpecifiedPriority);
        return false;
    }

    int8_t allowedPriority = getRequestPriority(clientPermissions, reqSpecifiedPriority);
    if(allowedPriority == -1) return false;
    req->setPriority(allowedPriority);

    if(req->getResDlMgr() == nullptr) {
        return false;
    }

    DL_ITERATE(req->getResDlMgr()) {
        if(iter == nullptr) {
            return false;
        }

        ResIterable* resIter = (ResIterable*) iter;
        if(resIter == nullptr || resIter->mData == nullptr) return false;

        Resource* resource = (Resource*) resIter->mData;
        if(resource == nullptr) {
            return false;
        }

        ResConfInfo* resourceConfig = ResourceRegistry::getInstance()->getResConf(resource->getResCode());

        // Basic sanity: Invalid ResCode
        if(resourceConfig == nullptr) {
            TYPELOGV(VERIFIER_INVALID_OPCODE, resource->getResCode());
            return false;
        }

        if(resource->getValuesCount() == 1) {
            // Verify value is in the range [LT, HT]
            int32_t configValue = resource->getValueAt(0);
            int32_t lowThreshold = resourceConfig->mLowThreshold;
            int32_t highThreshold = resourceConfig->mHighThreshold;

            if((lowThreshold != -1 && highThreshold != -1) &&
                (configValue < lowThreshold || configValue > highThreshold)) {
                TYPELOGV(VERIFIER_VALUE_OUT_OF_BOUNDS, configValue, resource->getResCode());
                return false;
            }
        } else {
            // No Range Check Verification mechanism is provided for Multi-Valued Resources
            // by default. Users are expected to provide their own Applier and Tear Callbacks
            // for such Resources, through the Extension Interface.
        }

        // Check for Client permissions
        if(resourceConfig->mPermissions == PERMISSION_SYSTEM && clientPermissions == PERMISSION_THIRD_PARTY) {
            TYPELOGV(VERIFIER_NOT_SUFFICIENT_PERMISSION, resource->getResCode());
            return false;
        }

        // Check if logical to physical mapping is needed for the resource, if needed
        // try to perform the translation.
        if(RC_IS_NOTOK(translateToPhysicalIDs(resource))) {
            // Translation needed but could not be performed, reject the request
            return false;
        }
    }

    return true;
}

static int8_t addToRequestManager(Request* request) {
    std::shared_ptr<RequestManager> requestManager = RequestManager::getInstance();

    if(requestManager->shouldRequestBeAdded(request)) {
        if(!requestManager->addRequest(request)) {
            TYPELOGV(REQUEST_MANAGER_DUPLICATE_FOUND, request->getHandle());
            return false;
        }
        return true;
    }

    return false;
}

static void processIncomingRequest(Request* request, int8_t isValidated=false) {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RateLimiter> rateLimiter = RateLimiter::getInstance();
    std::shared_ptr<RequestManager> requestManager = RequestManager::getInstance();
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();

    pid_t clientPid = request->getClientPID();
    pid_t clientTid = request->getClientTID();

    if(request->getRequestType() == REQ_RESOURCE_TUNING) {
        // Perform a Global Rate Limit Check before Processing the Request
        // This is to check if the current Active Request count has hit the
        // Max Number of Concurrent Requests Allowed Threshold
        // If the Threshold has been hit, we don't process the Request any further.
        // Note: Exempt Untune Requests from Global Rate Limiting
        if(!rateLimiter->isGlobalRateLimitHonored()) {
            TYPELOGV(RATE_LIMITER_GLOBAL_RATE_LIMIT_HIT, request->getHandle());
            // Free the Request Memory Block
            Request::cleanUpRequest(request);
            return;
        }

        // Client Checks
        if(!clientDataManager->clientExists(clientPid, clientTid)) {
            if(!clientDataManager->createNewClient(clientPid, clientTid)) {
                // Client Entry Could not be Created, don't Proceed further with the Request
                TYPELOGV(CLIENT_ENTRY_CREATION_FAILURE, request->getHandle());

                // Free the Request Memory Block
                Request::cleanUpRequest(request);
                return;
            }
        }
    }

    if(request->getRequestType() == REQ_RESOURCE_UNTUNING ||
       request->getRequestType() == REQ_RESOURCE_RETUNING) {
        // Note: Client Data Manager initialisation is not necessary for Untune / Retune Requests,
        // since the client is expected to be allocated already (as part of the Tune Request)
        if(!isValidated && !clientDataManager->clientExists(clientPid, clientTid)) {
            // Client does not exist, drop the request
            Request::cleanUpRequest(request);
            return;
        }

        if(request->getRequestType() == REQ_RESOURCE_UNTUNING) {
            // Update the Processing Status for this handle to false
            // This handles the Edge Case where the Client sends the Untune Request immediately.
            // Edge cases are listed below:
            // 1. Client sends the Untune Request immediately after sending the Tune Request,
            //    before a Client Data Entry is even created for the Tune Request.
            // 2. Tune and Untune Requests are sent concurrently and the Untune Request is picked
            //    up for Processing in the RequestQueue before the Tune Request is added to the RequestManager.
            if(!requestManager->disableRequestProcessing(request->getHandle())) {
                Request::cleanUpRequest(request);
                return;
            }
        }
    }

    // Trusted clients can skip per-client rate limiter checks
    if(!isValidated && !rateLimiter->isRateLimitHonored(request->getClientTID())) {
        TYPELOGV(RATE_LIMITER_RATE_LIMITED, request->getClientTID(), request->getHandle());
        Request::cleanUpRequest(request);
        return;
    }

    // For requests of type - retune / untune
    // - Check if request with specified handle exists or not
    // For requests of type tune
    // - Check for duplicate requests, using the RequestManager
    // - Check if any of the outstanding requests of this client matches the current request
    if(request->getRequestType() == REQ_RESOURCE_UNTUNING ||
       request->getRequestType() == REQ_RESOURCE_RETUNING) {
        if(!requestManager->verifyHandle(request->getHandle())) {
            TYPELOGV(REQUEST_MANAGER_REQUEST_NOT_ACTIVE, request->getHandle());
            Request::cleanUpRequest(request);
        } else {
            // Add it to request queue for further processing
            requestQueue->addAndWakeup(request);
        }

        return;
    }

    // Validate the Request
    if(!isValidated) {
        if(VerifyIncomingRequest(request)) {
            TYPELOGV(VERIFIER_REQUEST_VALIDATED, request->getHandle());

            if(addToRequestManager(request)) {
                // Add this request to the RequestQueue
                requestQueue->addAndWakeup(request);
            } else {
                Request::cleanUpRequest(request);
            }

        } else {
            TYPELOGV(VERIFIER_STATUS_FAILURE, request->getHandle());
            Request::cleanUpRequest(request);
        }
    } else {
        if(addToRequestManager(request)) {
            // Add this request to the RequestQueue
            requestQueue->addAndWakeup(request);
        } else {
            Request::cleanUpRequest(request);
        }
    }
}

void submitResProvisionRequest(Request* request, int8_t isValidated) {
    processIncomingRequest(request, isValidated);
}

void submitResProvisionReqMsg(void* msg) {
    if(msg == nullptr) return;

    MsgForwardInfo* info = (MsgForwardInfo*) msg;
    Request* request = nullptr;

    if(info == nullptr) return;

    try {
        request = MPLACED(Request);
        if(RC_IS_NOTOK(request->deserialize(info->mBuffer))) {
            Request::cleanUpRequest(request);
        } else {
            if(request->getRequestType() == REQ_RESOURCE_TUNING) {
                request->setHandle(info->mHandle);
            }
            processIncomingRequest(request);
        }

    } catch(const std::bad_alloc& e) {
        TYPELOGV(REQUEST_MEMORY_ALLOCATION_FAILURE, e.what());
    }

    if(info != nullptr) {
        FreeBlock<char[REQ_BUFFER_SIZE]>(info->mBuffer);
        FreeBlock<MsgForwardInfo>(info);
    }
}

size_t submitPropGetRequest(void* msg, std::string& result) {
    if(msg == nullptr) {
        return 0;
    }
    std::shared_ptr<PropertiesRegistry> propReg = PropertiesRegistry::getInstance();

    MsgForwardInfo* info = (MsgForwardInfo*) msg;
    if(info == nullptr) {
        return 0;
    }

    int8_t* ptr8 = (int8_t*)info->mBuffer;
    DEREF_AND_INCR(ptr8, int8_t);
    DEREF_AND_INCR(ptr8, int8_t);

    uint64_t* ptr64 = (uint64_t*)ptr8;
    DEREF_AND_INCR(ptr64, uint64_t);

    char* charIterator = (char*)ptr64;
    const char* propNamePtr = charIterator;

    while(*charIterator != '\0') {
        charIterator++;
    }
    charIterator++;
    std::string propName = propNamePtr;

    std::string buffer = "";
    size_t writtenBytes = propReg->queryProperty(propName, buffer);
    if(writtenBytes > 0) {
        result = buffer;
    }

    return writtenBytes;
}

size_t submitPropGetRequest(const std::string& propName,
                            std::string& result,
                            const std::string& defVal) {
    std::shared_ptr<PropertiesRegistry> propReg = PropertiesRegistry::getInstance();

    std::string buffer = "";
    size_t writtenBytes = propReg->queryProperty(propName, buffer);
    if(writtenBytes > 0) {
        result = buffer;
    } else {
        result = defVal;
    }

    return writtenBytes;
}
