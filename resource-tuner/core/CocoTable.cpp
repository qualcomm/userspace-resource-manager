// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "CocoTable.h"

static int8_t comparHBetter(DLRootNode* newNode, DLRootNode* targetNode) {
    Resource* first = (Resource*)((ResIterable*)newNode)->mData;
    Resource* second = (Resource*)((ResIterable*)targetNode)->mData;

    int32_t newValue;
    int32_t targetValue;

    if(first->getValuesCount() == 1) {
        newValue = first->getValueAt(0);
    } else {
        newValue = first->getValueAt(1);
    }

    if(second->getValuesCount() == 1) {
        targetValue = second->getValueAt(0);
    } else {
        targetValue = second->getValueAt(1);
    }

    return newValue > targetValue;
}

static int8_t comparLBetter(DLRootNode* newNode, DLRootNode* targetNode) {
    Resource* first = (Resource*)((ResIterable*)newNode)->mData;
    Resource* second = (Resource*)((ResIterable*)targetNode)->mData;

    int32_t newValue;
    int32_t targetValue;

    if(first->getValuesCount() == 1) {
        newValue = first->getValueAt(0);
    } else {
        newValue = first->getValueAt(1);
    }

    if(second->getValuesCount() == 1) {
        targetValue = second->getValueAt(0);
    } else {
        targetValue = second->getValueAt(1);
    }

    return newValue < targetValue;
}

std::shared_ptr<CocoTable> CocoTable::mCocoTableInstance = nullptr;
std::mutex CocoTable::instanceProtectionLock {};

CocoTable::CocoTable() {
    this->mResourceRegistry = ResourceRegistry::getInstance();
    int32_t totalResources = this->mResourceRegistry->getTotalResourcesCount();

    this->mCurrentlyAppliedPriority.resize(totalResources, -1);

    std::vector<int32_t> clusterIDs;
    TargetRegistry::getInstance()->getClusterIDs(clusterIDs);
    for(int32_t clusterID : clusterIDs) {
        int32_t index = this->mFlatClusterMap.size();
        this->mFlatClusterMap[clusterID] = index;
    }

    std::vector<CGroupConfigInfo*> cGroupConfigs;
    TargetRegistry::getInstance()->getCGroupConfigs(cGroupConfigs);
    for(CGroupConfigInfo* cGroupConfig: cGroupConfigs) {
        int32_t index = this->mFlatCGroupMap.size();
        this->mFlatCGroupMap[cGroupConfig->mCgroupID] = index;
    }

    /*
        Initialize the CocoTable, the table will contain a vector corresponding to each Resource from the ResourceTable
        For the Resource if there is no level of conflict (i.e. apply type is "global"), then a vector of size 4 will be
        allocated, where each of the 4 entry corresponds to a different priority, as Currently 4 priority levels are supported (SH / SL and TPH / TPL)
        In case of Conflicts:
        - For a Resource with Core Level Conflict (i.e. apply type is "core") a vector of size: 4 * NUMBER_OF_CORES will be created
        - For a Resource with Cluster Level Conflict (i.e. apply type is "cluster") a vector of size: 4 * NUMBER_OF_CLUSTER will be created
        - For a Resource with CGroup Level Conflict (i.e. apply type is "cgroup") a vector of size: 4 * NUMBER_OF_CGROUPS_CREATED will be created

        How the CocoTable (Vector) will look:
        - Say we have 5 resources R1, R2, R3, R4, R5
        - Where R3 has core level conflict
        - R4 has cluster level conflict
        - R5 has cgroup level conflict
        - R1 and R2 have no conflict
        - Say core count is 4, cluster count is 4 and cgroup count is 2
        - P1, P2, P3 and P4 represent the priorities
        [
            R1: [P1: [], P2: [], P3: [], P4: []],                                       # size of vector: 4
            R2: [P1: [], P2: [], P3: [], P4: []],                                       # size of vector: 4
            R3: [                                                                       # size of vector: 4 * 4 = 16
                Core1_P1: [], Core1_P2: [], Core1_P3: [], Core1_P4: [],
                Core2_P1: [], Core2_P2: [], Core2_P3: [], Core2_P4: [],
                Core3_P1: [], Core3_P2: [], Core3_P3: [], Core3_P4: [],
                Core4_P1: [], Core4_P2: [], Core4_P3: [], Core4_P4: []
                ],
            R4: [                                                                       # size of vector: 4 * 4 = 16
                Cluster1_P1: [], Cluster1_P2: [], Cluster1_P3: [], Cluster1_P4: [],
                Cluster2_P1: [], Cluster2_P2: [], Cluster2_P3: [], Cluster2_P4: [],
                Cluster3_P1: [], Cluster3_P2: [], Cluster3_P3: [], Cluster3_P4: [],
                Cluster4_P1: [], Cluster4_P2: [], Cluster4_P3: [], Cluster4_P4: []
                ],
            R5: [                                                                       # size of vector: 4 * 2 = 8
                Cgroup1_P1: [], Cgroup1_P2: [], Cgroup1_P3: [], Cgroup1_P4: [],
                Cgroup2_P1: [], Cgroup2_P2: [], Cgroup2_P3: [], Cgroup2_P4: []
                ]
        ]
    */
    for(ResConfInfo* resourceConfig: this->mResourceRegistry->getRegisteredResources()) {
        size_t vectorSize = TOTAL_PRIORITIES;
        if(resourceConfig->mApplyType == ResourceApplyType::APPLY_CORE) {
            int32_t totalCoreCount = UrmSettings::targetConfigs.mTotalCoreCount;
            vectorSize = TOTAL_PRIORITIES * totalCoreCount;

        } else if(resourceConfig->mApplyType == ResourceApplyType::APPLY_CLUSTER) {
            int32_t totalClusterCount = UrmSettings::targetConfigs.mTotalClusterCount;
            vectorSize = TOTAL_PRIORITIES * totalClusterCount;

        } else if(resourceConfig->mApplyType == ResourceApplyType::APPLY_CGROUP) {
            int32_t totalCGroupCount = TargetRegistry::getInstance()->getCreatedCGroupsCount();
            vectorSize = TOTAL_PRIORITIES * totalCGroupCount;

        } else if(resourceConfig->mApplyType == ResourceApplyType::APPLY_GLOBAL) {
            vectorSize = TOTAL_PRIORITIES;
        }

        std::vector<DLManager*> innerVec(vectorSize, nullptr);
        for(size_t i = 0; i < vectorSize; i++) {
            innerVec[i] = new DLManager(COCO_TABLE_DL_NR);
        }
        this->mCocoTable.push_back(innerVec);
    }
}

int8_t CocoTable::needsAllocation(Resource* res) {
    ResConfInfo* rConf = this->mResourceRegistry->getResConf(res->getResCode());
    return (rConf->mPolicy != Policy::PASS_THROUGH) &&
           (rConf->mPolicy != Policy::PASS_THROUGH_APPEND);
}

void CocoTable::applyAction(ResIterable* currNode, int32_t index, int8_t priority) {
    if(currNode == nullptr || currNode->mData == nullptr) return;

    Resource* resource = (Resource*) currNode->mData;

    if(this->mCurrentlyAppliedPriority[index] >= priority ||
       this->mCurrentlyAppliedPriority[index] == -1) {
        ResConfInfo* resourceConfig = this->mResourceRegistry->getResConf(resource->getResCode());
        if(resourceConfig->mModes & UrmSettings::targetConfigs.currMode) {
            // Check if a custom Applier (Callback) has been provided for this Resource, if yes, then call it
            // Note for resources with multiple values, the BU will need to provide a custom applier, which provides
            // the aggregation / selection logic.
            if(resourceConfig->mResourceApplierCallback != nullptr) {
                resourceConfig->mResourceApplierCallback(resource);
            }
            this->mCurrentlyAppliedPriority[index] = priority;
        } else {
            TYPELOGV(NOTIFY_RESMODE_REJECT, resource->getResCode(), UrmSettings::targetConfigs.currMode);
        }
    }
}

void CocoTable::fastPathApply(Resource* resource) {
    ResConfInfo* rConf = this->mResourceRegistry->getResConf(resource->getResCode());
    if(rConf->mModes & UrmSettings::targetConfigs.currMode) {
        // Check if a custom Applier (Callback) has been provided for this Resource, if yes, then call it
        // Note for resources with multiple values, the BU will need to provide a custom applier, which provides
        // the aggregation / selection logic.
        if(rConf->mResourceApplierCallback != nullptr) {
            rConf->mResourceApplierCallback(resource);
        }
    }
}

void CocoTable::removeAction(int32_t index, Resource* resource) {
    if(resource == nullptr) return;
    ResConfInfo* resConfInfo = this->mResourceRegistry->getResConf(resource->getResCode());
    if(resConfInfo != nullptr) {
        if(resConfInfo->mResourceTearCallback != nullptr) {
            resConfInfo->mResourceTearCallback(resource);
        }
        this->mCurrentlyAppliedPriority[index] = -1;
    }
}

void CocoTable::fastPathReset(Resource* resource) {
    ResConfInfo* rConf = this->mResourceRegistry->getResConf(resource->getResCode());
    if(rConf->mResourceApplierCallback != nullptr) {
        rConf->mResourceTearCallback(resource);
    }
}

int32_t CocoTable::getCocoTablePrimaryIndex(uint32_t opId) {
    if(this->mResourceRegistry->getResConf(opId) == nullptr) {
        return -1;
    }

    return this->mResourceRegistry->getResourceTableIndex(opId);
}

int32_t CocoTable::getCocoTableSecondaryIndex(Resource* resource, int8_t priority) {
    if(this->mResourceRegistry->getResConf(resource->getResCode()) == nullptr) {
        return -1;
    }

    ResConfInfo* resConfInfo = this->mResourceRegistry->getResConf(resource->getResCode());

    if(resConfInfo->mApplyType == ResourceApplyType::APPLY_CORE) {
        int32_t physicalCore = resource->getCoreValue();
        return physicalCore * TOTAL_PRIORITIES + priority;

    } else if(resConfInfo->mApplyType == ResourceApplyType::APPLY_CLUSTER) {
        int32_t physicalCluster = resource->getClusterValue();
        if(physicalCluster == -1) return -1;
        int32_t index = this->mFlatClusterMap[physicalCluster];
        return index * TOTAL_PRIORITIES + priority;

    } else if(resConfInfo->mApplyType == ResourceApplyType::APPLY_CGROUP) {
        int32_t cGroupIdentifier = resource->getValueAt(0);
        if(cGroupIdentifier == -1) return -1;

        int32_t index = this->mFlatCGroupMap[cGroupIdentifier];
        return index * TOTAL_PRIORITIES + priority;

    } else if(resConfInfo->mApplyType == ResourceApplyType::APPLY_GLOBAL) {
        return priority;
    }

    return -1;
}

// Resource Level DLLs manipulation logic
// The request with the highest priority at the head of the linked list is applied.
int8_t CocoTable::insertInCocoTable(ResIterable* newNode, int8_t priority) {
    if(newNode == nullptr) return false;
    Resource* resource = (Resource*) newNode->mData;
    ResConfInfo* rConf = this->mResourceRegistry->getResConf(resource->getResCode());

    // Each Resource is already indexed into the CocoTable
    int32_t primaryIndex = this->getCocoTablePrimaryIndex(resource->getResCode());
    int32_t secondaryIndex = this->getCocoTableSecondaryIndex(resource, priority);

    if(primaryIndex < 0 || secondaryIndex < 0 ||
       primaryIndex >= (int32_t)this->mCocoTable.size() ||
       secondaryIndex >= (int32_t)this->mCocoTable[primaryIndex].size()) {
        TYPELOGV(INV_COCO_TBL_INDEX, resource->getResCode(), primaryIndex, secondaryIndex);
        return false;
    }

    enum Policy policy = this->mResourceRegistry->getResConf(resource->getResCode())->mPolicy;
    DLManager* dlm = this->mCocoTable[primaryIndex][secondaryIndex];

    // Unlikely
    if(dlm == nullptr) {
        return false;
    }

    if(!this->needsAllocation(resource)) {
        if(rConf->mPolicy == Policy::PASS_THROUGH) {
            // Special handling for resources with policy: "pass_through"
            dlm->mRank++;
        }
        // straightaway apply the action
        this->fastPathApply(resource);
        return true;
    }

    switch(policy) {
        case INSTANT_APPLY: {
            // Insert this Request at the head of the linked list and apply it.
            if(RC_IS_OK(dlm->insert(newNode, DLOptions::INSERT_START))) {
                if(dlm->isNodeNth(0, newNode)) {
                    this->applyAction(newNode, primaryIndex, priority);
                }
            }
            break;
        }
        case HIGHER_BETTER: {
            // Insert the request in accordance with higher_is_better policy
            // If the request ends up at the head of the resource DLL, apply it
            if(RC_IS_OK(dlm->insertWithPolicy(newNode, comparHBetter))) {
                if(dlm->isNodeNth(0, newNode)) {
                    this->applyAction(newNode, primaryIndex, priority);
                }
            }
            break;
        }
        case LOWER_BETTER: {
            // Insert the request in accordance with lower_is_better policy
            // If the request ends up at the head of the resource DLL, apply it
            if(RC_IS_OK(dlm->insertWithPolicy(newNode, comparLBetter))) {
                if(dlm->isNodeNth(0, newNode)) {
                    this->applyAction(newNode, primaryIndex, priority);
                }
            }
            break;
        }
        case LAZY_APPLY: {
            // Insert the request at the end of the Resource DLL.
            if(RC_IS_OK(dlm->insert(newNode))) {
                if(dlm->isNodeNth(0, newNode)) {
                    this->applyAction(newNode, primaryIndex, priority);
                }
            }
            break;
        }
        default:
            return false;
    }

    return true;
}

// Insert a new Request into the CocoTable
// Note: This method is only called for Tune Requests
// As part of this Routine, we allocate CocoNodes for the Request.
// The count of CocoNodes allocated should be equal to the Number of Resources in the Request
// However, It is not guarenteed that all the CocoNodes get allocated due to potential
// Memory allocation failures, hence we resort to a best-effort approach, where in
// We allocate as many CocoNodes as possible for the Request.
int8_t CocoTable::insertRequest(Request* req) {
    if(req == nullptr) return false;
    TYPELOGV(NOTIFY_COCO_TABLE_INSERT_START, req->getHandle());

    // Create a time to associate with the request
    Timer* requestTimer = nullptr;
    req->setTimer(nullptr);

    // No timer allocation needed if the request duration is INF (-1).
    if(req->getDuration() != -1) {
        try {
            requestTimer = MPLACEV(Timer, std::bind(&CocoTable::timerExpired, this, req));
        } catch(const std::bad_alloc& e) {
            TYPELOGV(REQUEST_MEMORY_ALLOCATION_FAILURE_HANDLE, req->getHandle(), e.what());
            return false;
        }

        req->setTimer(requestTimer);
    }

    // Iterate over all the resources in the request and add them to the table.
    // Note the notion of "request being applied" refers to one or more of the resource configurations
    // part of the request being applied.
    if(req->getResDlMgr() == nullptr) {
        return false;
    }

    DL_ITERATE(req->getResDlMgr()) {
        // Expect ResIterable* iter to be provided by the macro
        if(iter == nullptr) continue;
        ResIterable* resIter = (ResIterable*) iter;
        this->insertInCocoTable(resIter, req->getPriority());
    }

    // Start the timer for this request
    if(req->getDuration() != -1 && requestTimer != nullptr) {
        if(!requestTimer->startTimer(req->getDuration())) {
            TYPELOGV(TIMER_START_FAILURE, req->getHandle());
            return false;
        }
    } else if(req->getDuration() != -1 && requestTimer == nullptr) {
        // Should not be hit, since bad-allocation check should have already handled this condition.
        TYPELOGV(TIMER_START_FAILURE, req->getHandle());
        return false;
    }

    return true;
}

int8_t CocoTable::updateRequest(Request* req, int64_t duration) {
    if(req == nullptr || duration < -1 || (duration > 0 && (duration < req->getDuration()))) {
        return false;
    }
    TYPELOGV(NOTIFY_COCO_TABLE_UPDATE_START, req->getHandle(), duration);

    // Update the duration of the request, and the corresponding timer interval.
    Timer* currTimer = req->getTimer();
    req->unsetTimer();
    currTimer->killTimer();
    FreeBlock<Timer>(static_cast<void*>(currTimer));

    req->setDuration(duration);
    // Create a time to associate with the request
    Timer* requestTimer = nullptr;
    try {
        requestTimer = MPLACEV(Timer, std::bind(&CocoTable::timerExpired, this, req));
    } catch(const std::bad_alloc& e) {
        TYPELOGV(REQUEST_MEMORY_ALLOCATION_FAILURE_HANDLE, req->getHandle(), e.what());
        return false;
    }

    req->setTimer(requestTimer);

    // Start the timer for this request
    if(!requestTimer->startTimer(req->getDuration())) {
        TYPELOGV(TIMER_START_FAILURE, req->getHandle());
        return false;
    }

    TYPELOGV(NOTIFY_COCO_TABLE_INSERT_SUCCESS, req->getHandle());
    return true;
}

// Methods for Request Cleanup
// Phase 1:
// Iterate over all the resources part of the request and un-configure them
// If the resource[i] is applied (i.e. at the head of its Resource's DLL), then check if there
// is any pending configurations (waiting behind it), if found apply it.
// If the resource[i] is somewhere in the middle (or end) of the DLL, then simply remove it and
// adjust the prev and next pointers accordingly.
// Phase 2:
// Actually freeing up the Request and its associated memory resources. This is not handled by
// CocoTable, instead RequestQueue is responsible for freeing up the Request and untracking it
// from the RequestManager.
int8_t CocoTable::removeRequest(Request* request) {
    TYPELOGV(NOTIFY_COCO_TABLE_REMOVAL_START, request->getHandle());

    if(request == nullptr || request->getResDlMgr() == nullptr) {
        // nothing to do
        return 0;
    }

    DL_ITERATE(request->getResDlMgr()) {
        if(iter == nullptr) continue;

        ResIterable* resIter = (ResIterable*) iter;
        if(resIter == nullptr || resIter->mData == nullptr) continue;

        Resource* resource = (Resource*) resIter->mData;

        int8_t priority = request->getPriority();
        int32_t primaryIndex = this->getCocoTablePrimaryIndex(resource->getResCode());
        int32_t secondaryIndex = this->getCocoTableSecondaryIndex(resource, priority);

        if(primaryIndex < 0 || secondaryIndex < 0 ||
           primaryIndex >= (int32_t)this->mCocoTable.size() ||
           secondaryIndex >= (int32_t)this->mCocoTable[primaryIndex].size()) {
            continue;
        }

        DLManager* dlm = this->mCocoTable[primaryIndex][secondaryIndex];
        if(dlm == nullptr) continue;

        ResConfInfo* resourceConfig = this->mResourceRegistry->getResConf(resource->getResCode());
        if(!this->needsAllocation(resource)) {
            if(resourceConfig->mPolicy == Policy::PASS_THROUGH) {
                if(--dlm->mRank == 0) {
                    this->fastPathReset(resource);
                }
            } else {
                this->fastPathReset(resource);
            }
            continue;
        }

        // Check if the node is currently at the head of it's resource DLL.
        int8_t nodeIsHead = dlm->isNodeNth(0, iter);

        // Proceed with removal of the node from CocoTable
        dlm->deleteNode(iter);

        // Reset the Resource Node value or if there are pending Requests for this Resource
        // then apply those Requests in the order determined by Resource Policy and Priority Level.

        // If the current list becomes empty, start from the highest priority
        // and look for available requests.
        // If all lists are empty, apply default action.
        if(dlm->mHead == nullptr) {
            int8_t allListsEmpty = true;
            int32_t reIndexIncrement = 0;

            if(resourceConfig->mApplyType == ResourceApplyType::APPLY_CORE ||
                resourceConfig->mApplyType == ResourceApplyType::APPLY_CLUSTER ||
                resourceConfig->mApplyType == ResourceApplyType::APPLY_CGROUP) {
                reIndexIncrement = this->getCocoTableSecondaryIndex(resource, SYSTEM_HIGH);
                if(reIndexIncrement == -1) continue;
            }

            for(int32_t prioLevel = 0; prioLevel < TOTAL_PRIORITIES; prioLevel++) {
                if(this->mCocoTable[primaryIndex][prioLevel + reIndexIncrement]->mHead != nullptr) {
                    this->mCurrentlyAppliedPriority[primaryIndex] = prioLevel;
                    this->applyAction(
                        static_cast<ResIterable*>(this->mCocoTable[primaryIndex][prioLevel + reIndexIncrement]->mHead),
                        primaryIndex,
                        prioLevel
                    );
                    allListsEmpty = false;
                    break;
                }
            }
            if(allListsEmpty == true) {
                this->removeAction(primaryIndex, resource);
            }

        } else {
            // Current list is not empty.
            // Check if current node is at the head
            if(nodeIsHead) {
                // If it is head, Apply the next node (i.e. the next Request)
                this->applyAction(static_cast<ResIterable*>(dlm->mHead), primaryIndex, priority);
            }
            // If node is not head, it implies some other Request is already applied
            // for this Resource, hence no action is needed here.
        }
    }
    return 0;
}

void CocoTable::timerExpired(Request* request) {
    TYPELOGV(NOTIFY_COCO_TABLE_REQUEST_EXPIRY, request->getHandle());

    Request* untuneRequest = nullptr;
    try {
        untuneRequest = MPLACED(Request);
    } catch(const std::bad_alloc& e) {
        return;
    }

    if(untuneRequest != nullptr) {
        request->populateUntuneRequest(untuneRequest);
        std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();
        requestQueue->addAndWakeup(untuneRequest);
    }
}

// CocoNodes allocated for the Request will be freed up as part of Request Cleanup,
// Use the Request::cleanUpRequest method, for freeing up these nodes.
CocoTable::~CocoTable() {
    for(int32_t i = 0; i < (int32_t)this->mCocoTable.size(); i++) {
        for(int32_t j = 0; j < (int32_t)this->mCocoTable[i].size(); j++) {
            delete(this->mCocoTable[i][j]);
            this->mCocoTable[i][j] = nullptr;
        }
    }
}
