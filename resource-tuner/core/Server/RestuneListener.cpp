// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "RestuneListener.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)
#endif

SocketServer::SocketServer(
    ServerOnlineCheckCallback mServerOnlineCheckCb,
    MessageReceivedCallback mMessageRecvCb) {

    this->sockFd = -1;
    this->mServerOnlineCheckCb = mServerOnlineCheckCb;
    this->mMessageRecvCb = mMessageRecvCb;
}

// Called by server, this will put the server in listening mode
int32_t SocketServer::ListenForClientRequests() {
    if((this->sockFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        TYPELOGV(ERRNO_LOG, "socket", strerror(errno));
        LOGE("RESTUNE_SOCKET_SERVER", "Failed to initialize Server Socket");
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    // Make the socket Non-Blocking
    if (fcntl(this->sockFd, F_SETFL, O_NONBLOCK) < 0) {
        close(this->sockFd);
        this->sockFd = -1;
        LOGE("RESTUNE_SOCKET_SERVER", std::string("Failed to make socket non-blocking: ") + strerror(errno));
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(sockaddr_un));
    addr.sun_family = AF_UNIX;
    size_t written = snprintf(addr.sun_path, UNIX_PATH_MAX, RESTUNE_SOCKET_PATH);
    if(written >= UNIX_PATH_MAX) {
        LOGE("RESTUNE_SOCKET_SERVER", "Socket path too long");
        close(this->sockFd);
        this->sockFd = -1;
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    // Remove old socket file
    unlink(addr.sun_path);

    if(bind(this->sockFd, (const sockaddr*)&addr, sizeof(addr)) < 0) {
        TYPELOGV(ERRNO_LOG, "bind", strerror(errno));
        close(this->sockFd);
        this->sockFd = -1;
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    // Set permissions for server
    mode_t perm = 0666;
    if(chmod(RESTUNE_SOCKET_PATH, perm) < 0) {
        TYPELOGV(ERRNO_LOG, "permission", strerror(errno));
        close(this->sockFd);
        this->sockFd = -1;
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    if(listen(this->sockFd, maxEvents) < 0) {
        TYPELOGV(ERRNO_LOG, "listen", strerror(errno));
        close(this->sockFd);
        this->sockFd = -1;
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    int32_t epollFd = epoll_create1(0);
    if(epollFd < 0) {
        TYPELOGV(ERRNO_LOG, "epoll_create1", strerror(errno));
        close(this->sockFd);
        this->sockFd = -1;
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    epoll_event event{}, events[maxEvents];
    event.events = EPOLLIN;
    event.data.fd = this->sockFd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, this->sockFd, &event) < 0) {
        TYPELOGV(ERRNO_LOG, "epoll_ctl", strerror(errno));
        close(epollFd);
        close(this->sockFd);
        this->sockFd = -1;
        return RC_SOCKET_CONN_NOT_INITIALIZED;
    }

    while(this->mServerOnlineCheckCb()) {
        int32_t clientsFdCount = epoll_wait(epollFd, events, maxEvents, 1000);

        for(int32_t i = 0; i < clientsFdCount; i++) {
            if(events[i].data.fd == this->sockFd) {
                // Process all the Requests in the backlog
                while(true) {
                    int32_t clientSocket = -1;
                    if((clientSocket = accept(this->sockFd, nullptr, nullptr)) < 0) {
                        if(errno != EAGAIN && errno != EWOULDBLOCK) {
                            TYPELOGV(ERRNO_LOG, "accept", strerror(errno));
                            LOGE("RESTUNE_SOCKET_SERVER", "Server Socket-Endpoint crashed");
                            return RC_SOCKET_OP_FAILURE;
                        } else {
                            // No more clients to accept, Backlog is completely drained.
                            break;
                        }
                    }

                    if(clientSocket >= 0) {
                        MsgForwardInfo* info = nullptr;
                        char* reqBuf = nullptr;

                        try {
                            info = new (GetBlock<MsgForwardInfo>()) MsgForwardInfo;
                            reqBuf = new (GetBlock<char[REQ_BUFFER_SIZE]>()) char[REQ_BUFFER_SIZE];

                            info->mBuffer = reqBuf;
                            info->mBufferSize = REQ_BUFFER_SIZE;

                        } catch(const std::bad_alloc& e) {
                            FreeBlock<MsgForwardInfo>(info);
                            FreeBlock<char[REQ_BUFFER_SIZE]>(reqBuf);

                            // Failed to allocate memory for Request, close client socket.
                            close(clientSocket);
                            continue;
                        }

                        int32_t bytesRead = 0;
                        if((bytesRead = recv(clientSocket, info->mBuffer, info->mBufferSize, 0)) < 0) {
                            if(errno != EAGAIN && errno != EWOULDBLOCK) {
                                TYPELOGV(ERRNO_LOG, "recv", strerror(errno));
                                LOGE("RESTUNE_SOCKET_SERVER", "Server Socket-Endpoint crashed");
                                return RC_SOCKET_OP_FAILURE;
                            }
                        }

                        if(bytesRead > 0) {
                            this->mMessageRecvCb(clientSocket, info);
                        }
                        close(clientSocket);
                    }
                }
            }
        }
    }

    return RC_SUCCESS;
}

int32_t SocketServer::closeConnection() {
    if(this->sockFd != -1) {
        close(this->sockFd);
        this->sockFd = -1;
    }
    return RC_SOCKET_FD_CLOSE_FAILURE;
}

SocketServer::~SocketServer() {
    if(this->sockFd != -1) {
        close(this->sockFd);
        this->sockFd = -1;
    }
}
