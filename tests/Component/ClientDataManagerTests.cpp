// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ErrCodes.h"
#include "TestUtils.h"
#include "AuxRoutines.h"
#include "URMTests.h"
#include "ClientDataManager.h"

#define TEST_CLASS "COMPONENT"

static void Init() {
    static int8_t initDone = false;
    if(!initDone) {
        initDone = true;

        MakeAlloc<ClientInfo> (30);
        MakeAlloc<ClientTidData> (30);
        MakeAlloc<std::unordered_set<int64_t>> (30);
    }
}

URM_TEST(TestClientDataManagerClientEntryCreation1, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == false));
    clientDataManager->createNewClient(testClientPID, testClientTID);
    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == true));

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
})

// Use threads to simulate different clients (PIDs essentially)
URM_TEST(TestClientDataManagerClientEntryCreation2, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    std::vector<std::thread> threads;
    std::vector<int32_t> clientPIDs;

    for(int32_t i = 0; i < 10; i++) {
        auto threadRoutine = [&] (void* arg) {
            int32_t id = *(int32_t*) arg;

            E_ASSERT((clientDataManager->clientExists(id, id) == false));
            clientDataManager->createNewClient(id, id);
            E_ASSERT((clientDataManager->clientExists(id, id) == true));

            free(arg);
        };

        int32_t* ptr = (int32_t*) malloc(sizeof(int32_t));
        *ptr = i + 1;

        threads.push_back(std::thread(threadRoutine, ptr));
        clientPIDs.push_back(i + 1);
    }

    for(int32_t i = 0; i < (int32_t)threads.size(); i++) {
        threads[i].join();
    }

    for(int32_t clientID: clientPIDs) {
        clientDataManager->deleteClientPID(clientID);
        clientDataManager->deleteClientTID(clientID);
    }
})

URM_TEST(TestClientDataManagerClientEntryDeletion, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == false));
    clientDataManager->createNewClient(testClientPID, testClientTID);
    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == true));

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == false));
})

URM_TEST(TestClientDataManagerRateLimiterUtilsHealth, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == false));
    clientDataManager->createNewClient(testClientPID, testClientTID);
    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == true));

    double health = clientDataManager->getHealthByClientID(testClientTID);
    E_ASSERT((health == 100.0));

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
})

URM_TEST(TestClientDataManagerRateLimiterUtilsHealthSetGet, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == false));
    clientDataManager->createNewClient(testClientPID, testClientTID);
    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == true));

    clientDataManager->updateHealthByClientID(testClientTID, 55);
    double health = clientDataManager->getHealthByClientID(testClientTID);

    E_ASSERT((health == 55.0));

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
})

URM_TEST(TestClientDataManagerRateLimiterUtilsLastRequestTimestampSetGet, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == false));
    clientDataManager->createNewClient(testClientPID, testClientTID);
    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == true));

    int64_t currentMillis = AuxRoutines::getCurrentTimeInMilliseconds();

    clientDataManager->updateLastRequestTimestampByClientID(testClientTID, currentMillis);
    int64_t lastRequestTimestamp = clientDataManager->getLastRequestTimestampByClientID(testClientTID);

    E_ASSERT((lastRequestTimestamp == currentMillis));

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
})

URM_TEST(TestClientDataManagerPulseMonitorClientListFetch, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    // Insert a few clients into the table
    for(int32_t i = 100; i < 120; i++) {
        E_ASSERT((clientDataManager->clientExists(i, i) == false));
        clientDataManager->createNewClient(i, i);
        E_ASSERT((clientDataManager->clientExists(i, i) == true));
    }

    std::vector<int32_t> clientList;
    clientDataManager->getActiveClientList(clientList);

    E_ASSERT((clientList.size() == 20));
    for(int32_t clientPID: clientList) {
        E_ASSERT((clientPID < 120));
        E_ASSERT((clientPID >= 100));
    }

    for(int32_t i = 100; i < 120; i++) {
        clientDataManager->deleteClientPID(i);
        clientDataManager->deleteClientTID(i);
    }
})

URM_TEST(TestClientDataManagerRequestMapInsertion, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == false));
    clientDataManager->createNewClient(testClientPID, testClientTID);
    E_ASSERT((clientDataManager->clientExists(testClientPID, testClientTID) == true));

    for(int32_t i = 0; i < 20; i++) {
        clientDataManager->insertRequestByClientId(testClientTID, i + 1);
    }

    std::unordered_set<int64_t>* clientRequests =
                    clientDataManager->getRequestsByClientID(testClientTID);

    E_ASSERT((clientRequests != nullptr));
    E_ASSERT((clientRequests->size() == 20));

    for(int32_t i = 0; i < 20; i++) {
        clientDataManager->deleteRequestByClientId(testClientTID, i + 1);
    }

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);

    E_ASSERT((clientRequests->size() == 0));
})

URM_TEST(TestClientDataManagerClientThreadTracking1, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    std::vector<std::thread> clientThreads;

    for(int32_t i = 0; i < 20; i++) {
        auto threadRoutine = [&](void* arg) {
            int32_t threadID = *(int32_t*)arg;
            E_ASSERT((clientDataManager->clientExists(testClientPID, threadID) == false));
            clientDataManager->createNewClient(testClientPID, threadID);
            E_ASSERT((clientDataManager->clientExists(testClientPID, threadID) == true));

            free(arg);
        };

        int32_t* index = (int32_t*)malloc(sizeof(int32_t));
        *index = i + 1;
        clientThreads.push_back(std::thread(threadRoutine, index));
    }

    for(int32_t i = 0; i < (int32_t)clientThreads.size(); i++) {
        clientThreads[i].join();
    }

    std::vector<int32_t> threadIds;
    clientDataManager->getThreadsByClientId(testClientPID, threadIds);
    E_ASSERT((threadIds.size() == 20));

    clientDataManager->deleteClientPID(testClientPID);

    for(int32_t i = 0; i < 20; i++) {
        clientDataManager->deleteClientTID(i + 1);
    }
})

URM_TEST(TestClientDataManagerClientThreadTracking2, {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;

    for(int32_t i = 0; i < 20; i++) {
        E_ASSERT((clientDataManager->clientExists(testClientPID, i + 1) == false));
        clientDataManager->createNewClient(testClientPID, i + 1);
        E_ASSERT((clientDataManager->clientExists(testClientPID, i + 1) == true));
    }

    std::vector<int32_t> threadIds;
    clientDataManager->getThreadsByClientId(testClientPID, threadIds);
    E_ASSERT((threadIds.size() == 20));

    for(int32_t i = 0; i < 20; i++) {
        clientDataManager->insertRequestByClientId(i + 1, 5 * i + 7);
    }

    for(int32_t i = 0; i < 20; i++) {
        std::unordered_set<int64_t>* clientRequests =
                    clientDataManager->getRequestsByClientID(i + 1);

        E_ASSERT((clientRequests != nullptr));
        E_ASSERT((clientRequests->size() == 1));

        clientDataManager->deleteRequestByClientId(i + 1, 5 * i + 7);
    }

    for(int32_t i = 0; i < 20; i++) {
        std::unordered_set<int64_t>* clientRequests =
                clientDataManager->getRequestsByClientID(i + 1);

        E_ASSERT((clientRequests != nullptr));
        E_ASSERT((clientRequests->size() == 0));
    }

    clientDataManager->deleteClientPID(testClientPID);

    for(int32_t i = 0; i < 20; i++) {
        clientDataManager->deleteClientTID(i + 1);
    }
})
