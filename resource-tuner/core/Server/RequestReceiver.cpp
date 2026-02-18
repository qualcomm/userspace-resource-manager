// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "RequestReceiver.h"

std::shared_ptr<RequestReceiver> RequestReceiver::mRequestReceiverInstance = nullptr;
ThreadPool* RequestReceiver::mRequestsThreadPool = nullptr;

RequestReceiver::RequestReceiver() {}

void RequestReceiver::forwardMessage(int32_t clientSocket, MsgForwardInfo* info) {
    int8_t moduleID = *(int8_t*) info->mBuffer;
    int8_t requestType = *(int8_t*) ((unsigned char*) info->mBuffer + sizeof(int8_t));

    info->mModuleID = moduleID;
    info->mRequestType = requestType;

    if(info->mRequestType == REQ_PROP_GET) {
        std::string result = "";
        size_t writeLen = submitPropGetRequest(info, result);
        if(writeLen == 0) {
            result = "na";
            writeLen = result.length();
        }

        if(write(clientSocket, result.c_str(), writeLen) == -1) {
            TYPELOGV(ERRNO_LOG, "write", strerror(errno));
        }
        return;
    }

    info->mHandle = AuxRoutines::generateUniqueHandle();
    if(info->mHandle < 0) {
        // Handle Generation Failure
        LOGE("RESTUNE_REQUEST_RECEIVER", "Failed to Generate Request handle");
        return;
    }

    if(this->mRequestsThreadPool == nullptr) {
        LOGE("URM_SERVER_ENDPOINT", "Thread pool not initialized, Dropping the Request");
        return;
    }

    // Enqueue the Request to the Thread Pool for async processing.
    switch(info->mRequestType) {
        case REQ_RESOURCE_TUNING:
        case REQ_RESOURCE_RETUNING:
        case REQ_RESOURCE_UNTUNING: {
            if(!this->mRequestsThreadPool->enqueueTask(submitResProvisionReqMsg, info)) {
                LOGE("URM_SERVER_ENDPOINT", "Failed to enqueue the Request to the Thread Pool");
            }
            break;
        }

        case REQ_SIGNAL_TUNING:
        case REQ_SIGNAL_UNTUNING:
        case REQ_SIGNAL_RELAY: {
            if(!this->mRequestsThreadPool->enqueueTask(submitSignalRequest, info)) {
                LOGE("URM_SERVER_ENDPOINT", "Failed to enqueue the Request to the Thread Pool");
            }
            break;
        }
    }

    // Only in Case of Tune Requests, Write back the handle to the client.
    if(requestType == REQ_RESOURCE_TUNING || requestType == REQ_SIGNAL_TUNING) {
        if(write(clientSocket, (const void*)&info->mHandle, sizeof(int64_t)) == -1) {
            TYPELOGV(ERRNO_LOG, "write", strerror(errno));
        }
    }
}

static int8_t checkServerOnlineStatus() {
    return UrmSettings::isServerOnline();
}

void onMsgRecvCallback(int32_t clientSocket, MsgForwardInfo* msgForwardInfo) {
    std::shared_ptr<RequestReceiver> requestReceiver = RequestReceiver::getInstance();
    requestReceiver->forwardMessage(clientSocket, msgForwardInfo);
}

void listenerThreadStartRoutine() {
    SocketServer* connection = nullptr;

    try {
        connection = new SocketServer(checkServerOnlineStatus, onMsgRecvCallback);
    } catch(const std::exception& e) {
        LOGE("URM_SERVER_ENDPOINT",
             "Failed to start the Resource Tuner Listener, error: " + std::string(e.what()));
        return;
    }

    if(RC_IS_NOTOK(connection->ListenForClientRequests())) {
        LOGE("URM_SERVER_ENDPOINT", "Server Socket Endpoint crashed");
    }

    if(connection != nullptr) {
        delete(connection);
    }
}
