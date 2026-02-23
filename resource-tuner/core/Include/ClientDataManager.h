// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef CLIENT_DATA_MANAGER_H
#define CLIENT_DATA_MANAGER_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <memory>
#include <mutex>
#include "string.h"
#include "unistd.h"
#include "fstream"
#include "sstream"

#include "ErrCodes.h"
#include "DLManager.h"
#include "UrmSettings.h"
#include "MemoryPool.h"
#include "Logger.h"
#include "Utils.h"

#include "RestuneVisibility.h"

#define PER_CLIENT_TID_CAP 32

typedef struct _client_info {
    uint8_t mClientType;
    int32_t mCurClientThreads;
    int32_t mClientTIDs[PER_CLIENT_TID_CAP];

    _client_info(): mCurClientThreads(0) {}
} ClientInfo;

typedef struct {
    std::unordered_set<int64_t>* mClientHandles;
    int64_t mLastRequestTimestamp;
    double mHealth;
} ClientTidData;

/**
 * @details Stores and Maintains Client Tracking Data for all the Active Clients (i.e. clients with
 *          outstanding Requests). The Data Tracked for each Client includes:
 *          - PID, and the Access Level Permissions (Third Party or System) for the Client
 *          - List of Threads Belonging to the PID
 *          - List of Requests (identified by Handle) belonging to this Client
 *          - Health and Timestamp of Last Request (Used by RateLimiter)
 *          - Essentially ClientDataManager is a central storage for Client Data, and other Components
 *            like RateLimiter, PulseMonitor and RequestManager are clients of the ClientDataManager.
 */
class RESTUNE_INTERNAL_EXPORT ClientDataManager {
private:
    static std::shared_ptr<ClientDataManager> mClientDataManagerInstance;
    static std::mutex instanceProtectionLock;
    std::unordered_map<pid_t, ClientInfo*> mClientRepo; //!< Maintains Client Info indexed by PID
    std::unordered_map<pid_t, ClientTidData*> mClientTidRepo; //!< Maintains Client Info indexed by TID
    std::shared_timed_mutex mGlobalTableMutex;

    ClientDataManager();

public:
    /**
     * @brief Checks if the client with the given ID exists in the Client Data Table.
     * @param clientPID PID of the client
     * @param clientTID TID of the client
     * @return int8_t:\n
     *            - 1: if the client already exists\n
     *            - 0: otherwise
     */
    int8_t clientExists(pid_t clientPID, pid_t clientTID);

    /**
     * @brief Create a new entry for the client with the given PID in the ClientData Table.
     * @details This method should only be called if the clientExists method returns 0.
     * @param clientPID PID of the client
     * @param clientTID TID of the client
     * @return int8_t:\n
     *             - 1: Indicating that a new Client Tracking Entry was successfully Created.\n
     *             - 0: Otherwise
     */
    int8_t createNewClient(pid_t clientPID, pid_t clientTID);

    /**
     * @brief Returns a list of active requests for the client with the given PID.
     * @param clientTID Process TID of the client
     * @return std::unordered_set<int64_t>*:\n
     *             - Pointer to an unordered_set containing the requests.
     */
    std::unordered_set<int64_t>* getRequestsByClientID(pid_t clientTID);

    /**
     * @brief This method is called by the RequestMap to insert a new Request (represented by it's handle)
     *        for the client with the given ID in the Client Data Table.
     * @param clientTID Process TID of the client
     * @param requestHandle Handle of the Request
     */
    void insertRequestByClientId(pid_t clientTID, int64_t requestHandle);

    /**
     * @brief This method is called by the RequestMap to delete a Request (represented by it's handle)
     *        for the client with the given ID in the Client Data Table.
     * @param clientTID Process TID of the client
     * @param requestHandle Handle of the Request
     */
    void deleteRequestByClientId(pid_t clientTID, int64_t requestHandle);

    /**
     * @brief This method is called by the RateLimiter to fetch the current health for a given
     *        client in the Client Data Table.
     * @param clientPID Process ID of the client
     * @return double:\n
     *           - Health of the Client.
     */
    double getHealthByClientID(pid_t clientTID);

    /**
     * @brief This method is called by the RateLimiter to fetch the Last Request Timestamp for a given
     *        client in the Client Data Table.
     * @param clientTID TID of the client
     * @return int64_t:\n
     *             - Timestamp of Last Request (A value of 0, indicates no prior Requests).
     */
    int64_t getLastRequestTimestampByClientID(pid_t clientTID);

    /**
     * @brief This method is called by the RateLimiter to update the current health for a given
     *        client in the Client Data Table.
     * @param clientTID TID of the client
     * @param health Update value of the Health for the client
     */
    void updateHealthByClientID(pid_t clientTID, double health);

    /**
     * @brief This method is called by the RateLimiter to update the Last Request Timestamp for a given
     *        client in the Client Data Table.
     * @param clientTID TID of the client
     * @param currentMillis New value for the Last Request Timestamp for the client
     */
    void updateLastRequestTimestampByClientID(pid_t clientTID, int64_t currentMillis);

    /**
     * @brief This method is called by the Verifier to fetch the Permission Level for a given
     *        client in the Client Data Table, i.e. whether the client has SYSTEM (Root) or THIRD_PARTY (User) permissions.
     * @param clientPID Process ID of the client
     * @return int8_t:\n
     *            - PERMISSION_SYSTEM: If the client has System level access\n
     *            - PERMISSION_THIRD_PARTY: If the Client has Third Party level access\n
     *            - -1: If the Client could not be determined.
     */
    int8_t getClientLevelByID(pid_t clientPID);

    void getThreadsByClientId(pid_t clientPID, std::vector<pid_t>& threadIDs);

    /**
     * @brief This method is called by the PulseMonitor to fetch the list of all active clients.
     * @param clientList An IN/OUT parameter to store the list of active clients.
     */
    void getActiveClientList(std::vector<pid_t>& clientList);

    /**
     * @brief Delete a client PID Entry from the Client Table.
     * @param clientPID Process ID of the client
     */
    void deleteClientPID(pid_t clientPID);

    /**
     * @brief Delete a client TID Entry from the Client TID Data Table.
     * @param clientTID TID of the client
     */
    void deleteClientTID(pid_t clientTID);

    static std::shared_ptr<ClientDataManager> getInstance() {
        if(mClientDataManagerInstance == nullptr) {
            instanceProtectionLock.lock();
            if(mClientDataManagerInstance == nullptr) {
                try {
                    mClientDataManagerInstance = std::shared_ptr<ClientDataManager> (new ClientDataManager());
                } catch(const std::bad_alloc& e) {
                    instanceProtectionLock.unlock();
                    return nullptr;
                }
            }
            instanceProtectionLock.unlock();
        }
        return mClientDataManagerInstance;
    }
};

#endif
