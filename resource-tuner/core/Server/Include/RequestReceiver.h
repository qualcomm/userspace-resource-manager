// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef RESTUNE_REQUEST_RECEIVER_H
#define RESTUNE_REQUEST_RECEIVER_H

#include <memory>
#include <cstring>
#include <fstream>
#include <sstream>

#include "ErrCodes.h"
#include "Logger.h"
#include "RestuneInternal.h"
#include "SignalInternal.h"
#include "RestuneListener.h"
#include "UrmSettings.h"
#include "AuxRoutines.h"
#include "ComponentRegistry.h"

/**
 * @brief RequestReceiver
 * @details Handles incoming client-requests, by forwarding the request to the
 *          appropriate module, using that module's registered callback.
 *          Note, the callback is invoked on a separate thread (taken from the ThreadPool)
 */
class __attribute__((visibility("default"))) RequestReceiver {
private:
    static std::shared_ptr<RequestReceiver> mRequestReceiverInstance;

    RequestReceiver();

public:
    static ThreadPool* mRequestsThreadPool;

    void forwardMessage(int32_t clientSocket, MsgForwardInfo* msgForwardInfo);

    static std::shared_ptr<RequestReceiver> getInstance() {
        if(mRequestReceiverInstance == nullptr) {
            mRequestReceiverInstance = std::shared_ptr<RequestReceiver>(new RequestReceiver());
        }
        return mRequestReceiverInstance;
    }
};

__attribute__((visibility("default"))) void listenerThreadStartRoutine();

#endif
