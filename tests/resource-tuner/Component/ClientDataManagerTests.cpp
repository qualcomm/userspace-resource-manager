// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ErrCodes.h"
#include "TestUtils.h"
#include "AuxRoutines.h"
#include "ClientDataManager.h"
#include "TestAggregator.h"
#include "MemoryPool.h"   
#include <vector>
#include <thread>
#include <unordered_set>
#include <memory>
#include <cstdint>
#include <cstdlib>        
#include <iostream>

#define MTEST_NO_MAIN
#include "../framework/mini.h"


using namespace mtest;

// -------------------------
// Original Init() preserved
// -------------------------
static void Init() {
    MakeAlloc<ClientInfo>(30);
    MakeAlloc<ClientTidData>(30);
    MakeAlloc<std::unordered_set<int64_t>>(30);
}

// Ensure Init() runs exactly once (replaces old RunTests() orchestration)
static void EnsureInit() {
    static bool done = false;
    if (!done) { Init(); done = true; }
}

// -------------------------
// Thread helpers (NO lambdas)
// -------------------------

// Routine for TestClientDataManagerClientEntryCreation2:
// arg points to a heap-allocated int32_t (id)
static void ThreadRoutine_CreateClient(void* arg, std::shared_ptr<ClientDataManager> mgr) {
    int32_t id = *(int32_t*)arg;
    // Pre-conditions and actions as in original test
    if (!mgr->clientExists(id, id)) {
        mgr->createNewClient(id, id);
    }
    // Clean up thread argument
    std::free(arg);
}

// Routine for TestClientDataManagerClientThreadTracking1:
// arg points to a heap-allocated int32_t (threadID)
static void ThreadRoutine_CreateClientUnderPID(void* arg,
                                               std::shared_ptr<ClientDataManager> mgr,
                                               int32_t pid) {
    int32_t threadID = *(int32_t*)arg;
    if (!mgr->clientExists(pid, threadID)) {
        mgr->createNewClient(pid, threadID);
    }
    std::free(arg);
}

// Wrapper function to use with std::thread (since std::thread needs a callable signature)
static void ThreadStart_CreateClient(void* arg,
                                     std::shared_ptr<ClientDataManager> mgr) {
    ThreadRoutine_CreateClient(arg, mgr);
}

static void ThreadStart_CreateClientUnderPID(void* arg,
                                             std::shared_ptr<ClientDataManager> mgr,
                                             int32_t pid) {
    ThreadRoutine_CreateClientUnderPID(arg, mgr, pid);
}

// ---------------------------
// Suite: Component 
// Tag:  clientdatamanager
// ---------------------------

MT_TEST(Component, TestClientDataManagerClientEntryCreation1, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), false);
    clientDataManager->createNewClient(testClientPID, testClientTID);
    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), true);

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
}

MT_TEST(Component, TestClientDataManagerClientEntryCreation2, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    std::vector<std::thread> threads;
    std::vector<int32_t> clientPIDs;

    for (int32_t i = 0; i < 10; i++) {
        int32_t* ptr = (int32_t*)std::malloc(sizeof(int32_t));
        *ptr = i + 1;

        // Start thread using free function (no lambdas)
        threads.emplace_back(ThreadStart_CreateClient, (void*)ptr, clientDataManager);

        clientPIDs.push_back(i + 1);
    }

    for (size_t i = 0; i < threads.size(); i++) {
        threads[i].join();
    }

    // Verify results in the main test thread
    for (int32_t clientID : clientPIDs) {
        MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(clientID, clientID), true);
    }

    for (int32_t clientID : clientPIDs) {
        clientDataManager->deleteClientPID(clientID);
        clientDataManager->deleteClientTID(clientID);
    }
}

MT_TEST(Component, TestClientDataManagerClientEntryDeletion, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), false);
    clientDataManager->createNewClient(testClientPID, testClientTID);
    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), true);

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), false);
}

MT_TEST(Component, TestClientDataManagerRateLimiterUtilsHealth, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), false);
    clientDataManager->createNewClient(testClientPID, testClientTID);
    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), true);

    double health = clientDataManager->getHealthByClientID(testClientTID);
    MT_REQUIRE_EQ(ctx, health, 100.0);

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
}

MT_TEST(Component, TestClientDataManagerRateLimiterUtilsHealthSetGet, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), false);
    clientDataManager->createNewClient(testClientPID, testClientTID);
    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), true);

    clientDataManager->updateHealthByClientID(testClientTID, 55);
    double health = clientDataManager->getHealthByClientID(testClientTID);

    MT_REQUIRE_EQ(ctx, health, 55.0);

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
}

MT_TEST(Component, TestClientDataManagerRateLimiterUtilsLastRequestTimestampSetGet, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), false);
    clientDataManager->createNewClient(testClientPID, testClientTID);
    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), true);

    int64_t currentMillis = AuxRoutines::getCurrentTimeInMilliseconds();

    clientDataManager->updateLastRequestTimestampByClientID(testClientTID, currentMillis);
    int64_t lastRequestTimestamp = clientDataManager->getLastRequestTimestampByClientID(testClientTID);

    MT_REQUIRE_EQ(ctx, lastRequestTimestamp, currentMillis);

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);
}

MT_TEST(Component, TestClientDataManagerPulseMonitorClientListFetch, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    // Insert a few clients into the table
    for (int32_t i = 100; i < 120; i++) {
        MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(i, i), false);
        clientDataManager->createNewClient(i, i);
        MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(i, i), true);
    }

    std::vector<int32_t> clientList;
    clientDataManager->getActiveClientList(clientList);

    MT_REQUIRE_EQ(ctx, static_cast<int>(clientList.size()), 20);
    for (int32_t clientPID : clientList) {
        MT_REQUIRE(ctx, clientPID < 120);
        MT_REQUIRE(ctx, clientPID >= 100);
    }

    for (int32_t i = 100; i < 120; i++) {
        clientDataManager->deleteClientPID(i);
        clientDataManager->deleteClientTID(i);
    }
}

MT_TEST(Component, TestClientDataManagerRequestMapInsertion, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    int32_t testClientTID = 252;

    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), false);
    clientDataManager->createNewClient(testClientPID, testClientTID);
    MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, testClientTID), true);

    for (int32_t i = 0; i < 20; i++) {
        clientDataManager->insertRequestByClientId(testClientTID, i + 1);
    }

    std::unordered_set<int64_t>* clientRequests =
        clientDataManager->getRequestsByClientID(testClientTID);

    MT_REQUIRE(ctx, clientRequests != nullptr);
    MT_REQUIRE_EQ(ctx, static_cast<int>(clientRequests->size()), 20);

    for (int32_t i = 0; i < 20; i++) {
        clientDataManager->deleteRequestByClientId(testClientTID, i + 1);
    }

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);

    MT_REQUIRE_EQ(ctx, static_cast<int>(clientRequests->size()), 0);
}

MT_TEST(Component, TestClientDataManagerClientThreadTracking1, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;
    std::vector<std::thread> clientThreads;

    for (int32_t i = 0; i < 20; i++) {
        int32_t* index = (int32_t*)std::malloc(sizeof(int32_t));
        *index = i + 1;
        // Start thread using free function (no lambdas)
        clientThreads.emplace_back(ThreadStart_CreateClientUnderPID,
                                   (void*)index,
                                   clientDataManager,
                                   testClientPID);
    }

    for (size_t i = 0; i < clientThreads.size(); i++) {
        clientThreads[i].join();
    }

    std::vector<int32_t> threadIds;
    clientDataManager->getThreadsByClientId(testClientPID, threadIds);
    MT_REQUIRE_EQ(ctx, static_cast<int>(threadIds.size()), 20);

    clientDataManager->deleteClientPID(testClientPID);

    for (int32_t i = 0; i < 20; i++) {
        clientDataManager->deleteClientTID(i + 1);
    }
}

MT_TEST(Component, TestClientDataManagerClientThreadTracking2, "clientdatamanager") {
    EnsureInit();

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();

    int32_t testClientPID = 252;

    for (int32_t i = 0; i < 20; i++) {
        MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, i + 1), false);
        clientDataManager->createNewClient(testClientPID, i + 1);
        MT_REQUIRE_EQ(ctx, clientDataManager->clientExists(testClientPID, i + 1), true);
    }

    std::vector<int32_t> threadIds;
    clientDataManager->getThreadsByClientId(testClientPID, threadIds);
    MT_REQUIRE_EQ(ctx, static_cast<int>(threadIds.size()), 20);

    for (int32_t i = 0; i < 20; i++) {
        clientDataManager->insertRequestByClientId(i + 1, 5 * i + 7);
    }

    for (int32_t i = 0; i < 20; i++) {
        std::unordered_set<int64_t>* clientRequests =
            clientDataManager->getRequestsByClientID(i + 1);

        MT_REQUIRE(ctx, clientRequests != nullptr);
        MT_REQUIRE_EQ(ctx, static_cast<int>(clientRequests->size()), 1);

        clientDataManager->deleteRequestByClientId(i + 1, 5 * i + 7);
    }

    for (int32_t i = 0; i < 20; i++) {
        std::unordered_set<int64_t>* clientRequests =
            clientDataManager->getRequestsByClientID(i + 1);

        MT_REQUIRE(ctx, clientRequests != nullptr);
        MT_REQUIRE_EQ(ctx, static_cast<int>(clientRequests->size()), 0);
    }

    clientDataManager->deleteClientPID(testClientPID);

    for (int32_t i = 0; i < 20; i++) {
        clientDataManager->deleteClientTID(i + 1);
    }
}


