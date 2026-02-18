// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef CLIENT_GARBAGE_COLLECTOR_H
#define CLIENT_GARBAGE_COLLECTOR_H

/*!
 * \file  ClientGarbageCollector.h
 */

/*!
 * \ingroup  CLIENT_GARBAGE_COLLECTOR
 * \defgroup CLIENT_GARBAGE_COLLECTOR Client Garbage Collector
 * \details Runs as a Daemon Thread and Periodically (Every 83 seconds) and performs cleanup for
 *          a pre-defined max number of clients found in the Garbage Collector Queue (added by the Pulse Monitor).\n
 *          As part of the cleanup:\n\n
 *          1) All the active Requests from the client (if any) are untuned.\n\n
 *          2) The Request Manager is updated, so that these requests are no longer tracked
 *             as active Requests.\n\n
 *          3) The Client tracking entries maintained by the ClientDataManager for this client PID are cleared.\n\n
 *
 *          Note, not all clients in the queue are cleaned up at once, instead a pre-defined
 *          upper bound is placed on the number of clients to be cleaned in one iteration. The pending
 *          clients will be taken up in subsequent iterations.
 * @{
 */

#include <memory>
#include <queue>
#include <mutex>

#include "ErrCodes.h"
#include "Timer.h"
#include "RequestQueue.h"
#include "RequestManager.h"

/**
 * @brief ClientGarbageCollector
 * @details Untunes any outstanding Tune Requests for dead clients and removes the client tracking
 *          entries from the ClientDataManager.
 */
class ClientGarbageCollector {
private:
    static std::shared_ptr<ClientGarbageCollector> mClientGarbageCollectorInstance;

    Timer* mTimer;
    std::mutex mGcQueueMutex;
    std::queue<pid_t> mGcQueue;
    uint32_t mGarbageCollectionDuration;

    ClientGarbageCollector();

    void performCleanup();

public:
    ~ClientGarbageCollector();

    /**
     * @brief Starts the Client Garbage Collector
     * @details To start the Client Garbage Collector a recurring timer is created by using a
     *          thread from the Thread Pool. This Thread will wake up periodically
     *          and cleanup clients present in the garbage collector queue.
     * @return ErrCode:\n
     *            - RC_SUCCESS If the Client Garbage Collector is successfully started\n
     *            - Enum Code indicating error: Otherwise.
     */
    ErrCode startClientGarbageCollectorDaemon();

    void stopClientGarbageCollectorDaemon();
    void submitClientForCleanup(pid_t clientPid);

    static std::shared_ptr<ClientGarbageCollector> getInstance() {
        if(mClientGarbageCollectorInstance == nullptr) {
            mClientGarbageCollectorInstance = std::shared_ptr<ClientGarbageCollector> (new ClientGarbageCollector());
        }
        return mClientGarbageCollectorInstance;
    }
};

__attribute__((visibility("default"))) ErrCode startClientGarbageCollectorDaemon();
__attribute__((visibility("default"))) void stopClientGarbageCollectorDaemon();

#endif

/*! @} */
