// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef REQUEST_MANAGER_H
#define REQUEST_MANAGER_H

#include <unordered_set>
#include <unordered_map>
#include <memory>

#include "Request.h"
#include "CocoTable.h"
#include "ClientDataManager.h"

typedef std::pair<Request*, int8_t> RequestInfo;

enum RequestListType {
    ACTIVE_TUNE = 0,
    PENDING_TUNE,
};

enum RequestProcessingStatus : int8_t {
    REQ_UNCHANGED = 0x01,
    REQ_CANCELLED = 0x02,
    REQ_COMPLETED = 0x04,
    REQ_NOT_FOUND = 0x08,
};

/**
 * @brief RequestManager
 * @details Responsible for Tracking and Maintaining all the active Requests, currently
 *          submitted to the Resource Tuner Server. Additionally it is responsible for performing
 *          Request Duplication Check, which aims to improve System efficiency by reducing
 *          wasteful duplicate processing.
 */
class RequestManager {
private:
    static std::shared_ptr<RequestManager> mReqeustManagerInstance;
    static std::mutex instanceProtectionLock;

    int64_t mTotalRequestServed;
    std::unordered_set<Request*> mRequestsList[2];
    std::unordered_map<int64_t, RequestInfo> mActiveRequests;
    MinLRUCache mUntuneCache;
    std::shared_timed_mutex mRequestMapMutex;

    RequestManager();

    int8_t checkOwnership(Request* request, Request* targetRequest);
    int8_t isSane(Request* request);
    int8_t requestMatch(Request* request);

public:
    ~RequestManager();

    /**
     * @brief Check if a Request with the specified handle exists in the RequestMap
     * @details This routine is used by the retune/untune APIs
     * @param handle Request Handle
     * @return int8_t:\n
     *             - 1: if a request with the specified handle exists\n
     *             - 0: otherwise
     */
    int8_t verifyHandle(int64_t handle);

    /**
     * @brief Checks whether the specified Request should be added to the RequestMap
     * @details This routine will perform Request Sanity and Duplicate checking.
     * @param request pointer to the request to be added to the map
     * @return int8_t:\n
     *             - 1: if the request should be added\n
     *             - 0: otherwise
     */
    int8_t shouldRequestBeAdded(Request* request);

    /**
     * @brief Add the specified request to the RequestMap
     * @details This routine should only be called if shouldRequestBeAdded returns 1.
     * @param request pointer to the request to be added to the map
     */
    int8_t addRequest(Request* request);

    /**
     * @brief Remove a given request from the RequestMap
     * @param request pointer to the request to be removed from the map
     */
    void removeRequest(Request* request);

    /**
     * @brief Retrieve the Request with the given Handle.
     * @param handle Request Handle
     * @return Request*:\n
     *            - Pointer to the request with the specified index.
     */
    RequestInfo getRequestFromMap(int64_t handle);

    /**
     * @brief Get the current Global Active Requests Count
     * @return: int64_t
     */
    int64_t getActiveReqeustsCount();

    /**
     * @brief Mark the Request Handle, so that the Request won't be applied.
     * @details This method is used to Handle Untune Requests, as soon as an Untune
     *          Request for Handle h1 is received, we call this Rountine for h1, so that
     *          it doesn't get serviced in case it has not been inserted into the Coco
     *          Table yet.
     *          Note: If the Request was already inserted, then it will be untuned as and
     *          when the Untune Request gets serviced by the RequestQueue.
     *
     * @param handle Request Identifier, for which processing needs to be disabled.
     */
    int8_t disableRequestProcessing(int64_t handle);

    void markRequestAsComplete(int64_t handle);

    int8_t getRequestProcessingStatus(int64_t handle);

    std::vector<Request*> getPendingList();

    /**
     * @brief Handles Device Mode transition from RESUME to SUSPEND
     * @details As part of this routine, the CocoTable will be drained out, i.e. all active
     *          Requests will be untuned, and the Resources restored to their original values.
     *          Requests which are not eligible for background processing will be removed from the
     *          Active List and put into the Pending List, so that they get processed again when the
     *          Device transitions back to the RESUME mode.
     *
     */
    void moveToPendingList();

    void clearPending();

    static std::shared_ptr<RequestManager> getInstance() {
        if(mReqeustManagerInstance == nullptr) {
            instanceProtectionLock.lock();
            if(mReqeustManagerInstance == nullptr) {
                try {
                    mReqeustManagerInstance = std::shared_ptr<RequestManager> (new RequestManager());
                } catch(const std::bad_alloc& e) {
                    instanceProtectionLock.unlock();
                    return nullptr;
                }
            }
            instanceProtectionLock.unlock();
        }
        return mReqeustManagerInstance;
    }
};

#endif
