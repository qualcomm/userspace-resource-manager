// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <memory>
#include <mutex>

#include "UrmAPIs.h"
#include "Utils.h"
#include "AuxRoutines.h"
#include "SocketClient.h"

#define REQ_SEND_ERR(e) "Failed to send Request to Server, Error: " + std::string(e)
#define CONN_SEND_FAIL "Failed to send Request to Server"
#define CONN_INIT_FAIL "Failed to initialize Connection to resource-tuner Server"

static std::shared_ptr<ClientEndpoint> conn(new SocketClient());

// Byte Encoder
static FlatBuffEncoder batch;
static std::mutex apiLock;
static const int32_t maxResPerReq = 20;

static int8_t sendMsgHelper(char* buf) {
    if(conn == nullptr || RC_IS_NOTOK(conn->initiateConnection())) {
        LOGE("RESTUNE_CLIENT", CONN_INIT_FAIL);
        return -1;
    }

    // Send the request to Resource Tuner Server
    if(RC_IS_NOTOK(conn->sendMsg(buf, REQ_BUFFER_SIZE))) {
        LOGE("RESTUNE_CLIENT", CONN_SEND_FAIL);
        return -1;
    }

    return 0;
}

static int64_t readHandleHelper() {
    // Get the handle
    char resultBuf[64] = {0};
    if(RC_IS_NOTOK(conn->readMsg(resultBuf, sizeof(resultBuf)))) {
        return -1;
    }

    int64_t handleReceived = -1;
    std::memcpy(&handleReceived, resultBuf, sizeof(handleReceived));
    return handleReceived;
}

// - Construct a Request object and populate it with the API specified Params
// - Initiate a connection to the Resource Tuner Server, and send the request to the server
// - Wait for the response from the server, and return the response to the caller (end-client).
int64_t tuneResources(int64_t duration,
                      int32_t properties,
                      int32_t numRes,
                      SysResource* resourceList) {
    // Only one client Thread can send a Request at any moment
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(conn);

        // Preliminary Tests
        // These are some basic checks done at the Client end itself to detect
        // Potentially Malformed Reqeusts, to prevent wastage of Server-End Resources.
        if(resourceList == nullptr || numRes <= 0 || duration == 0 || duration < -1) {
            LOGE("RESTUNE_CLIENT", "Invalid Request Params");
            return -1;
        }

        if(numRes > maxResPerReq) {
            LOGE("RESTUNE_CLIENT", "Number of Resources in Request exceeds max limit.");
            return -1;
        }

        VALIDATE_GE(properties, 0);

        // Encoding Order:
        // 0. Module ID
        // 1. Request Type
        // 2. Request Handle (applicable for untune and retune requests)
        // 3. Duration
        // 4. Number of Resources
        // 5. Properties
        // 6. PID
        // 7. TID
        // 8. Resource List:
        //      Each resource is encoded as:
        //          8.1 ResCode
        //          8.2 ResInfo
        //          8.3 OptionalInfo
        //          8.4 NumValues
        //          8.5 List of "#NumValues" values.

        char buf[REQ_BUFFER_SIZE] = {0};
        batch.setBuf(buf);
        batch.append<int8_t>(MOD_RESTUNE)
             .append<int8_t>(REQ_RESOURCE_TUNING)
             .append<int64_t>(0)
             .append<int64_t>(duration)
             .append<int32_t>(numRes)
             .append<int32_t>(properties)
             .append<int32_t>(getpid())
             .append<int32_t>(gettid());

        for(int32_t i = 0; i < numRes; i++) {
            SysResource resource = SafeDeref((resourceList + i));

            VALIDATE_GE(resource.mResCode, 0);
            VALIDATE_GE(resource.mResInfo, 0);
            VALIDATE_GE(resource.mOptionalInfo, 0);
            VALIDATE_GT(resource.mNumValues, 0);

            batch.append<uint32_t>(resource.mResCode)
                 .append<int32_t>(resource.mResInfo)
                 .append<int32_t>(resource.mOptionalInfo)
                 .append<int32_t>(resource.mNumValues);

            if(resource.mNumValues == 1) {
                batch.append<int32_t>(resource.mResValue.value);
            } else {
                for(int32_t j = 0; j < resource.mNumValues; j++) {
                    batch.append<int32_t>(resource.mResValue.values[j]);
                }
            }
        }

        if(batch.isBufSane()) {
            if(sendMsgHelper(buf) == 0) return readHandleHelper();
        } else {
            LOGE("RESTUNE_CLIENT", "Request Size exceeds max capacity");
        }

    } catch(const std::exception& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
    }

    return -1;
}

// - Construct a Request object and populate it with the API specified Params
// - Initiate a connection to the Resource Tuner Server, and send the request to the server
int8_t retuneResources(int64_t handle, int64_t duration) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(conn);

        if(handle <= 0  || duration == 0 || duration < -1) {
            LOGE("RESTUNE_CLIENT", "Invalid Request Params");
            return -1;
        }

        char buf[1024];
        int8_t* ptr8 = (int8_t*)buf;
        ASSIGN_AND_INCR(ptr8, MOD_RESTUNE);
        ASSIGN_AND_INCR(ptr8, REQ_RESOURCE_RETUNING);

        int64_t* ptr64 = (int64_t*)ptr8;
        ASSIGN_AND_INCR(ptr64, handle);
        ASSIGN_AND_INCR(ptr64, duration);

        int32_t* ptr = (int32_t*)ptr64;
        ASSIGN_AND_INCR(ptr, 0);
        ASSIGN_AND_INCR(ptr, 0);
        ASSIGN_AND_INCR(ptr, (int32_t)getpid());
        ASSIGN_AND_INCR(ptr, (int32_t)gettid());

        if(RC_IS_NOTOK(conn == nullptr || conn->initiateConnection())) {
            LOGE("RESTUNE_CLIENT", CONN_INIT_FAIL);
            return -1;
        }

        if(RC_IS_NOTOK(conn->sendMsg(buf, sizeof(buf)))) {
            LOGE("RESTUNE_CLIENT", CONN_SEND_FAIL);
            return -1;
        }

        return 0;

    } catch(const std::invalid_argument& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;

    } catch(const std::exception& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;
    }
}

// - Construct a Request object and populate it with the API specified Params
// - Initiate a connection to the Resource Tuner Server, and send the request to the server
int8_t untuneResources(int64_t handle) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(conn);

        if(handle <= 0) {
            LOGE("RESTUNE_CLIENT", "Invalid Request Params");
            return -1;
        }

        char buf[1024];
        int8_t* ptr8 = (int8_t*)buf;
        ASSIGN_AND_INCR(ptr8, MOD_RESTUNE);
        ASSIGN_AND_INCR(ptr8, REQ_RESOURCE_UNTUNING);

        int64_t* ptr64 = (int64_t*)ptr8;
        ASSIGN_AND_INCR(ptr64, handle);
        ASSIGN_AND_INCR(ptr64, -1);

        int32_t* ptr = (int32_t*)ptr64;
        ASSIGN_AND_INCR(ptr, 0);
        ASSIGN_AND_INCR(ptr, 0);
        ASSIGN_AND_INCR(ptr, (int32_t)getpid());
        ASSIGN_AND_INCR(ptr, (int32_t)gettid());

        if(conn == nullptr || RC_IS_NOTOK(conn->initiateConnection())) {
            LOGE("RESTUNE_CLIENT", CONN_INIT_FAIL);
            return -1;
        }

        if(RC_IS_NOTOK(conn->sendMsg(buf, sizeof(buf)))) {
            LOGE("RESTUNE_CLIENT", CONN_SEND_FAIL);
            return -1;
        }

        return 0;

    } catch(const std::invalid_argument& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;

    } catch(const std::exception& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;
    }
}

// - Construct a Prop Get Request object and populate it with the Request Params
// - Initiate a connection to the Resource Tuner Server, and send the request to the server
// - Wait for the response from the server, and return the response to the caller (end-client).
int8_t getProp(const char* prop, char* buffer, size_t bufferSize, const char* defValue) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(conn);

        char buf[1024];
        int8_t* ptr8 = (int8_t*)buf;
        ASSIGN_AND_INCR(ptr8, MOD_RESTUNE);
        ASSIGN_AND_INCR(ptr8, REQ_PROP_GET);

        const char* charIterator = prop;
        char* charPointer = (char*) ptr8;

        while(*charIterator != '\0') {
            ASSIGN_AND_INCR(charPointer, *charIterator);
            charIterator++;
        }

        ASSIGN_AND_INCR(charPointer, '\0');

        uint64_t* ptr64 = (uint64_t*)charPointer;
        ASSIGN_AND_INCR(ptr64, bufferSize);

        if(conn == nullptr || RC_IS_NOTOK(conn->initiateConnection())) {
            LOGE("RESTUNE_CLIENT", CONN_INIT_FAIL);
            return -1;
        }

        if(RC_IS_NOTOK(conn->sendMsg(buf, sizeof(buf)))) {
            LOGE("RESTUNE_CLIENT", CONN_SEND_FAIL);
            return -1;
        }

        // read the response
        char resultBuf[bufferSize];
        if(RC_IS_NOTOK(conn->readMsg(resultBuf, sizeof(resultBuf)))) {
            return -1;
        }

        buffer[bufferSize - 1] = '\0';
        if(strncmp(resultBuf, "na", 2) == 0) {
            // Copy default value
            strncpy(buffer, defValue, bufferSize - 1);
        } else {
            strncpy(buffer, resultBuf, bufferSize - 1);
        }

        return 0;

    } catch(const std::invalid_argument& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;

    } catch(const std::exception& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;
    }

    return -1;
}

// - Construct a Signal object and populate it with the Signal Request Params
// - Initiate a connection to the Resource Tuner Server, and send the request to the server
// - Wait for the response from the server, and return the response to the caller (end-client).
int64_t tuneSignal(uint32_t sigId,
                   uint32_t sigType,
                   int64_t duration,
                   int32_t properties,
                   const char* appName,
                   const char* scenario,
                   int32_t numArgs,
                   uint32_t* list) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(conn);

        if(duration < -1) {
            LOGE("RESTUNE_CLIENT", "Invalid Request Params");
            return -1;
        }

        char buf[1024];
        int8_t* ptr8 = (int8_t*)buf;
        ASSIGN_AND_INCR(ptr8, MOD_RESTUNE);
        ASSIGN_AND_INCR(ptr8, REQ_SIGNAL_TUNING);

        int32_t* ptr = (int32_t*)ptr8;
        ASSIGN_AND_INCR(ptr, sigId);
        ASSIGN_AND_INCR(ptr, sigType);

        int64_t* ptr64 = (int64_t*)ptr;
        ASSIGN_AND_INCR(ptr64, 0);
        ASSIGN_AND_INCR(ptr64, duration);

        const char* charIterator = appName;
        char* charPointer = (char*) ptr64;

        while(*charIterator != '\0') {
            ASSIGN_AND_INCR(charPointer, *charIterator);
            charIterator++;
        }

        ASSIGN_AND_INCR(charPointer, '\0');

        charIterator = scenario;

        while(*charIterator != '\0') {
            ASSIGN_AND_INCR(charPointer, *charIterator);
            charIterator++;
        }

        ASSIGN_AND_INCR(charPointer, '\0');

        ptr = (int32_t*)charPointer;
        ASSIGN_AND_INCR(ptr, VALIDATE_GE(numArgs, 0));
        ASSIGN_AND_INCR(ptr, VALIDATE_GE(properties, 0));
        ASSIGN_AND_INCR(ptr, (int32_t)getpid());
        ASSIGN_AND_INCR(ptr, (int32_t)gettid());

        for(int32_t i = 0; i < numArgs; i++) {
            uint32_t arg = list[i];
            ASSIGN_AND_INCR(ptr, arg)
        }

        if(conn == nullptr || RC_IS_NOTOK(conn->initiateConnection())) {
            LOGE("RESTUNE_CLIENT", CONN_INIT_FAIL);
            return -1;
        }

        // Send the request to Resource Tuner Server
        if(RC_IS_NOTOK(conn->sendMsg(buf, sizeof(buf)))) {
            LOGE("RESTUNE_CLIENT", CONN_SEND_FAIL);
            return -1;
        }

        // Get the handle
        char resultBuf[64] = {0};
        if(RC_IS_NOTOK(conn->readMsg(resultBuf, sizeof(resultBuf)))) {
            return -1;
        }

        int64_t handleReceived = -1;
        std::memcpy(&handleReceived, resultBuf, sizeof(handleReceived));

        return handleReceived;

    } catch(const std::invalid_argument& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;

    } catch(const std::exception& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;
    }

    return -1;
}

// - Construct a Signal object and populate it with the Signal Request Params
// - Initiate a connection to the Resource Tuner Server, and send the request to the server
int8_t untuneSignal(int64_t handle) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(conn);

        if(handle <= 0) {
            LOGE("RESTUNE_CLIENT", "Invalid Request Params");
            return -1;
        }

        char buf[1024];
        int8_t* ptr8 = (int8_t*)buf;
        ASSIGN_AND_INCR(ptr8, MOD_RESTUNE);
        ASSIGN_AND_INCR(ptr8, REQ_SIGNAL_UNTUNING);

        int32_t* ptr = (int32_t*)ptr8;
        ASSIGN_AND_INCR(ptr, 0);
        ASSIGN_AND_INCR(ptr, 0);

        int64_t* ptr64 = (int64_t*)ptr;
        ASSIGN_AND_INCR(ptr64, handle);
        ASSIGN_AND_INCR(ptr64, -1);

        const char* charIterator = "";
        char* charPointer = (char*) ptr64;

        while(*charIterator != '\0') {
            ASSIGN_AND_INCR(charPointer, *charIterator);
            charIterator++;
        }

        ASSIGN_AND_INCR(charPointer, '\0');

        charIterator = "";

        while(*charIterator != '\0') {
            ASSIGN_AND_INCR(charPointer, *charIterator);
            charIterator++;
        }

        ASSIGN_AND_INCR(charPointer, '\0');

        ptr = (int32_t*)charPointer;
        ASSIGN_AND_INCR(ptr, 0);
        ASSIGN_AND_INCR(ptr, 0);
        ASSIGN_AND_INCR(ptr, (int32_t)getpid());
        ASSIGN_AND_INCR(ptr, (int32_t)gettid());

        if(conn == nullptr || RC_IS_NOTOK(conn->initiateConnection())) {
            LOGE("RESTUNE_CLIENT", CONN_INIT_FAIL);
            return -1;
        }

        // Send the request to Resource Tuner Server
        if(RC_IS_NOTOK(conn->sendMsg(buf, sizeof(buf)))) {
            LOGE("RESTUNE_CLIENT", CONN_SEND_FAIL);
            return -1;
        }

        return 0;

    } catch(const std::invalid_argument& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;

    } catch(const std::exception& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;
    }

    return -1;
}

// - Construct a Signal object and populate it with the Signal Request Params
// - Initiate a connection to the Resource Tuner Server, and send the request to the server
int8_t relaySignal(uint32_t sigId,
                   uint32_t sigType,
                   int64_t duration,
                   int32_t properties,
                   const char* appName,
                   const char* scenario,
                   int32_t numArgs,
                   uint32_t* list) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(conn);

        if(duration < -1) {
            LOGE("RESTUNE_CLIENT", "Invalid Request Params");
            return -1;
        }

        char buf[1024];
        int8_t* ptr8 = (int8_t*)buf;
        ASSIGN_AND_INCR(ptr8, MOD_RESTUNE);
        ASSIGN_AND_INCR(ptr8, REQ_SIGNAL_RELAY);

        int32_t* ptr = (int32_t*)ptr8;
        ASSIGN_AND_INCR(ptr, sigId);
        ASSIGN_AND_INCR(ptr, sigType);

        int64_t* ptr64 = (int64_t*)ptr;
        ASSIGN_AND_INCR(ptr64, 0);
        ASSIGN_AND_INCR(ptr64, duration);

        const char* charIterator = appName;
        char* charPointer = (char*) ptr64;

        while(*charIterator != '\0') {
            ASSIGN_AND_INCR(charPointer, *charIterator);
            charIterator++;
        }

        ASSIGN_AND_INCR(charPointer, '\0');

        charIterator = scenario;

        while(*charIterator != '\0') {
            ASSIGN_AND_INCR(charPointer, *charIterator);
            charIterator++;
        }

        ASSIGN_AND_INCR(charPointer, '\0');

        ptr = (int32_t*)charPointer;
        ASSIGN_AND_INCR(ptr, VALIDATE_GE(numArgs, 0));
        ASSIGN_AND_INCR(ptr, VALIDATE_GE(properties, 0));
        ASSIGN_AND_INCR(ptr, (int32_t)getpid());
        ASSIGN_AND_INCR(ptr, (int32_t)gettid());

        for(int32_t i = 0; i < numArgs; i++) {
            uint32_t arg = list[i];
            ASSIGN_AND_INCR(ptr, arg)
        }

        if(conn == nullptr || RC_IS_NOTOK(conn->initiateConnection())) {
            LOGE("RESTUNE_CLIENT", CONN_INIT_FAIL);
            return -1;
        }

        // Send the request to Resource Tuner Server
        if(RC_IS_NOTOK(conn->sendMsg(buf, sizeof(buf)))) {
            LOGE("RESTUNE_CLIENT", CONN_SEND_FAIL);
            return -1;
        }

        return 0;

    } catch(const std::invalid_argument& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;

    } catch(const std::exception& e) {
        LOGE("RESTUNE_CLIENT", REQ_SEND_ERR(e.what()));
        return -1;
    }

    return -1;
}
