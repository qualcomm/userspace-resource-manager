// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "RequestManager.h"

static int8_t resourceCmpPolicy(DLRootNode* src, DLRootNode* target) {
    if(target == nullptr) return false;
    // This covers the case where the client fires the exact same request multiple times
    Resource* res1 = (Resource*)((ResIterable*)src)->mData;
    Resource* res2 = (Resource*) ((ResIterable*)target)->mData;

    if(res1->getResCode() != res2->getResCode()) return false;
    if(res1->getResInfo() != res2->getResInfo()) return false;
    if(res1->getOptionalInfo() != res2->getOptionalInfo()) return false;
    if(res1->getValuesCount() != res2->getValuesCount()) return false;

    if(res1->getValuesCount() == 1) {
        if(res1->getValueAt(0) != res2->getValueAt(0)) {
            return false;
        }
    } else {
        for(int32_t i = 0; i < res1->getValuesCount(); i++) {
            int32_t res1Val = res1->getValueAt(i);
            int32_t res2Val = res2->getValueAt(i);
            if(res1Val != res2Val) {
                return false;
            }
        }
    }

    // Consideration:
    // How to efficiently check duplicate in cases where the client sends 2
    // requests with the same Resources, but in different orders. For example:
    // Rq1 -> Rs1, Rs2, Rs3
    // Rq2 - Rs1, Rs3, Rs2
    // - These requests are still duplicates, but the above logic won't catch it
    return true;
}

std::shared_ptr<RequestManager> RequestManager::mReqeustManagerInstance = nullptr;
std::mutex RequestManager::instanceProtectionLock{};

RequestManager::RequestManager() {
    this->mTotalRequestServed = 0;
    this->mActiveRequests.reserve(UrmSettings::metaConfigs.mMaxConcurrentRequests);
    this->mActiveRequests.max_load_factor(1.0f);
}

int8_t RequestManager::isSane(Request* request) {
    try {
        if(request == nullptr) {
            throw std::invalid_argument("Request is nullptr");
        }
    } catch(const std::exception& e) {
        LOGE("RESTUNE_REQUEST_MANAGER",
             "Cannot Check Request Sanity: " +  std::string(e.what()));
        return false;
    }

    if(request->getRequestType() != REQ_RESOURCE_TUNING &&
       request->getRequestType() != REQ_RESOURCE_RETUNING &&
       request->getRequestType() != REQ_RESOURCE_UNTUNING) {
        return false;
    }

    if(request->getHandle() <= 0 || request->getResourcesCount() < 0 ||
       request->getPriority() < 0 || request->getClientPID() < 0 || request->getClientTID() < 0) {
        return false;
    }

    if(request->getResDlMgr() == nullptr) return false;
    DL_ITERATE(request->getResDlMgr()) {
        if(iter == nullptr) return false;
        ResIterable* resIter = (ResIterable*) iter;
        if(resIter == nullptr || resIter->mData == nullptr) {
            return false;
        }
    }

    return true;
}

int8_t RequestManager::requestMatch(Request* request) {
    int32_t clientTID = request->getClientTID();

    // Get the list of Requests for this client
    std::unordered_set<int64_t>* clientHandles =
        ClientDataManager::getInstance()->getRequestsByClientID(clientTID);

    if(clientHandles == nullptr || clientHandles->size() == 0) {
        return false;
    }
    // Assumptions: Number of Resources per request won't be too large
    // If it is, we can use multiple threads from the pool for faster checking

    for(int64_t handle: *clientHandles) {
        Request* targetRequest = this->mActiveRequests[handle].first;
        if(targetRequest == nullptr) {
            continue;
        }

        // Check that the Number of Resources in the requests are the same
        if(request->getResourcesCount() != targetRequest->getResourcesCount()) {
            return false;
        }

        if(!request->getResDlMgr()->matchAgainst(targetRequest->getResDlMgr(), resourceCmpPolicy)) {
            return false;
        }
    }

    return true;
}

int8_t RequestManager::verifyHandle(int64_t handle) {
    this->mRequestMapMutex.lock_shared();
    int8_t handleExists = (this->mActiveRequests.find(handle) != this->mActiveRequests.end());
    this->mRequestMapMutex.unlock_shared();

    return handleExists;
}

RequestInfo RequestManager::getRequestFromMap(int64_t handle) {
    this->mRequestMapMutex.lock_shared();
    if(this->mActiveRequests.find(handle) == this->mActiveRequests.end()) {
        this->mRequestMapMutex.unlock_shared();
        return RequestInfo {nullptr, REQ_CANCELLED};
    }

    RequestInfo reqInfo = this->mActiveRequests[handle];
    this->mRequestMapMutex.unlock_shared();
    return reqInfo;
}

int8_t RequestManager::shouldRequestBeAdded(Request* request) {
    //sanity check.
    if(!isSane(request)) return false;

    this->mRequestMapMutex.lock_shared();
    if(this->mActiveRequests.size() >= UrmSettings::metaConfigs.mMaxConcurrentRequests) {
        this->mRequestMapMutex.unlock_shared();
        return false;
    }

    // Check for duplicates
    int8_t duplicateFound = this->requestMatch(request);

    this->mRequestMapMutex.unlock_shared();

    return !duplicateFound;
}

int8_t RequestManager::addRequest(Request* request) {
    if(request == nullptr) return false;
    this->mRequestMapMutex.lock();

    if(this->mActiveRequests.size() >= UrmSettings::metaConfigs.mMaxConcurrentRequests) {
        this->mRequestMapMutex.unlock();
        return false;
    }

    int64_t handle = request->getHandle();
    if(this->mActiveRequests.find(handle) != this->mActiveRequests.end()) {
        this->mActiveRequests.erase(handle);
        this->mRequestMapMutex.unlock();
        return false;
    }

    if(this->mUntuneCache.isPresent(handle)) {
        this->mRequestMapMutex.unlock();
        return false;
    }

    // Populate all the Trackers with info for this Request
    this->mTotalRequestServed++;
    this->mActiveRequests[handle] = {request, REQ_UNCHANGED};

    // Add this request handle to the client list
    int32_t clientTID = request->getClientTID();
    ClientDataManager::getInstance()->insertRequestByClientId(clientTID, handle);

    this->mRequestMapMutex.unlock();
    return true;
}

void RequestManager::removeRequest(Request* request) {
    if(request == nullptr) return;
    this->mRequestMapMutex.lock();

    // Remove the handle reference from the client handles list
    int32_t clientTID = request->getClientTID();
    int64_t handle = request->getHandle();

    ClientDataManager::getInstance()->deleteRequestByClientId(clientTID, handle);

    // Remove the handle from the list of active requests
    this->mActiveRequests.erase(handle);
    this->mRequestMapMutex.unlock();
}

std::vector<Request*> RequestManager::getPendingList() {
    this->mRequestMapMutex.lock_shared();
    std::vector<Request*> pendingList;
    for(Request* request: this->mRequestsList[PENDING_TUNE]) {
        pendingList.push_back(request);
    }
    this->mRequestMapMutex.unlock_shared();
    return pendingList;
}

int8_t RequestManager::disableRequestProcessing(int64_t handle) {
    this->mRequestMapMutex.lock();

    if(this->mActiveRequests.find(handle) == this->mActiveRequests.end()) {
        // Request not in the activeList
        this->mUntuneCache.insert(handle);
        this->mRequestMapMutex.unlock();
        return false;

    } else {
        this->mActiveRequests[handle].second |= REQ_CANCELLED;
    }

    this->mRequestMapMutex.unlock();
    return true;
}

int64_t RequestManager::getActiveReqeustsCount() {
    this->mRequestMapMutex.lock_shared();
    int32_t size = this->mActiveRequests.size();
    this->mRequestMapMutex.unlock_shared();
    return size;
}

void RequestManager::markRequestAsComplete(int64_t handle) {
    this->mRequestMapMutex.lock();
    if(this->mActiveRequests.find(handle) != this->mActiveRequests.end()) {
        this->mActiveRequests[handle].second |= REQ_COMPLETED;
    }
    this->mRequestMapMutex.unlock();
}

int8_t RequestManager::getRequestProcessingStatus(int64_t handle) {
    if(this->mActiveRequests.find(handle) != this->mActiveRequests.end()) {
        return this->mActiveRequests[handle].second;
    }

    return REQ_NOT_FOUND;
}

void RequestManager::moveToPendingList() {
    this->mRequestMapMutex.lock();

    // This method will essentially drain out the CocoTable
    // The Requests will be moved to the Pending List or Kept in the Active Requests List
    // based on whether background Processing is enabled for the Request.
    for(Request* request: this->mRequestsList[ACTIVE_TUNE]) {
        // Iterate over all the Requests in the activeRequestsList
        // - Send Corresponding Untune Request for each of the Requests
        // - Add the Requests to the Pending List, which are not eligible for background processing
        //   while removing them from the activeRequestsList
        if((request->getProcessingModes() & MODE_SUSPEND) == 0) {
            Request* untuneRequest = nullptr;
            try {
                untuneRequest = MPLACED(Request);
            } catch(const std::bad_alloc& e) {
                LOGI("RESTUNE_REQUEST_MANAGER"
                     "Failed to create Untune Request for Request: ", std::to_string(request->getHandle()));
            }

            if(untuneRequest != nullptr) {
                request->populateUntuneRequest(untuneRequest);

                // Keep the Untune Request's Priority as high as possible
                // So that all the existing Requests are untuned before the new Requests are Added.
                untuneRequest->setPriority(HIGH_TRANSFER_PRIORITY);
                RequestQueue::getInstance()->addAndWakeup(untuneRequest);
            }

            this->mRequestsList[PENDING_TUNE].insert(request);
        }
    }

    this->mRequestMapMutex.unlock();
}

void RequestManager::clearPending() {
    this->mRequestMapMutex.lock();
    this->mRequestsList[PENDING_TUNE].clear();
    this->mRequestMapMutex.unlock();
}

RequestManager::~RequestManager() {}

HandleCache::HandleCache(int32_t maxSize) {
    this->mMaxSize = maxSize;
    this->mHandleSet.reserve(this->mMaxSize);
}

void HandleCache::insert(int64_t handle) {
    if(this->mHandleSet.size() >= this->mMaxSize) {
        int64_t oldestHandle = this->mRecencyQueue.front();
        this->mRecencyQueue.pop();
        this->mHandleSet.erase(oldestHandle);
    }

    this->mHandleSet.insert(handle);
    this->mRecencyQueue.push(handle);
}

int8_t HandleCache::isPresent(int64_t handle) {
    return (this->mHandleSet.find(handle) != this->mHandleSet.end());
}
