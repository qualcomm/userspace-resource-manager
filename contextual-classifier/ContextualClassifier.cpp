// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

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
#include "Logger.h"
#include "Request.h"
#include "Inference.h"
#include "Extensions.h"
#include "AuxRoutines.h"
#include "UrmPlatformAL.h"
#include "SignalRegistry.h"
#include "RestuneInternal.h"
#include "ContextualClassifier.h"
#include "ClientGarbageCollector.h"

#define CLASSIFIER_TAG "CONTEXTUAL_CLASSIFIER"
#define CLASSIFIER_CONFIGS_DIR "/etc/urm/classifier/"

static const std::string FT_MODEL_PATH =
    CLASSIFIER_CONFIGS_DIR "floret_model_supervised.bin";
static const std::string IGNORE_PROC_PATH =
    CLASSIFIER_CONFIGS_DIR "classifier-blocklist.txt";
static const std::string ALLOW_LIST_PATH =
    CLASSIFIER_CONFIGS_DIR "allow-list.txt";

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

static Request* createTuneRequestFromSignal(uint32_t sigId,
                                            uint32_t sigType,
                                            pid_t incomingPID,
                                            pid_t incomingTID) {
    try {
        std::shared_ptr<SignalRegistry> sigRegistry = SignalRegistry::getInstance();

        // Check if a Signal with the given ID exists in the Registry
        SignalInfo* signalInfo = sigRegistry->getSignalConfigById(sigId, sigType);

        if(signalInfo == nullptr) return nullptr;

        Request* request = MPLACED(Request);

        int64_t handleGenerated = AuxRoutines::generateUniqueHandle();
        request->setHandle(handleGenerated);

        request->setRequestType(REQ_RESOURCE_TUNING);
        request->setDuration(signalInfo->mTimeout);
        request->setProperties(SYSTEM_HIGH);
        request->setClientPID(incomingPID);
        request->setClientTID(incomingTID);

        std::vector<Resource*>* signalLocks = signalInfo->mSignalResources;

        for(int32_t i = 0; i < (int32_t)signalLocks->size(); i++) {
            if((*signalLocks)[i] == nullptr) {
                continue;
            }

            // Copy
            Resource* resource = MPLACEV(Resource, (*((*signalLocks)[i])));

            ResIterable* resIterable = MPLACED(ResIterable);
            resIterable->mData = resource;
            request->addResource(resIterable);
        }

        return request;

    } catch(const std::bad_alloc& e) {
        return nullptr;
    }

    return nullptr;
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

    this->LoadIgnoredProcesses();

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

        if(ev.type == CC_APP_OPEN) {
            std::string comm;
            uint32_t sigId = URM_SIG_APP_OPEN;
            uint32_t sigType = DEFAULT_SIGNAL_TYPE;
            uint32_t ctxDetails = 0U;

            if(ev.pid != -1) {
                if(AuxRoutines::fetchComm(ev.pid, comm) != 0) {
                    continue;
                }

                // Step 1: Figure out workload type
                int32_t contextType =
                    this->ClassifyProcess(ev.pid, ev.tgid, comm, ctxDetails);
				if(contextType == CC_IGNORE) {
					// Ignore and wait for next event
					continue;
				}

                // Identify if any signal configuration exists
                // Will return the sigID based on the workload
                // For example: game, browser, multimedia
                sigId = this->GetSignalIDForWorkload(contextType);

                // Step 2:
                // Untune any Configurations from the last proc-invocation
                for(int64_t handle: this->mCurrRestuneHandles) {
                    if(handle > 0) {
                        untuneRequestHelper(handle);
                    }
                }
                this->mCurrRestuneHandles.clear();

                // Step 3:
                // - Move the process to focused-cgroup, Also involves removing the process
                //  already there from the cgroup.
                // - Move the "threads" from per-app config to appropriate cgroups
                this->MoveAppThreadsToCGroup(ev.pid, ev.tgid, comm, FOCUSED_CGROUP_IDENTIFIER);

                // Step 4:
                // Configure any per-app config specified signals.
                this->configureAppSignals(ev.pid, ev.tgid, comm);

                // Step 5: If the post processing block exists, call it
                // It might provide us a more specific sigID or sigType
                PostProcessingCallback postCb =
                    Extensions::getPostProcessingCallback(comm);
                if(postCb != nullptr) {
                    PostProcessCBData postProcessData = {
                        .mPid = ev.pid,
                        .mSigId = sigId,
                        .mSigType = sigType,
                    };
                    postCb((void*)&postProcessData);

                    sigId = postProcessData.mSigId;
                    sigType = postProcessData.mSigType;
                }

                // Apply actions, call tuneSignal
                this->ApplyActions(sigId, sigType, ev.pid, ev.tgid);
            }
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

void ContextualClassifier::ApplyActions(uint32_t sigId,
                                        uint32_t sigType,
                                        pid_t incomingPID,
                                        pid_t incomingTID) {
    Request* request =
        createTuneRequestFromSignal(sigId, sigType, incomingPID, incomingTID);
    if(request != nullptr) {
        if(request->getResourcesCount() > 0) {
            // Record:
            this->mCurrRestuneHandles.push_back(request->getHandle());

            // fast path to Request Queue
            submitResProvisionRequest(request, true);

        } else {
            Request::cleanUpRequest(request);
        }
    }
}

void ContextualClassifier::RemoveActions(pid_t processPid, pid_t processTgid) {
	(void)processPid;
    (void)processTgid;

    return;
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

void ContextualClassifier::LoadIgnoredProcesses() {
    int8_t isAllowedListPresent = false;
    std::string filePath = ALLOW_LIST_PATH;
    if(AuxRoutines::fileExists(filePath)) {
        isAllowedListPresent = true;
    } else {
        filePath = IGNORE_PROC_PATH;
    }

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
                if(isAllowedListPresent) {
                    this->mAllowedProcesses.insert(segment);
                } else {
                    this->mIgnoredProcesses.insert(segment);
                }
            }
        }
    }
    LOGI(CLASSIFIER_TAG, "Loaded filter processes");
}

int8_t ContextualClassifier::shouldProcBeIgnored(int32_t evType, pid_t pid) {
    // For context close, see if pid is in ignored list and remove it.
    if(evType == CC_APP_CLOSE) {
        return false;
    }

    std::string procName = "";
    if(!AuxRoutines::getProcName(pid, procName)) {
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

void ContextualClassifier::MoveAppThreadsToCGroup(pid_t incomingPID,
                                                  pid_t incomingTID,
                                                  const std::string& comm,
                                                  int32_t cgroupIdentifier) {
    try {
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

        AppConfig* appConfig = AppConfigs::getInstance()->getAppConfig(comm);
        if(appConfig != nullptr && appConfig->mThreadNameList != nullptr) {
            int32_t numThreads = appConfig->mNumThreads;
            // Go over the list of proc names (comm) and get their pids
            for(int32_t i = 0; i < numThreads; i++) {
                std::string targetComm = appConfig->mThreadNameList[i];
                pid_t targetPID = AuxRoutines::fetchPid(targetComm);
                if(targetPID != -1 && targetPID != incomingPID) {
                    // Get the CGroup
                    int32_t currCGroupID = appConfig->mCGroupIds[i];
                    request->addResource(createMovePidResource(currCGroupID, targetPID));
                }
            }
        }

        // Anything to issue
        if(request->getResourcesCount() > 0) {
            // Record:
            this->mCurrRestuneHandles.push_back(request->getHandle());

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
    try {
        // Configure any associated signal
        AppConfig* appConfig = AppConfigs::getInstance()->getAppConfig(comm);
        if(appConfig != nullptr && appConfig->mSignalCodes != nullptr) {
            int32_t numSignals = appConfig->mNumSignals;
            // Go over the list of proc names (comm) and get their pids
            for(int32_t i = 0; i < numSignals; i++) {
                Request* request = createTuneRequestFromSignal(
                    appConfig->mSignalCodes[i],
                    0,
                    incomingPID,
                    incomingTID);

                if(request != nullptr) {
                    if(request->getResourcesCount() > 0) {
                        // fast path to Request Queue
                        this->mCurrRestuneHandles.push_back(request->getHandle());
                        submitResProvisionRequest(request, true);

                    } else {
                        Request::cleanUpRequest(request);
                    }
                }
            }
        }
    } catch(const std::exception& e) {
        LOGE(CLASSIFIER_TAG,
             "Failed to acquire per-app config signal, Error: " + std::string(e.what()));
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
