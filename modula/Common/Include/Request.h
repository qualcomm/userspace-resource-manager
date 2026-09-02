// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef REQUEST_H
#define REQUEST_H

#include <vector>

#include "ErrCodes.h"
#include "Timer.h"
#include "SafeOps.h"
#include "Utils.h"
#include "Message.h"
#include "Resource.h"
#include "DLManager.h"

#define REQUEST_DL_NR 0
#define COCO_TABLE_DL_NR 1

/**
 * @brief Encapsulation type for a Resource Provisioning Request.
 */
class Request : public Message {
private:
    Timer* mTimer; //!< Timer associated with the request.
    DLManager* mResourceList; //!< DLL to store the Resources to be configured.
    uint64_t mSource;

public:
    Request();
    ~Request();

    int32_t getResourcesCount();
    Timer* getTimer();
    DLManager* getResDlMgr();
    uint64_t getSource();

    void addResource(ResIterable* resIterable);
    void setTimer(Timer* timer);
    void unsetTimer();
    void setSource(uint64_t source);
    void clearResources();

    ErrCode deserialize(char* buf);

    void populateUntuneRequest(Request* request);
    void populateRetuneRequest(Request* request, int64_t duration);
    static void cleanUpRequest(Request* request);
};

#endif
