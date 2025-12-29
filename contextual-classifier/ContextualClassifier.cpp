// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Utils.h"
#include "Common.h"
#include "Request.h"
#include "Logger.h"
#include "AuxRoutines.h"
#include "ContextualClassifier.h"
#include "RestuneInternal.h"
#include "Extensions.h"
#include "Inference.h"
#include "UrmAPIs.h"

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <string>
#include <syslog.h>

#define CLASSIFIER_TAG "ContextualClassifier"
#define CLASSIFIER_CONF_DIR "/etc/classifier/"

const std::string FT_MODEL_PATH =
    CLASSIFIER_CONF_DIR "fasttext_model_supervised.bin";
const std::string IGNORE_PROC_PATH =
    CLASSIFIER_CONF_DIR "classifier-blocklist.txt";
const std::string IGNORE_TOKENS_PATH = CLASSIFIER_CONF_DIR "ignore-tokens.txt";

#ifdef USE_FASTTEXT
#include "MLInference.h"
#include "FeatureExtractor.h"
#include "FeaturePruner.h"
    //MLInference
Inference *ContextualClassifier::GetInferenceObject() {
	    return (new MLInference(FT_MODEL_PATH));
}
#else
    //inference
Inference *ContextualClassifier::GetInferenceObject() {
	    return (new Inference(FT_MODEL_PATH));
}
#endif

ContextualClassifier::ContextualClassifier() {
    mInference = GetInferenceObject();
}
// Global perf handle store, used across threads
static std::unordered_map<int, int> g_pid_perf_handle;

ContextualClassifier::~ContextualClassifier() {
    Terminate();
    if (mInference) {
		delete mInference;
        mInference = NULL;
	}
}

ErrCode ContextualClassifier::Init() {
    LOGI(CLASSIFIER_TAG, "Classifier module init.");

    // Record PID and TID
    mOurPid = getpid();
    mOurTid = gettid();
	
    LoadIgnoredProcesses();

    // Single worker thread for classification
    mClassifierMain = std::thread(&ContextualClassifier::ClassifierMain, this);

    if (mNetLinkComm.connect() == -1) {
        LOGE(CLASSIFIER_TAG, "Failed to connect to netlink socket.");
        return RC_SOCKET_OP_FAILURE;
    }

    if (mNetLinkComm.set_listen(true) == -1) {
        LOGE(CLASSIFIER_TAG, "Failed to set proc event listener.");
        mNetLinkComm.close_socket();
        return RC_SOCKET_OP_FAILURE;
    }
    LOGI(CLASSIFIER_TAG, "Now listening for process events.");

    mNetlinkThread = std::thread(&ContextualClassifier::HandleProcEv, this);

    return RC_SUCCESS;
}

ErrCode ContextualClassifier::Terminate() {
    LOGI(CLASSIFIER_TAG, "Classifier module terminate.");

    if (mNetLinkComm.get_socket() != -1) {
        mNetLinkComm.set_listen(false);
    }

    {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        mNeedExit = true;
        // Clear any pending PIDs so the worker doesn't see stale entries
        while (!mPendingEv.empty()) {
            mPendingEv.pop();
        }
    }
    mQueueCond.notify_all();

    mNetLinkComm.close_socket();

    if (mNetlinkThread.joinable()) {
        mNetlinkThread.join();
    }

    if (mClassifierMain.joinable()) {
        mClassifierMain.join();
    }

    return RC_SUCCESS;
}

void ContextualClassifier::ClassifierMain() {
    pthread_setname_np(pthread_self(), "uRMClassifierMain");
    while (true) {
        ProcEvent ev{};
        {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            mQueueCond.wait(
                lock, [this] { return !mPendingEv.empty() || mNeedExit; });

            if (mNeedExit) {
                return;
            }

            ev = mPendingEv.front();
            mPendingEv.pop();
        }

        if (ev.type == CC_APP_OPEN) {
            std::string comm;
            uint32_t sigId = CC_APP_OPEN;
            uint32_t sigSubtype = DEFAULT_CONFIG;
            uint32_t ctxDetails = 0U;

            if (ev.pid != -1) {
                if (FetchComm(ev.pid, comm) != 0) {
                    continue;
                }

                // Step 1: Figure out workload type
                int contextType =
                    ClassifyProcess(ev.pid, ev.tgid, comm, ctxDetails);
				if (contextType == CC_IGNORE) {
					//ignore and wait for next event
					continue;
				}
                // Identify if any signal configuration exists
                // Will return the sigID based on the workload
                // For example: game, browser, multimedia
                GetSignalDetailsForWorkload(contextType, sigId, sigSubtype);

                // Step 2: Move focused app "threads" + any threads from per-app config
                // to appropriate cgroups
                // Step 1: Move the process to focused-cgroup
                // Also involves removing the process already there from focused.
                const int perf_handle = move_pid_to_cgroup(ev.pid, CAMERA_C_GROUP_ID);

                // Step 2: Move the "threads" from per-app config to appropriate cgroups

                // Code for determining per-app config threads
                AppConfig* appConfig =
                    AppConfigs::getInstance()->getAppConfig(comm);

                if(appConfig != nullptr) {
                    int32_t threadsCount = appConfig->mNumThreads;
                    if(threadsCount > 0) {
                        MoveAppThreadsToCGroup(appConfig);
                    }
                }

                //Step 3: If the post processing block exists, call it
                // It might provide us a more specific sigSubtype
                PostProcessingCallback postCb =
                    Extensions::getPostProcessingCallback(comm);
                if(postCb) {
                    PostProcessCBData postProcessData = {
                        .mPid = ev.pid,
                        .mSigId = sigId,
                        .mSigSubtype = sigSubtype,
                    };
                    postCb((void*)&postProcessData);

                    sigId = postProcessData.mSigId;
                    sigSubtype = postProcessData.mSigSubtype;
                }

                //Step 4: Apply actions, call tuneSignal
                ApplyActions(comm, sigId, sigSubtype);
            }
        } else if (ev.type == CC_APP_CLOSE) {
			//Step1: move process to original cgroup

			//Step2: remove actions, call untune signal
            RemoveActions(ev.pid, ev.tgid);
        }
    }
}

/* Add process_pid to focus cgroup
Create a resource list with the cgroup id and process pid
This is a temporary solution to pass the cgroup id and process pid to the resource manager.
In future, we will have a proper resource management system. 
For now, we are using a simple approach to move the process to the cgroup.
The resource list will be passed to the resource manager to create the cgroup and add the process to it. */
int ContextualClassifier::move_pid_to_cgroup(int process_pid, int cgroupId)
{
    
    if(g_pid_perf_handle.find(process_pid) != g_pid_perf_handle.end())
    {
        syslog(LOG_DEBUG, "PID %d already in focus cgroup", process_pid);
        return g_pid_perf_handle[process_pid];
    }

    // Store the new handle for this process
    // g_pid_perf_handle[process_pid] = perf_handle;
    // If there was a previous handle, remove it
    // int prev_handle = g_pid_perf_handle[process_pid]; 
    // Optional: untune previous handle if API requires it
    // untuneResources(g_pid_perf_handle[prev_handle]);  

    // TODO: Make this dynamic based on number of cgroups available.
    // TODO: Add error handling for when cgroup creation fails.
    
    // Hardcoded 1 as we are using only one cgroup for now. 
    SysResource* resourceList = new SysResource[1];
    // memset OR: resourceList[0] = SysResource{};
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = RES_CGRP_MOVE_PID;
    resourceList[0].mResInfo = 0x00000000;
    // First value represents the cgroup id and rest of the values represent the pids which needs to be added in the cgroup
    int32_t* resourceVec = new int32_t[2];
    resourceVec[0] = cgroupId;
    resourceVec[1] = process_pid;
    resourceList[0].mNumValues = 2;
    // TODO: Which approach to copy values is better?
    // resourceList[0].mResValue.values = resourceVec;
    // TODO: Do we need to delete resourceList[0].mResValue.values after use?
    resourceList[0].mResValue.values = new int32_t[resourceList[0].mNumValues];
    for(int32_t k = 0; k < resourceList[0].mNumValues; k++) {
        resourceList[0].mResValue.values[k] = resourceVec[k];
    }
    
    int32_t properties = 0;
    // Set the Priority as High
    properties = SET_REQUEST_PRIORITY(properties, REQ_PRIORITY_HIGH);

    const int perf_handle = tuneResources(-1, properties, 1, resourceList);

    // Clean up temporaries
    delete[] resourceVec;
    delete[] resourceList;

    if (perf_handle < 0) {
        syslog(LOG_WARNING, "PID %d: move_pid_to_cgroup failed", process_pid);
        return perf_handle;
    } 
    
    g_pid_perf_handle[process_pid] = perf_handle;
    syslog(LOG_DEBUG, "Stored perf handle=%d for PID %d", perf_handle, process_pid);
    return perf_handle;
}

int ContextualClassifier::HandleProcEv() {
    pthread_setname_np(pthread_self(), "ClassNetlink");
    int rc = 0;

    while (!mNeedExit) {
        ProcEvent ev{};
        rc = mNetLinkComm.RecvEvent(ev);
        if (rc == CC_IGNORE) {
            continue;
        } else if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (mNeedExit) {
                return 0;
            }
            LOGE(CLASSIFIER_TAG, "netlink recv error");
            return -1;
        }

        switch (rc) {
        case CC_APP_OPEN:
            LOGD(CLASSIFIER_TAG,
                 "Received CC_APP_OPEN for pid=%d" + std::to_string(ev.pid));
            if (isIgnoredProcess(ev.type, ev.pid)) {
                break;
            }
            {
                std::lock_guard<std::mutex> lock(mQueueMutex);
                mPendingEv.push(ev);
                mQueueCond.notify_one();
            }
            break;
        case CC_APP_CLOSE:
            LOGD(CLASSIFIER_TAG,
                 "Received CC_APP_CLOSE pid=%d tgid=%d" + std::to_string(ev.pid));
            if (isIgnoredProcess(ev.type, ev.pid)) {
                break;
            }
            {
                std::lock_guard<std::mutex> lock(mQueueMutex);
                mPendingEv.push(ev);
                mQueueCond.notify_one();
            }
            break;
        default:
            LOGW(CLASSIFIER_TAG, "unhandled proc event");
            break;
        }
    }
    return 0;
}

int32_t ContextualClassifier::ClassifyProcess(pid_t process_pid, pid_t process_tgid,
                                          const std::string &comm,
                                          uint32_t &ctxDetails) {
    (void)process_tgid;
    (void)ctxDetails;
    CC_TYPE context = CC_APP;

    if (mIgnoredProcesses.count(comm) != 0U) {
        LOGD(CLASSIFIER_TAG,
             "Skipping inference for ignored process: "+ comm);
        return CC_IGNORE;
    }

    LOGD(CLASSIFIER_TAG,
         "Starting classification for PID: "+ std::to_string(process_pid));
    context = mInference->Classify(process_pid);
    return context;
}

void ContextualClassifier::ApplyActions(std::string comm, int32_t sigId,
                                        int32_t sigType) {
    (void)comm;
	(void)sigId;
	(void)sigType;
    // tuneSignal and update the handles
    // mResTunerHandles
    return;
}

void ContextualClassifier::RemoveActions(pid_t process_pid, pid_t process_tgid) {
	(void)process_pid;
    (void)process_tgid;
    // untuneSignal and erase handles
    // mResTunerHandles
    return;
}

void ContextualClassifier::GetSignalDetailsForWorkload(int32_t contextType,
                                                       uint32_t &sigId,
                                                       uint32_t &sigSubtype) {
    (void)sigSubtype;
    switch (contextType) {
    case CC_MULTIMEDIA:
        sigId = CC_MULTIMEDIA_APP_OPEN;
        break;
    case CC_GAME:
        sigId = CC_GAME_APP_OPEN;
        break;
    case CC_BROWSER:
        sigId = CC_BROWSER_APP_OPEN;
        break;
    }
    return;
}

void ContextualClassifier::LoadIgnoredProcesses() {
    std::ifstream file(IGNORE_PROC_PATH);
    if (!file.is_open()) {
        LOGW(CLASSIFIER_TAG,
             "Could not open ignore process file: "+IGNORE_PROC_PATH);
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string segment;
        while (std::getline(ss, segment, ',')) {
            size_t first = segment.find_first_not_of(" \t\n\r");
            if (first == std::string::npos)
                continue;
            size_t last = segment.find_last_not_of(" \t\n\r");
            segment = segment.substr(first, (last - first + 1));
            if (!segment.empty()) {
                mIgnoredProcesses.insert(segment);
            }
        }
    }
    LOGI(CLASSIFIER_TAG, "Loaded ignored processes.");
}

bool ContextualClassifier::isIgnoredProcess(int32_t evType, pid_t pid) {
    bool ignore = false;

    // For context close, see if pid is in ignored list and remove it.
    if (evType == CC_APP_CLOSE) {
        auto it = mIgnoredPids.find(pid);
        if (it != mIgnoredPids.end()) {
            mIgnoredPids.erase(it);
            ignore = true;
        }
        return ignore;
    }

    // For context open, check if comm is in ignored list and track pid.
    std::string comm_path = "/proc/" + std::to_string(pid) + "/comm";
    std::ifstream comm_file(comm_path);
    if (comm_file.is_open()) {
        std::string proc_name;
        std::getline(comm_file, proc_name);
        // Trim
        size_t first = proc_name.find_first_not_of(" \t\n\r");
        if (first != std::string::npos) {
            size_t last = proc_name.find_last_not_of(" \t\n\r");
            proc_name = proc_name.substr(first, (last - first + 1));
        }
        if (mIgnoredProcesses.count(proc_name) != 0U) {
            LOGD(CLASSIFIER_TAG, "Ignoring process: "+proc_name);
            mIgnoredPids.insert(pid);
            ignore = true;
        }
    } else {
        LOGD(CLASSIFIER_TAG,
             "Process " + std::to_string(pid) + " exited before initial check. Skipping.");
    }

    return ignore;
}

int32_t ContextualClassifier::FetchComm(pid_t pid, std::string &comm) {
    std::string proc_path = "/proc/" + std::to_string(pid);
    if (access(proc_path.c_str(), F_OK) == -1) {
        LOGD(CLASSIFIER_TAG, "Process %d has exited." + std::to_string(pid));
        return -1;
    }

    std::string comm_path = "/proc/" + std::to_string(pid) + "/comm";
    std::ifstream comm_file(comm_path);
    if (comm_file.is_open()) {
        std::getline(comm_file, comm);
        // Trim
        size_t first = comm.find_first_not_of(" \t\n\r");
        if (first != std::string::npos) {
            size_t last = comm.find_last_not_of(" \t\n\r");
            comm = comm.substr(first, (last - first + 1));
        }
    }
    return 0;
}

// Function to get the first matching PID for a given process name
pid_t ContextualClassifier::FetchPid(const std::string& process_name) {
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        std::cerr << "Failed to open /proc directory." << std::endl;
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type == DT_DIR && IsNumericString(entry->d_name)) {
            std::string pid_str = entry->d_name;
            std::string comm_path = "/proc/" + pid_str + "/comm";
            std::ifstream comm_file(comm_path);
            std::string comm;
            if (comm_file) {
                std::getline(comm_file, comm);
                if (comm.find(process_name) != std::string::npos) {
                    closedir(proc_dir);
                    return static_cast<pid_t>(std::stoi(pid_str));
                }
            }
        }
    }

    closedir(proc_dir);
    return -1; // Not found
}

// Helper to check if a string contains only digits
bool ContextualClassifier::IsNumericString(const std::string& str) {
    return std::all_of(str.begin(), str.end(), ::isdigit);
}

// ALERT !!!!!
// This will flood CocoTable and RequestManager (tracker) up very very quickly
// Need to think of some better approach rather than arbitrarily issuing all
// requests with an INF duration (-1).
void ContextualClassifier::MoveAppThreadsToCGroup(AppConfig* appConfig) {
    try {
        Request* request = MPLACED(Request);
        request->setRequestType(REQ_RESOURCE_TUNING);
        request->setHandle(AuxRoutines::generateUniqueHandle());
        request->setDuration(-1);
        request->setProperties(RequestPriority::REQ_PRIORITY_HIGH);
        request->setClientPID(mOurPid);
        request->setClientTID(mOurTid);

        if(appConfig->mThreadNameList != nullptr) {
            int32_t numThreads = appConfig->mNumThreads;
            // Go over the list of proc names (comm) and get their pids
            for(int32_t i = 0; i < numThreads; i++) {
                std::string targetComm = appConfig->mThreadNameList[i];
                pid_t targetPID = FetchPid(targetComm);
                if(targetPID != -1) {
                    // Get the CGroup
                    int32_t currCGroupID = appConfig->mCGroupIds[i];
                    // Make the move via Resource Tuner APIs

                    Resource* resource = MPLACEV(Resource);
                    resource->setResCode(RES_CGRP_MOVE_PID);
                    resource->setNumValues(2);
                    resource->setValueAt(0, currCGroupID);
                    resource->setValueAt(1, targetPID);

                    ResIterable* resIterable = MPLACED(ResIterable);
                    resIterable->mData = resource;
                    request->addResource(resIterable);
                }
            }
        }

        // Anything to issue
        if(request->getResourcesCount() > 0) {
            // fast path to Request Queue
            submitResProvisionRequest(request, true);
        } else {
            Request::cleanUpRequest(request);
        }

    } catch(const std::exception& e) {
        LOGE("CLASSIFIER",
             "Failed to move per-app threads to cgroup, Error: " + std::string(e.what()));
    }
}

static ContextualClassifier *g_classifier = nullptr;

// Public C interface exported from the contextual-classifier shared library.
// These are what the URM module entrypoints will call.
extern "C" ErrCode cc_init(void) {
    if (!g_classifier) {
        g_classifier = new ContextualClassifier();
    }
    return g_classifier->Init();
}

extern "C" ErrCode cc_terminate(void) {
    if (g_classifier) {
        ErrCode rc = g_classifier->Terminate();
        delete g_classifier;
        g_classifier = nullptr;
        return rc;
    }
    return RC_SUCCESS;
}
