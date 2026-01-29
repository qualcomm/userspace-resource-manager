// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SocketClient.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)
#endif

SocketClient::SocketClient() {
    this->sockFd = -1;
}

int32_t SocketClient::initiateConnection() {
    if((this->sockFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        TYPELOGV(ERRNO_LOG, "socket", strerror(errno));
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    size_t written = snprintf(addr.sun_path, UNIX_PATH_MAX, RESTUNE_SOCKET_PATH);
    if(written >= UNIX_PATH_MAX) {
        LOGE("RESTUNE_SOCKET_CLIENT", "Socket path too long");
        close(this->sockFd);
        this->sockFd = -1;
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    if(connect(this->sockFd, (const sockaddr*)&addr, sizeof(struct sockaddr_un)) < 0) {
        TYPELOGV(ERRNO_LOG, "connect", strerror(errno));
        close(this->sockFd);
        this->sockFd = -1;
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    return RC_SUCCESS;
}

int32_t SocketClient::sendMsg(char* buf, size_t bufSize) {
    if(buf == nullptr) return RC_BAD_ARG;

    if(write(this->sockFd, buf, bufSize) == -1) {
        TYPELOGV(ERRNO_LOG, "write", strerror(errno));
        return RC_SOCKET_FD_WRITE_FAILURE;
    }

    return RC_SUCCESS;
}

int32_t SocketClient::readMsg(char* buf, size_t bufSize) {
    if(buf == nullptr || bufSize == 0) {
        return RC_BAD_ARG;
    }
    int32_t statusCode;

    if((statusCode = read(this->sockFd, buf, bufSize)) < 0) {
        TYPELOGV(ERRNO_LOG, "read", strerror(errno));
        return RC_SOCKET_FD_READ_FAILURE;
    }

    return RC_SUCCESS;
}

int32_t SocketClient::closeConnection() {
    if(this->sockFd != -1) {
        return close(this->sockFd);
    }
    return RC_SOCKET_FD_CLOSE_FAILURE;
}

SocketClient::~SocketClient() {
    if(this->sockFd != -1) {
        close(this->sockFd);
    }
    this->sockFd = -1;
}
