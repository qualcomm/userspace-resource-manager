// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef RESTUNE_SOCKET_SERVER_H
#define RESTUNE_SOCKET_SERVER_H

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "MemoryPool.h"
#include "Request.h"
#include "Signal.h"
#include "SafeOps.h"
#include "ServerEndpoint.h"
#include "UrmSettings.h"
#include "ErrCodes.h"
#include "Logger.h"

#define RESTUNE_SOCKET_PATH "/run/restune_sock"

static const uint32_t maxEvents = 128;

class SocketServer : public ServerEndpoint {
private:
    int32_t sockFd;
    ServerOnlineCheckCallback mServerOnlineCheckCb;
    MessageReceivedCallback mMessageRecvCb;

public:
    SocketServer(
        ServerOnlineCheckCallback mServerOnlineCheckCb,
        MessageReceivedCallback mMessageRecvCb);

    virtual ~SocketServer();

    virtual int32_t ListenForClientRequests();
    virtual int32_t closeConnection();
};

#endif
