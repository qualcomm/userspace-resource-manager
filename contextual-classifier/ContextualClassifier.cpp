// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include <chrono>
#include <string>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <algorithm>
#include <pthread.h>

#include "Utils.h"
#include "Config.h"
#include "Logger.h"
#include "Request.h"
#include "Inference.h"
#include "Extensions.h"
#include "AuxRoutines.h"
#include "UrmPlatformAL.h"
#include "SignalInternal.h"
#include "SignalRegistry.h"
#include "RestuneInternal.h"
#include "ContextualClassifier.h"
#include "ClientGarbageCollector.h"

#define CLASSIFIER_TAG "CONTEXTUAL_CLASSIFIER"
#define CLASSIFIER_CONFIGS_DIR URM_CLASSIFIER_DIR

static const std::string FT_MODEL_PATH =
    CLASSIFIER_CONFIGS_DIR "floret_model_supervised.bin";
static const std::string IGNORE_PROC_PATH =
    CLASSIFIER_CONFIGS_DIR "classifier-blocklist.txt";
static const std::string ALLOW_LIST_PATH =
    CLASSIFIER_CONFIGS_DIR "allow-list.txt";
static const std::string PRIORITY_CLIENTS_LIST_PATH =
    CLASSIFIER_CONFIGS_DIR "priority-clients.txt";

#ifdef USE_FLORET
#include "MLInference.h"
#include "FeatureExtractor.h"
#include "FeaturePruner.h"

// MLInference
Inference *ContextualClassifier::GetInferenceObject() {
    return (new MLInference(FT_MODEL_PATH));
}
#else

// Generic / Stub Inference Object
Inference *ContextualClassifier::GetInferenceObject() {
    return (new Inference(FT_MODEL_PATH));
}
#endif

static ContextualClassifier *gClassifier = nullptr;
static const int32_t pendingQueueControlSize = 30;

ContextualClassifier::ContextualClassifier() {
    this->mClientTracker = 0;
    this->mActiveClientCount = 0;
    this->mIsPriorityClient = false;
    this->mActiveAppThreshold = 1; // single-app mode

    // In multi-app mode, at most 3 concurrent apps are supported
    if(UrmSettings::metaConfigs.mAcceptMode & ACCEPT_AND_PERSIST) {
        this->mActiveAppThreshold = 3;
    }
    this->mInference = GetInferenceObject();
}

void ContextualClassifier::untuneRequestHelper(int64_t handle) {
    try {
        Request* untuneRequest = MPLACED(Request);

        untuneRequest->setRequestType(REQ_RESOURCE_UNTUNING);
        untuneRequest->setHandle(handle);
        untuneRequest->setDuration(-1);
        // Passing priority as HIGH_TRANSFER_PRIORITY (= -1)
        // - Ensures untune requests are processed before even SERVER_HIGH priority tune requests
        //   which helps in freeing memory
        // - Since this level of priority is only used internally, hence it has been customized to
        //   not free up the underlying Request object, allowing for reuse.
        // Priority Level: -2 is used to force server termination and cleanup so should not be used otherwise.
        untuneRequest->setPriority(SYSTEM_HIGH);
        untuneRequest->setClientPID(0);
        untuneRequest->setClientTID(0);

        // fast path to Request Queue
        // Mark verification status as true. Request still goes through RequestManager though.
        submitResProvisionRequest(untuneRequest, true);

    } catch(const std::exception& e) {
        LOGE(CLASSIFIER_TAG,
             "Failed to create untune request, Error: " + std::string(e.what()));
    }
}

static ResIterable* createMovePidResource(int32_t cGroupdId, pid_t pid) {
    ResIterable* resIterable = MPLACED(ResIterable);
    Resource* resource = MPLACED(Resource);
    resource->setResCode(RES_CGRP_MOVE_PID);
    resource->setNumValues(2);
    resource->setValueAt(0, cGroupdId);
    resource->setValueAt(1, pid);

    resIterable->mData = resource;
    return resIterable;
}

ContextualClassifier::~ContextualClassifier() {
    this->Terminate();
    if(this->mInference) {
        delete this->mInference;
        this->mInference = NULL;
    }
}

ErrCode ContextualClassifier::Init() {
    LOGI(CLASSIFIER_TAG, "Classifier module init");

    this->loadStaticProcData();

    // Single worker thread for classification
    this->mClassifierMain = std::thread(&ContextualClassifier::ClassifierMain, this);

    if(this->mNetLinkComm.connect() == -1) {
        LOGE(CLASSIFIER_TAG, "Failed to connect to netlink socket");
        return RC_SOCKET_OP_FAILURE;
    }

    if(this->mNetLinkComm.setListen(true) == -1) {
        LOGE(CLASSIFIER_TAG, "Failed to set proc event listener");
        mNetLinkComm.closeSocket();
        return RC_SOCKET_OP_FAILURE;
    }
    LOGI(CLASSIFIER_TAG, "Now listening for process events");

    this->mNetlinkThread = std::thread(&ContextualClassifier::HandleProcEv, this);
    return RC_SUCCESS;
}

ErrCode ContextualClassifier::Terminate() {
    LOGI(CLASSIFIER_TAG, "Classifier module terminate.");

    if(this->mNetLinkComm.getSocket() != -1) {
        this->mNetLinkComm.setListen(false);
    }

    std::unique_lock<std::mutex> lock(this->mQueueMutex);
    this->mNeedExit = true;
    // Clear any pending PIDs so the worker doesn't see stale entries
    while(!this->mPendingEv.empty()) {
        this->mPendingEv.pop();
    }
    this->mQueueCond.notify_all();

    this->mNetLinkComm.closeSocket();

    if(this->mNetlinkThread.joinable()) {
        this->mNetlinkThread.join();
    }

    if(this->mClassifierMain.joinable()) {
        this->mClassifierMain.join();
    }

    return RC_SUCCESS;
}

void ContextualClassifier::trackClient(int64_t handle) {
    if(this->mIsPriorityClient) {
        // Priority clients are not tracked here, they are long-living
        // and should not be evicted unless they die.
        // Pulse Monitor takes care of eviction for such clients.
        return;
    }

    if(handle != -1) {
        this->mCurrRestuneHandles.push(
            {this->mClientTracker, handle}
        );
    }
}

void ContextualClassifier::reShuffle() {
    if(this->mCurrRestuneHandles.empty()) {
        this->mActiveClientCount = 0;
    } else if(this->mActiveClientCount >= this->mActiveAppThreshold &&
        !this->mCurrRestuneHandles.empty()) {
        uint64_t minClientId = this->mCurrRestuneHandles.front().first;
        int8_t evictedClient = false;
        while(!this->mCurrRestuneHandles.empty()) {
            if(minClientId == this->mCurrRestuneHandles.front().first) {
                int64_t handle = this->mCurrRestuneHandles.front().second;
                if(handle > 0) {
                    this->untuneRequestHelper(handle);
                }
                this->mCurrRestuneHandles.pop();
                evictedClient = true;
            } else {
                break;
            }
        }

        if(evictedClient && this->mActiveClientCount > 0) {
            this->mActiveClientCount--;
        }
    }
}

void ContextualClassifier::ClassifierMain() {
    pthread_setname_np(pthread_self(), "urmClassifier");
    while (true) {
        ProcEvent ev{};

        std::unique_lock<std::mutex> lock(this->mQueueMutex);
        this->mQueueCond.wait(
            lock,
            [this] {
                return !this->mPendingEv.empty() || this->mNeedExit;
            }
        );

        if(this->mNeedExit) {
            return;
        }

        ev = this->mPendingEv.front();
        this->mPendingEv.pop();

        // Clear out mIsPriorityClient flag
        this->mIsPriorityClient = false;

        if(ev.type == CC_APP_OPEN) {
            if(ev.pid != -1) {
                std::string comm;
                uint32_t sigId = URM_SIG_APP_OPEN;
                uint32_t sigType = DEFAULT_SIGNAL_TYPE;
                uint32_t ctxDetails = 0U;

                if(AuxRoutines::fetchComm(ev.pid, comm) != 0) {
                    continue;
                }

                this->mIsPriorityClient = this->checkClientPriority(comm);

                // Step 1:
                // Keep active client count accurate. A client can own multiple
                // handles, so evict all handles for the oldest client together.
                if(!this->mIsPriorityClient) {
                    this->reShuffle();
                }

                size_t handlesBeforeClient = this->mCurrRestuneHandles.size();

                // Step 2:
                // - Move the process to focused-cgroup, Also involves removing the process
                //  already there from the cgroup.
                // - Move the "threads" from per-app config to appropriate cgroups
                this->MoveAppThreadsToCGroup(ev.pid, ev.tgid, comm, FOCUSED_CGROUP_IDENTIFIER);

                // Step 3:
                // Configure any per-app config specified signals.
                this->configureAppSignals(ev.pid, ev.tgid, comm);

                // Step 4: If the post processing block exists, call it
                // It might provide us a more specific sigID or sigType
                PostProcessingCallback postCb =
                    Extensions::getPostProcessingCallback(comm);
                if(postCb != nullptr) {
                    PostProcessCBData postProcessData = {
                        .mPid = ev.pid,
                        .mSigId = sigId,
                        .mSigType = sigType,
                        .mNumArgs = 0,
                        .mArgs = nullptr,
                        .mHandleAcq = -1,
                    };
                    postCb((void*)&postProcessData);

                    // Record any Configurations made
                    this->trackClient(postProcessData.mHandleAcq);

                } else {
                    // Figure out workload type
                    int32_t contextType =
                        this->ClassifyProcess(ev.pid, ev.tgid, comm, ctxDetails);
                    if(contextType != CC_IGNORE) {
                        // Identify if any signal configuration exists
                        // Will return the sigID based on the workload
                        // For example: game, browser, multimedia
                        sigId = this->GetSignalIDForWorkload(contextType);

                        int64_t handle = acquireSignal(
                            sigId,
                            sigType,
                            ev.pid,
                            ev.tgid
                        );
                        this->trackClient(handle);
                    }
                }

                if(this->mCurrRestuneHandles.size() > handlesBeforeClient) {
                    this->mActiveClientCount++;
                }
            }

            // Increment client count, this helps in tracking older Requests,
            // which will need to be untuned once the active threshold is hit.
            this->mClientTracker++;

        } else if(ev.type == CC_APP_CLOSE) {
            // No Action Needed, Pulse Monitor to take care of cleanup
            ClientGarbageCollector::getInstance()->submitClientForCleanup(ev.pid);
            ClientDataManager::getInstance()->deleteClientPID(ev.pid);
        }
    }
}

int32_t ContextualClassifier::HandleProcEv() {
    pthread_setname_np(pthread_self(), "urmNetlinkListener");
    int32_t rc = 0;

    while(!this->mNeedExit) {
        ProcEvent ev{};
        rc = mNetLinkComm.recvEvent(ev);
        if(rc == CC_IGNORE) {
            continue;
        }
        if(rc == -1) {
            if(errno == EINTR) {
                continue;
            }

            if(this->mNeedExit) {
                return 0;
            }

            // Error would have already been logged by this point.
            return -1;
        }

        // Process still up?
        if(!AuxRoutines::fileExists(COMM(ev.pid))) {
            continue;
        }

        switch(rc) {
            case CC_APP_OPEN:
                if(!this->shouldProcBeIgnored(ev.type, ev.pid)) {
                    const std::lock_guard<std::mutex> lock(mQueueMutex);
                    if(this->mClassifierPidCache.isPresent(ev.pid)) {
                        // Duplicate Notification, skip.
                        break;
                    }
                    this->mPendingEv.push(ev);
                    this->mClassifierPidCache.insert(ev.pid);
                    if(this->mPendingEv.size() > pendingQueueControlSize) {
                        this->mPendingEv.pop();
                    }
                    this->mQueueCond.notify_one();
                }

                break;

            case CC_APP_CLOSE:
                if(!this->shouldProcBeIgnored(ev.type, ev.pid)) {
                    const std::lock_guard<std::mutex> lock(mQueueMutex);
                    this->mPendingEv.push(ev);
                    if(this->mPendingEv.size() > pendingQueueControlSize) {
                        this->mPendingEv.pop();
                    }
                    this->mQueueCond.notify_one();
                }

                break;

            default:
                break;
        }
    }

    return 0;
}

int32_t ContextualClassifier::ClassifyProcess(pid_t processPid,
                                              pid_t processTgid,
                                              const std::string &comm,
                                              uint32_t &ctxDetails) {
    (void)processTgid;
    (void)ctxDetails;

    CC_TYPE context = CC_APP;

    // Check if the process is still alive
    if(!AuxRoutines::fileExists(COMM(processPid))) {
        LOGD(CLASSIFIER_TAG,
             "Skipping inference, process is dead: " + comm);
        return CC_IGNORE;
    }

    LOGD(CLASSIFIER_TAG,
         "Starting classification for PID: "
            + std::to_string(processPid)
            + ", " + comm
        );
    context = mInference->Classify(processPid);
    return context;
}

uint32_t ContextualClassifier::GetSignalIDForWorkload(int32_t contextType) {
    switch(contextType) {
        case CC_MULTIMEDIA:
            return URM_SIG_MULTIMEDIA_APP_OPEN;
        case CC_GAME:
            return URM_SIG_GAME_APP_OPEN;
        case CC_BROWSER:
            return URM_SIG_BROWSER_APP_OPEN;
        default:
            break;
    }

    // CC_APP
    return URM_SIG_APP_OPEN;
}

void ContextualClassifier::loadProcessNames(const std::string& filePath,
                                            std::unordered_set<std::string>& names) {
    std::ifstream file(filePath);
    if(!file.is_open()) {
        LOGW(CLASSIFIER_TAG,
             "Could not open process filter file: " + filePath);
        return;
    }

    std::string line;
    while(std::getline(file, line)) {
        std::stringstream ss(line);
        std::string segment;
        while(std::getline(ss, segment, ',')) {
            size_t first = segment.find_first_not_of(" \t\n\r");
            if(first == std::string::npos) {
                continue;
            }

            size_t last = segment.find_last_not_of(" \t\n\r");
            segment = segment.substr(first, (last - first + 1));
            if(!segment.empty()) {
                names.insert(segment);
            }
        }
    }
}

void ContextualClassifier::loadStaticProcData() {
    // First Load Allowed / Blacklisted processes
    if(AuxRoutines::fileExists(ALLOW_LIST_PATH)) {
        this->loadProcessNames(ALLOW_LIST_PATH, this->mAllowedProcesses);
    } else {
        this->loadProcessNames(IGNORE_PROC_PATH, this->mIgnoredProcesses);
    }

    // Next Load Priority (long-living) clients
    this->loadProcessNames(PRIORITY_CLIENTS_LIST_PATH, this->mPriorityClients);

    // All static proc data loaded
    LOGI(CLASSIFIER_TAG, "Successfully loaded static proc data");
}

int8_t ContextualClassifier::isContainerProcess(pid_t pid) {
    // Check if the process lives inside a container cgroup scope by reading
    // /proc/<pid>/cgroup. A Docker container init process has a path like:
    //   0::/system.slice/docker-<CID>.scope
    // Moving it out of this scope causes systemd to remove the now-empty scope,
    // which breaks subsequent docker exec calls with:
    //   "can't open cgroup: .../docker-<CID>.scope: No such file or directory"

    // TODO: extend to Kubernetes container runtimes when needed:
    //   - containerd (k8s default runtime): cgroup path contains "containerd-<CID>.scope"
    //   - CRI-O (OpenShift / k8s alternative): cgroup path contains "crio-<CID>.scope"

    std::ifstream cgroupFile("/proc/" + std::to_string(pid) + "/cgroup");
    if(cgroupFile.is_open()) {
        std::string line;
        while(std::getline(cgroupFile, line)) {
            if(line.find("docker-") != std::string::npos &&
               line.find(".scope")  != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

int8_t ContextualClassifier::shouldProcBeIgnored(int32_t evType, pid_t pid) {
    if(evType == CC_APP_CLOSE) {
        return false;
    }

    // Skip Docker container init processes to avoid breaking docker exec.
    if(this->isContainerProcess(pid)) {
        LOGI(CLASSIFIER_TAG,
             "Ignoring Docker container process PID: " + std::to_string(pid));
        return true;
    }

    std::string procName = "";
    if(AuxRoutines::fetchComm(pid, procName) != 0) {
        return true;
    }

    if(this->mAllowedProcesses.size() > 0) {
        // Check in allowed-list is present
        if(this->mAllowedProcesses.count(procName) == 0U) {
            return true;
        }
    } else {
        // If allow-list is not present, check in blocklist
        if(this->mIgnoredProcesses.count(procName) != 0U) {
            return true;
        }
    }

    return false;
}

int8_t ContextualClassifier::checkClientPriority(const std::string& procName) {
    return this->mPriorityClients.count(procName) != 0U;
}

void ContextualClassifier::MoveAppThreadsToCGroup(pid_t incomingPID,
                                                  pid_t incomingTID,
                                                  const std::string& comm,
                                                  int32_t cgroupIdentifier) {
    try {
        AppConfig* appConfig = AppConfigs::getInstance()->getAppConfig(comm);
        if(appConfig == nullptr || appConfig->isMovementAllowed == false) {
            return;
        }

        int64_t handleGenerated = -1;
        // Issue a tune request for the new pid (and any associated app-config pids)
        Request* request = MPLACED(Request);
        request->setRequestType(REQ_RESOURCE_TUNING);

        // Generate and store the handle for future use
        handleGenerated = AuxRoutines::generateUniqueHandle();
        request->setHandle(handleGenerated);
        request->setDuration(-1);
        request->setPriority(SYSTEM_LOW);
        request->setClientPID(incomingPID);
        request->setClientTID(incomingTID);

        // Move the incoming pid
        ResIterable* resIter = createMovePidResource(cgroupIdentifier, incomingPID);
        request->addResource(resIter);

        if(appConfig->mThreadNameList != nullptr) {
            int32_t numThreads = appConfig->mNumThreads;
            // Go over the list of proc names (comm) and get their pids
            for(int32_t i = 0; i < numThreads; i++) {
                std::string targetComm = appConfig->mThreadNameList[i];
                pid_t targetPID = AuxRoutines::fetchPid(targetComm);
                if(targetPID != -1) {
                    // Get the CGroup
                    int32_t targetCgrpId = appConfig->mCGroupIds[i];
                    request->addResource(createMovePidResource(targetCgrpId, targetPID));
                }
            }
        }

        // Anything to issue
        if(request->getResourcesCount() > 0) {
            // Record:
            this->trackClient(request->getHandle());

            // fast path to Request Queue
            submitResProvisionRequest(request, true);

        } else {
            Request::cleanUpRequest(request);
        }

    } catch(const std::exception& e) {
        LOGE(CLASSIFIER_TAG,
             "Failed to move per-app threads to cgroup, Error: " + std::string(e.what()));
    }
}

void ContextualClassifier::configureAppSignals(pid_t incomingPID,
                                               pid_t incomingTID,
                                               const std::string& comm) {
    // Configure any associated signal
    AppConfig* appConfig = AppConfigs::getInstance()->getAppConfig(comm);
    if(appConfig != nullptr && appConfig->mSignalCodes != nullptr) {
        // Go over the list of proc names (comm) and get their pids
        for(int32_t i = 0; i < appConfig->mNumSignals; i++) {
            int64_t handle = acquireSignal(
                appConfig->mSignalCodes[i],
                0,
                incomingPID,
                incomingTID
            );

            if(handle != -1) {
                this->trackClient(handle);
            }
        }
    }
}

// Public C interface exported from the contextual-classifier shared library.
// These are what the URM module entrypoints will call.
extern "C" ErrCode ccInit(void) {
    if(gClassifier == nullptr) {
        gClassifier = new ContextualClassifier();
    }
    return gClassifier->Init();
}

extern "C" ErrCode ccTerminate(void) {
    if(gClassifier != nullptr) {
        ErrCode opStatus = gClassifier->Terminate();
        delete gClassifier;
        gClassifier = nullptr;
        return opStatus;
    }
    return RC_SUCCESS;
}
