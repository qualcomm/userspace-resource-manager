// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONTEXTUAL_CLASSIFIER_H
#define CONTEXTUAL_CLASSIFIER_H

#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>

#include "Resource.h"
#include "AppConfigs.h"
#include "NetLinkComm.h"
#include "AuxRoutines.h"
#include "ComponentRegistry.h"

class Inference;

typedef enum : int8_t {
    CC_IGNORE = 0x00,
    CC_APP_OPEN = 0x01,
    CC_APP_CLOSE = 0x02
} EventType;

typedef enum CC_TYPE {
    CC_APP = 0x01,
    CC_BROWSER = 0x02,
    CC_GAME = 0x03,
    CC_MULTIMEDIA = 0x04,
} CC_TYPE;

struct ProcEvent {
    int32_t pid;
    int32_t tgid;
    int32_t type; // CC_APP_OPEN / CC_APP_CLOSE / CC_IGNORE
};

class ContextualClassifier {
private:
    int8_t mDebugMode = false;
    volatile int8_t mNeedExit = false;

    int8_t mIsPriorityClient;
    uint64_t mClientTracker;
    uint64_t mActiveClientCount;
    uint64_t mActiveAppThreshold;

    NetLinkComm mNetLinkComm;
    Inference* mInference;
    std::queue<std::pair<uint64_t, int64_t>> mCurrRestuneHandles;

    // PID cache to check for duplicates
    MinLRUCache mClassifierPidCache;

    // Event queue for classifier main thread
    std::queue<ProcEvent> mPendingEv;
    std::mutex mQueueMutex;
    std::condition_variable mQueueCond;
    std::thread mClassifierMain;
    std::thread mNetlinkThread;

    std::unordered_set<std::string> mIgnoredProcesses;
    std::unordered_set<std::string> mAllowedProcesses;
    std::unordered_set<std::string> mPriorityClients;

    void ClassifierMain();
    int32_t HandleProcEv();

    // Main classification flow
    int32_t ClassifyProcess(pid_t pid,
                            pid_t tgid,
                            const std::string &comm,
                            uint32_t &ctxDetails);

    // Fetch signal configuration info
    uint32_t GetSignalIDForWorkload(int32_t contextType);

    // Methods for loading static proc data (allowlist, blocklist, priorities)
    void loadProcessNames(const std::string& fileName,
                          std::unordered_set<std::string>& names);
    void loadStaticProcData();
    int8_t checkClientPriority(const std::string& procName);
    int8_t shouldProcBeIgnored(int32_t evType, pid_t pid);

    // Returns true if the process lives inside a container cgroup scope
    // (e.g. docker-<CID>.scope), indicating it should not be moved by URM.
    int8_t isContainerProcess(pid_t pid);

    // Transparently get the classification object, without expecting the client
    // to be aware of the underlying model / implementation.
    Inference* GetInferenceObject();

    // Methods to move the current process to focused-cgroup
    void MoveAppThreadsToCGroup(pid_t incomingPID,
                                pid_t incomingTID,
                                const std::string& comm,
                                int32_t cgroupIdentifier);

    void configureAppSignals(pid_t incomingPID,
                             pid_t incomingTID,
                             const std::string& comm);

    void untuneRequestHelper(int64_t handle);

    void reShuffle();
    void trackClient(int64_t handle);

public:
    ContextualClassifier();
    ~ContextualClassifier();

    ErrCode Init();
    ErrCode Terminate();
};

#endif // CONTEXTUAL_CLASSIFIER_H
