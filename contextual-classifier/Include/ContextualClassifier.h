// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef CONTEXTUAL_CLASSIFIER_H
#define CONTEXTUAL_CLASSIFIER_H

#include "ComponentRegistry.h"
#include "NetLinkComm.h"
#include "AppConfigs.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Inference;

enum { CC_IGNORE = 0x00, CC_APP_OPEN = 0x01, CC_APP_CLOSE = 0x02 };

enum {
    CC_BROWSER_APP_OPEN = 0x03,
    CC_GAME_APP_OPEN = 0x04,
    CC_MULTIMEDIA_APP_OPEN = 0x05
};

enum { DEFAULT_CONFIG, PER_APP_CONFIG };

typedef enum CC_TYPE {
    CC_APP = 0x01,
    CC_BROWSER = 0x02,
    CC_GAME = 0x03,
    CC_MULTIMEDIA = 0x04,
} CC_TYPE;

struct ProcEvent {
    int pid;
    int tgid;
    int type; // CC_APP_OPEN / CC_APP_CLOSE / CC_IGNORE
};

class ContextualClassifier {
  public:
    ContextualClassifier();
    ~ContextualClassifier();
    ErrCode Init();
    ErrCode Terminate();

  private:
    void ClassifierMain();
    int32_t HandleProcEv();

    int move_pid_to_cgroup(int process_pid, int cgroupId);

    int32_t ClassifyProcess(pid_t pid, pid_t tgid, const std::string &comm,
                        uint32_t &ctxDetails);
    void ApplyActions(std::string comm, int32_t sigId, int32_t sigType);
    void RemoveActions(pid_t pid, int tgid);

    Inference *GetInferenceObject();
	
	void GetSignalDetailsForWorkload(int32_t contextType, uint32_t &sigId,
                                     uint32_t &sigSubtype);

    void LoadIgnoredProcesses();
    bool isIgnoredProcess(int32_t evType, pid_t pid);

    int32_t FetchComm(pid_t pid, std::string &comm);
	pid_t FetchPid(const std::string& process_name);
	bool IsNumericString(const std::string& str);
    void MoveAppThreadsToCGroup(AppConfig* appConfig);

private:
    NetLinkComm mNetLinkComm;
    Inference *mInference;

    // Event queue for classifier main thread
    std::queue<ProcEvent> mPendingEv;
    std::mutex mQueueMutex;
    std::condition_variable mQueueCond;
    std::thread mClassifierMain;
    std::thread mNetlinkThread;
    volatile bool mNeedExit = false;

    std::unordered_set<std::string> mIgnoredProcesses;
    bool mDebugMode = false;

    std::unordered_set<pid_t> mIgnoredPids;
    std::unordered_map<pid_t, uint64_t> mResTunerHandles;

	pid_t mOurPid = 0;
    pid_t mOurTid = 0;
};

#endif // CONTEXTUAL_CLASSIFIER_H
