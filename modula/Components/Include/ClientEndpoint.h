// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef CLIENT_ENDPOINT_H
#define CLIENT_ENDPOINT_H

#include <cstdint>
#include <cstddef>

/**
 * @brief ClientEndpoint
 * @details Defines the Client Side Interface, which any Communication Medium
 *          (for example Socket) must Implement.
 */
class ClientEndpoint {
public:
    virtual int32_t initiateConnection() = 0;
    virtual int32_t sendMsg(char* buf, size_t bufSize) = 0;
    virtual int32_t readMsg(char* buf, size_t bufSize) = 0;
    virtual int32_t closeConnection() = 0;
};

#endif
