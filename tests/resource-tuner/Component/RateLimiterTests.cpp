// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <vector>
#include <memory>
#include <unordered_set>
#include <cstdint>
#include <limits>
#include <chrono>
#include <thread>
#include <new>       
#include <iostream>

#define MTEST_NO_MAIN
#include "../framework/mini.h"

#include "TestUtils.h"
#include "RequestManager.h"
#include "RateLimiter.h"
#include "TestAggregator.h"

// ---------- Init (unchanged) ----------
static void Init() {
    MakeAlloc<ClientInfo> (30);
    MakeAlloc<ClientTidData> (30);
    MakeAlloc<std::unordered_set<int64_t>> (30);
    MakeAlloc<Resource> (120);
    MakeAlloc<Request> (100);

    UrmSettings::metaConfigs.mDelta         = 1000;
    UrmSettings::metaConfigs.mPenaltyFactor = 2.0;
    UrmSettings::metaConfigs.mRewardFactor  = 0.4;
}

// ---------- Helper methods for Resource Generation (unchanged) ----------
static Resource* generateResourceForTesting(int32_t seed) {
    Resource* resource = (Resource*)malloc(sizeof(Resource));
    resource->setResCode(16 + seed);
    resource->setNumValues(1);
    resource->setValueAt(0, 2 * seed);
    return resource;
}

// ---------- Tests (ported one-to-one; no lambdas introduced) ----------

// TestClientSpammingScenario
MT_TEST(Component, TestClientSpammingScenario, "ratelimiter") {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RateLimiter>       rateLimiter       = RateLimiter::getInstance();

    int32_t clientPID = 999;
    int32_t clientTID = 999;

    std::vector<Request*> requests;

    try {
        // Generate 51 different requests from the same client
        for (int32_t i = 0; i < 51; i++) {
            Request* request = new (GetBlock<Request>()) Request;
            request->setRequestType(REQ_RESOURCE_TUNING);
            request->setHandle(300 + i);
            request->setDuration(-1);
            request->setPriority(REQ_PRIORITY_HIGH);
            request->setClientPID(clientPID);
            request->setClientTID(clientTID);

            if (!clientDataManager->clientExists(request->getClientPID(), request->getClientTID())) {
                clientDataManager->createNewClient(request->getClientPID(), request->getClientTID());
            }

            requests.push_back(request);
        }

        // Add first 50 requests — should be accepted
        for (int32_t i = 0; i < 50; i++) {
            int8_t result = rateLimiter->isRateLimitHonored(requests[i]->getClientTID());
            MT_REQUIRE(ctx, result == true);
        }

        // Add 51st request — should be rejected
        int8_t result = rateLimiter->isRateLimitHonored(requests[50]->getClientTID());
        MT_REQUIRE(ctx, result == false);

    } catch (const std::bad_alloc& /*e*/) {
        // original test ignored exceptions (no logic change)
    }

    clientDataManager->deleteClientPID(clientPID);
    clientDataManager->deleteClientTID(clientTID);

    // Cleanup
    for (Request* req : requests) {
        Request::cleanUpRequest(req);
    }
}

// TestClientHealthInCaseOfGoodRequests
MT_TEST(Component, TestClientHealthInCaseOfGoodRequests, "ratelimiter") {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RateLimiter>       rateLimiter       = RateLimiter::getInstance();

    int32_t clientPID = 999;
    int32_t clientTID = 999;

    std::vector<Request*> requests;

    try {
        // Generate 50 different requests from the same client
        for (int32_t i = 0; i < 50; i++) {
            Request* req = new (GetBlock<Request>()) Request;
            req->setRequestType(REQ_RESOURCE_TUNING);
            req->setHandle(300 + i);
            req->setDuration(-1);
            req->setPriority(REQ_PRIORITY_HIGH);
            req->setClientPID(clientPID);
            req->setClientTID(clientTID);

            if (!clientDataManager->clientExists(req->getClientPID(), req->getClientTID())) {
                clientDataManager->createNewClient(req->getClientPID(), req->getClientTID());
            }

            requests.push_back(req);

            // Original timing preserved (2s between some requests)
            std::this_thread::sleep_for(std::chrono::seconds(2));

            int8_t isRateLimitHonored = rateLimiter->isRateLimitHonored(req->getClientTID());
            MT_REQUIRE(ctx, isRateLimitHonored == true);
            MT_REQUIRE_EQ(ctx, clientDataManager->getHealthByClientID(req->getClientTID()), 100);
        }

    } catch (const std::bad_alloc& /*e*/) {
        // original test ignored exceptions
    }

    clientDataManager->deleteClientPID(clientPID);
    clientDataManager->deleteClientTID(clientTID);

    // Cleanup
    for (Request* req : requests) {
        Request::cleanUpRequest(req);
    }
}

// TestClientSpammingWithGoodRequests
MT_TEST(Component, TestClientSpammingWithGoodRequests, "ratelimiter") {
    Init();
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RateLimiter>       rateLimiter       = RateLimiter::getInstance();

    int32_t clientPID = 999;
    int32_t clientTID = 999;

    std::vector<Request*> requests;

    // Generate 63 different requests from the same client
    try {
        for (int32_t i = 0; i < 63; i++) {
            Request* req = new (GetBlock<Request>()) Request;
            req->setRequestType(REQ_RESOURCE_TUNING);
            req->setHandle(300 + i);
            req->setDuration(-1);
            req->setPriority(REQ_PRIORITY_HIGH);
            req->setClientPID(clientPID);
            req->setClientTID(clientTID);

            if (!clientDataManager->clientExists(req->getClientPID(), req->getClientTID())) {
                clientDataManager->createNewClient(req->getClientPID(), req->getClientTID());
            }
            requests.push_back(req);
        }

        // Add first 61 requests — should be accepted
        for (int32_t i = 0; i < 61; i++) {
            if (i % 5 == 0 && i < 50) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            int8_t result = rateLimiter->isRateLimitHonored(requests[i]->getClientTID());
            MT_REQUIRE(ctx, result == true);
        }

        // Add 62th request — should be rejected
        int8_t result = rateLimiter->isRateLimitHonored(requests[61]->getClientTID());
        MT_REQUIRE(ctx, result == false);

    } catch (const std::bad_alloc& /*e*/) {
        // original test ignored exceptions
    }

    clientDataManager->deleteClientPID(clientPID);
    clientDataManager->deleteClientTID(clientTID);

    // Cleanup
    for (Request* req : requests) {
        Request::cleanUpRequest(req);
    }
}

