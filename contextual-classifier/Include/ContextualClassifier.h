// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef CONTEXTUAL_CLASSIFIER_H
#define CONTEXTUAL_CLASSIFIER_H

#include "ComponentRegistry.h"
#include "NetLinkComm.h"
#include "AppConfigs.h"
#include "Resource.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Inference;

typedef enum : int8_t {
    CC_IGNORE = 0x00,
    CC_APP_OPEN = 0x01,
    CC_APP_CLOSE = 0x02
} EventType;

enum {
    CC_BROWSER_APP_OPEN = 0x03,
    CC_GAME_APP_OPEN = 0x04,
    CC_MULTIMEDIA_APP_OPEN = 0x05
};

enum {
    DEFAULT_CONFIG = 0,
    PER_APP_CONFIG
};

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

typedef struct {
    int64_t mCurHandle;
    pid_t mCurReqPid;
    pid_t mCurReqTid;
} RestuneHandleInfo;

class ContextualClassifier {
private:
    int8_t mDebugMode = false;
    volatile int8_t mNeedExit = false;

    NetLinkComm mNetLinkComm;
    Inference *mInference;
    std::vector<int64_t> mCurrRestuneHandles;

    // Event queue for classifier main thread
    std::queue<ProcEvent> mPendingEv;
    std::mutex mQueueMutex;
    std::condition_variable mQueueCond;
    std::thread mClassifierMain;
    std::thread mNetlinkThread;

    std::unordered_set<std::string> mIgnoredProcesses;
    std::unordered_set<std::string> mAllowedProcesses;

    void ClassifierMain();
    int32_t HandleProcEv();

    // Main classification flow
    int32_t ClassifyProcess(pid_t pid,
                            pid_t tgid,
                            const std::string &comm,
                            uint32_t &ctxDetails);

    // Fetch signal configuration info
    uint32_t GetSignalIDForWorkload(int32_t contextType);

    // Methods for tuning / untuning signals based on the workload
    void ApplyActions(uint32_t sigId,
                      uint32_t sigType,
                      pid_t incomingPID,
                      pid_t incomingTID);
    void RemoveActions(pid_t pid, int32_t tgid);

    // blacklisting mechanism
    void LoadIgnoredProcesses();
    int8_t shouldProcBeIgnored(int32_t evType, pid_t pid);

    // Transparently get the classification object, without expecting the client
    // to be aware of the underlying model / implementation.
    Inference* GetInferenceObject();

    // Methods to move the current process to focused-cgroup
    void MoveAppThreadsToCGroup(pid_t incomingPID,
                                pid_t incomingTID,
                                const std::string& comm,
                                int32_t cgroupIdentifier);

    void configureAssociatedAppSignals(pid_t incomingPID,
                                       pid_t incomingTID,
                                       const std::string& comm);

    void untuneRequestHelper(int64_t handle);

public:
    ContextualClassifier();
    ~ContextualClassifier();
    Inference *getInference() {
        return mInference;
    }

    ErrCode Init();
    ErrCode Terminate();
};

#endif // CONTEXTUAL_CLASSIFIER_H
