// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <mutex>
#include <memory>

#include "Utils.h"
#include "UrmAPIs.h"
#include "AuxRoutines.h"
#include "SocketClient.h"

#define FILE_TAG "URM_API_CLIENT"
#define REQ_SEND_ERR(e) "Failed to send Request to Server, Error: " + std::string(e)
#define CONN_SEND_FAIL "Failed to send Request to Server"
#define CONN_INIT_FAIL "Failed to initialize Connection to resource-tuner Server"

class ClientMgr {
public:
    int8_t isUrmCli;
    std::string mClientComm;
    std::shared_ptr<ClientEndpoint> conn;

    ClientMgr() {
        this->isUrmCli = false;
        this->mClientComm = "";
        if(AuxRoutines::fetchComm(getpid(), this->mClientComm) == 0) {
            if(this->mClientComm == "urmCli") {
                this->isUrmCli = true;
            }
        }

        if(conn == nullptr) {
            try {
                conn = std::shared_ptr<SocketClient> (new SocketClient());
            } catch(const std::bad_alloc& e) {
                LOGE(FILE_TAG, "Failed to establish connection with URM server");
            }
        }
    }
};

// Byte Encoder
static FlatBuffEncoder batch;
static std::mutex apiLock;

// Max cap of 20 Resources per Request
// Note: The actual number eventually allowed might be lower if there are
// a lot ofmulti-valued resources, as the entire request needs to be encoded
// as a byte-buffer of size REQ_BUFFER_SIZE.
static const int32_t maxResPerReq = 20;

static ClientMgr urmClientInfo;

static int8_t sendMsgHelper(char* buf) {
    if(urmClientInfo.conn == nullptr || RC_IS_NOTOK(urmClientInfo.conn->initiateConnection())) {
        LOGE(FILE_TAG, CONN_INIT_FAIL);
        return -1;
    }

    // Send the request to URM Server
    if(RC_IS_NOTOK(urmClientInfo.conn->sendMsg(buf, REQ_BUFFER_SIZE))) {
        LOGE(FILE_TAG, CONN_SEND_FAIL);
        return -1;
    }

    return 0;
}

static int64_t readHandleHelper() {
    // Get the handle
    char resultBuf[64] = {0};
    if(RC_IS_NOTOK(urmClientInfo.conn->readMsg(resultBuf, sizeof(resultBuf)))) {
        return -1;
    }

    int64_t handleReceived = -1;
    std::memcpy(&handleReceived, resultBuf, sizeof(handleReceived));
    return handleReceived;
}

static int32_t getClientPid() {
    if(urmClientInfo.isUrmCli) {
        return (int32_t)getppid();
    }
    return (int32_t)getpid();
}

static int32_t getClientTid() {
    if(urmClientInfo.isUrmCli) {
        return (int32_t)getppid();
    }
    return (int32_t)gettid();
}

// - Construct a Request object and populate it with the API specified Params
// - Initiate a connection to the URM Server, and send the request to the server
// - Wait for the response from the server, and return the response to the caller (end-client).
int64_t tuneResources(int64_t duration,
                      int32_t properties,
                      int32_t numRes,
                      SysResource* resourceList) {
    // Only one client Thread can send a Request at any moment
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(urmClientInfo.conn);

        // Preliminary Tests
        // These are some basic checks done at the Client end itself to detect
        // Potentially Malformed Reqeusts, to prevent wastage of Server-End Resources.
        if(resourceList == nullptr || numRes <= 0 || duration == 0 || duration < -1) {
            LOGE(FILE_TAG, "Invalid Request Params");
            return -1;
        }

        if(numRes > maxResPerReq) {
            LOGE(FILE_TAG, "Number of Resources in Request exceeds max limit.");
            return -1;
        }

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
             .append<int32_t>(VALIDATE_GT(numRes, 0))
             .append<int32_t>(VALIDATE_GE(properties, 0))
             .append<int32_t>(getClientPid())
             .append<int32_t>(getClientTid());

        for(int32_t i = 0; i < numRes; i++) {
            SysResource resource = SafeDeref((resourceList + i));

            batch.append<uint32_t>(VALIDATE_GT(resource.mResCode, 0))
                 .append<int32_t>(VALIDATE_GE(resource.mResInfo, 0))
                 .append<int32_t>(VALIDATE_GE(resource.mOptionalInfo, 0))
                 .append<int32_t>(VALIDATE_GT(resource.mNumValues, 0));

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
            LOGE(FILE_TAG, "Request Size exceeds max capacity");
        }

    } catch(const std::exception& e) {
        LOGE(FILE_TAG, REQ_SEND_ERR(e.what()));
    }

    return -1;
}

// - Construct a Request object and populate it with the API specified Params
// - Initiate a connection to the URM Server, and send the request to the server
int8_t retuneResources(int64_t handle, int64_t duration) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(urmClientInfo.conn);

        if(handle <= 0 || duration == 0 || duration < -1) {
            LOGE(FILE_TAG, "Invalid Request Params");
            return -1;
        }

        char buf[REQ_BUFFER_SIZE] = {0};
        batch.setBuf(buf);
        batch.append<int8_t>(MOD_RESTUNE)
             .append<int8_t>(REQ_RESOURCE_RETUNING)
             .append<int64_t>(handle)
             .append<int64_t>(duration)
             .append<int32_t>(0)
             .append<int32_t>(0)
             .append<int32_t>(getClientPid())
             .append<int32_t>(getClientTid());

        if(batch.isBufSane()) {
            if(sendMsgHelper(buf) == 0) return 0;
        } else {
            LOGE(FILE_TAG, "Malformed Request");
        }

    } catch(const std::exception& e) {
        LOGE(FILE_TAG, REQ_SEND_ERR(e.what()));
    }

    return -1;
}

// - Construct a Request object and populate it with the API specified Params
// - Initiate a connection to the URM Server, and send the request to the server
int8_t untuneResources(int64_t handle) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(urmClientInfo.conn);

        if(handle <= 0) {
            LOGE(FILE_TAG, "Invalid Request Params");
            return -1;
        }

        char buf[REQ_BUFFER_SIZE] = {0};
        batch.setBuf(buf);
        batch.append<int8_t>(MOD_RESTUNE)
             .append<int8_t>(REQ_RESOURCE_UNTUNING)
             .append<int64_t>(handle)
             .append<int64_t>(-1)
             .append<int32_t>(0)
             .append<int32_t>(0)
             .append<int32_t>(getClientPid())
             .append<int32_t>(getClientTid());

        if(batch.isBufSane()) {
            if(sendMsgHelper(buf) == 0) return 0;
        } else {
            LOGE(FILE_TAG, "Malformed Request");
        }

    } catch(const std::exception& e) {
        LOGE(FILE_TAG, REQ_SEND_ERR(e.what()));
    }

    return -1;
}

// - Construct a Prop Get Request object and populate it with the Request Params
// - Initiate a connection to the URM Server, and send the request to the server
// - Wait for the response from the server, and return the response to the caller (end-client).
int8_t getProp(const char* prop, char* buffer, size_t bufferSize, const char* defValue) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(urmClientInfo.conn);

        if(prop == nullptr || buffer == nullptr) {
            LOGE(FILE_TAG, "Invalid Request Params");
            return -1;
        }

        char buf[REQ_BUFFER_SIZE] = {0};
        batch.setBuf(buf);
        batch.append<int8_t>(MOD_RESTUNE)
             .append<int8_t>(REQ_PROP_GET)
             .append<uint64_t>(bufferSize);

        const char* charIterator = prop;
        while(*charIterator != '\0') {
            batch.append<uint8_t>(*charIterator);
            charIterator++;
        }

        batch.append<uint8_t>('\0');
        if(!batch.isBufSane()) {
            LOGE(FILE_TAG, "Malformed Request");
            return -1;
        }

        if(urmClientInfo.conn == nullptr || RC_IS_NOTOK(urmClientInfo.conn->initiateConnection())) {
            LOGE(FILE_TAG, CONN_INIT_FAIL);
            return -1;
        }

        if(RC_IS_NOTOK(urmClientInfo.conn->sendMsg(buf, sizeof(buf)))) {
            LOGE(FILE_TAG, CONN_SEND_FAIL);
            return -1;
        }

        // read the response
        char resultBuf[bufferSize];
        memset(resultBuf, 0, sizeof(resultBuf));
        if(RC_IS_NOTOK(urmClientInfo.conn->readMsg(resultBuf, sizeof(resultBuf)))) {
            return -1;
        }

        if(strncmp(resultBuf, "na", 2) == 0) {
            // Copy default value
            strncpy(buffer, defValue, bufferSize - 1);
        } else {
            strncpy(buffer, resultBuf, bufferSize - 1);
        }

        return 0;

    } catch(const std::invalid_argument& e) {
        LOGE(FILE_TAG, REQ_SEND_ERR(e.what()));
        return -1;

    } catch(const std::exception& e) {
        LOGE(FILE_TAG, REQ_SEND_ERR(e.what()));
        return -1;
    }

    return -1;
}

// - Construct a Signal object and populate it with the Signal Request Params
// - Initiate a connection to the URM Server, and send the request to the server
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
        const ConnectionManager connMgr(urmClientInfo.conn);

        // Duration == 0 is a placeholder for default signal-config specified duration
        if(duration < -1 || numArgs < 0 || (list != nullptr && numArgs == 0)) {
            LOGE(FILE_TAG, "Invalid Request Params");
            return -1;
        }

        char buf[REQ_BUFFER_SIZE] = {0};
        batch.setBuf(buf);
        batch.append<int8_t>(MOD_RESTUNE)
             .append<int8_t>(REQ_SIGNAL_TUNING)
             .append<uint32_t>(sigId)
             .append<uint32_t>(sigType)
             .append<int64_t>(0)
             .append<int64_t>(duration);

        const char* charIterator = appName;
        while(*charIterator != '\0') {
            batch.append<uint8_t>(*charIterator);
            charIterator++;
        }

        batch.append<uint8_t>('\0');

        charIterator = scenario;
        while(*charIterator != '\0') {
            batch.append<uint8_t>(*charIterator);
            charIterator++;
        }

        batch.append<uint8_t>('\0');
        batch.append<int32_t>(VALIDATE_GE(numArgs, 0))
             .append<int32_t>(VALIDATE_GE(properties, 0))
             .append<int32_t>(getClientPid())
             .append<int32_t>(getClientTid());

        for(int32_t i = 0; i < numArgs; i++) {
            batch.append<uint32_t>(list[i]);
        }

        if(batch.isBufSane()) {
            if(sendMsgHelper(buf) == 0) return readHandleHelper();
        } else {
            LOGE(FILE_TAG, "Request Size exceeds max capacity");
        }

    } catch(const std::exception& e) {
        LOGE(FILE_TAG, REQ_SEND_ERR(e.what()));
    }

    return -1;
}

// - Construct a Signal object and populate it with the Signal Request Params
// - Initiate a connection to the URM Server, and send the request to the server
int8_t untuneSignal(int64_t handle) {
    try {
        const std::lock_guard<std::mutex> lock(apiLock);
        const ConnectionManager connMgr(urmClientInfo.conn);

        if(handle <= 0) {
            LOGE(FILE_TAG, "Invalid Request Params");
            return -1;
        }

        char buf[REQ_BUFFER_SIZE] = {0};
        batch.setBuf(buf);
        batch.append<int8_t>(MOD_RESTUNE)
             .append<int8_t>(REQ_SIGNAL_UNTUNING)
             .append<uint32_t>(0)
             .append<uint32_t>(0)
             .append<int64_t>(handle)
             .append<int64_t>(-1)
             .append<uint8_t>('\0')
             .append<uint8_t>('\0')
             .append<int32_t>(0)
             .append<int32_t>(0)
             .append<int32_t>(getClientPid())
             .append<int32_t>(getClientTid());


        if(batch.isBufSane()) {
            if(sendMsgHelper(buf) == 0) return 0;
        } else {
            LOGE(FILE_TAG, "Malformed Request");
        }

    } catch(const std::exception& e) {
        LOGE(FILE_TAG, REQ_SEND_ERR(e.what()));
    }

    return -1;
}

// - Construct a Signal object and populate it with the Signal Request Params
// - Initiate a connection to the URM Server, and send the request to the server
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
        const ConnectionManager connMgr(urmClientInfo.conn);

        // Duration == 0 is a placeholder for default signal-config specified duration
        if(duration < -1 || numArgs < 0 || (list != nullptr && numArgs == 0)) {
            LOGE(FILE_TAG, "Invalid Request Params");
            return -1;
        }

        char buf[REQ_BUFFER_SIZE] = {0};
        batch.setBuf(buf);
        batch.append<int8_t>(MOD_RESTUNE)
             .append<int8_t>(REQ_SIGNAL_RELAY)
             .append<uint32_t>(sigId)
             .append<uint32_t>(sigType)
             .append<int64_t>(0)
             .append<int64_t>(duration);

        const char* charIterator = appName;
        while(*charIterator != '\0') {
            batch.append<uint8_t>(*charIterator);
            charIterator++;
        }

        batch.append<uint8_t>('\0');

        charIterator = scenario;
        while(*charIterator != '\0') {
            batch.append<uint8_t>(*charIterator);
            charIterator++;
        }

        batch.append<uint8_t>('\0');
        batch.append<int32_t>(VALIDATE_GE(numArgs, 0))
             .append<int32_t>(VALIDATE_GE(properties, 0))
             .append<int32_t>(getClientPid())
             .append<int32_t>(getClientTid());

        for(int32_t i = 0; i < numArgs; i++) {
            batch.append<uint32_t>(list[i]);
        }

        if(batch.isBufSane()) {
            if(sendMsgHelper(buf) == 0) return 0;
        } else {
            LOGE(FILE_TAG, "Request Size exceeds max capacity");
        }

    } catch(const std::exception& e) {
        LOGE(FILE_TAG, REQ_SEND_ERR(e.what()));
    }

    return -1;
}
