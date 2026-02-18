// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "TestUtils.h"
#include "RequestManager.h"
#include "RateLimiter.h"
#include "MemoryPool.h"
#include "TestAggregator.h"

static void Init() {
    static int8_t initDone = false;
    if(!initDone) {
        initDone = true;

        MakeAlloc<ClientInfo> (30);
        MakeAlloc<ClientTidData> (30);
        MakeAlloc<std::unordered_set<int64_t>> (30);
        MakeAlloc<Resource> (30);
        MakeAlloc<ResIterable> (30);
        MakeAlloc<Request> (30);
    }
}

// Helper methods for Resource Generation
static Resource* generateResourceForTesting(int32_t seed) {
    Resource* resource = nullptr;
    try {
        resource = (Resource*) GetBlock<Resource>();
        resource->setResCode(16 + seed);
        resource->setNumValues(1);
        resource->setValueAt(0, 2 * seed);
    } catch(const std::bad_alloc& e) {
        throw std::bad_alloc();
    }

    return resource;
}

Resource* generateResourceFromMemoryPoolForTesting(int32_t seed) {
    Resource* resource = new(GetBlock<Resource>()) Resource;
    resource->setResCode(16 + seed);
    resource->setNumValues(1);
    resource->setValueAt(0, 2 * seed);

    return resource;
}

// No prior requests in the map, add a new one
// The request should be accepted
static void TestSingleRequestScenario() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource* resource = MPLACED(Resource);
    resource->setResCode(16);
    resource->setNumValues(1);
    resource->setValueAt(0, 8);

    ResIterable* resIterable = MPLACED(ResIterable);
    resIterable->mData = resource;

    Request* request = new (GetBlock<Request>()) Request;
    request->setRequestType(REQ_RESOURCE_TUNING);
    request->setHandle(25);
    request->setDuration(-1);
    request->setPriority(REQ_PRIORITY_HIGH);
    request->setClientPID(321);
    request->setClientTID(321);
    request->setBackgroundProcessing(false);
    request->addResource(resIterable);

    if(!clientDataManager->clientExists(request->getClientPID(), request->getClientTID())) {
        clientDataManager->createNewClient(request->getClientPID(), request->getClientTID());
    }

    int8_t result = requestMap->shouldRequestBeAdded(request);

    C_ASSERT(result == true);

    clientDataManager->deleteClientPID(request->getClientPID());
    clientDataManager->deleteClientTID(request->getClientTID());

    Request::cleanUpRequest(request);
}

// Add duplicate requests. The second request should not be accepted
static void TestDuplicateRequestScenario1() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource* resource1 = generateResourceForTesting(1);
    Resource* resource2 = generateResourceForTesting(1);

    ResIterable* resIterable1 = MPLACED(ResIterable);
    ResIterable* resIterable2 = MPLACED(ResIterable);
    resIterable1->mData = resource1;
    resIterable2->mData = resource2;

    Request* firstRequest = new (GetBlock<Request>()) Request;
    firstRequest->setRequestType(REQ_RESOURCE_TUNING);
    firstRequest->setHandle(20);
    firstRequest->setDuration(-1);
    firstRequest->setPriority(REQ_PRIORITY_HIGH);
    firstRequest->addResource(resIterable1);
    firstRequest->setClientPID(321);
    firstRequest->setClientTID(321);
    firstRequest->setBackgroundProcessing(false);

    Request* secondRequest = new (GetBlock<Request>()) Request;
    secondRequest->setRequestType(REQ_RESOURCE_TUNING);
    secondRequest->setHandle(21);
    secondRequest->setDuration(-1);
    secondRequest->setPriority(REQ_PRIORITY_HIGH);
    secondRequest->addResource(resIterable2);
    secondRequest->setClientPID(321);
    secondRequest->setClientTID(321);
    secondRequest->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(firstRequest->getClientPID(), firstRequest->getClientTID())) {
        clientDataManager->createNewClient(firstRequest->getClientPID(), firstRequest->getClientTID());
    }

    int8_t resultFirst = requestMap->shouldRequestBeAdded(firstRequest);
    if(resultFirst) {
        requestMap->addRequest(firstRequest);
    }

    int8_t resultSecond = requestMap->shouldRequestBeAdded(secondRequest);
    if(resultSecond) {
        requestMap->addRequest(secondRequest);
    }

    C_ASSERT((resultFirst == true));
    C_ASSERT((resultSecond == false));

    requestMap->removeRequest(firstRequest);
    requestMap->removeRequest(secondRequest);

    clientDataManager->deleteClientPID(firstRequest->getClientPID());
    clientDataManager->deleteClientTID(firstRequest->getClientTID());

    Request::cleanUpRequest(firstRequest);
    Request::cleanUpRequest(secondRequest);
}

// Add duplicate requests with multiple resources. The second request should not be accepted
static void TestDuplicateRequestScenario2() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource* resource1 = generateResourceForTesting(1);
    Resource* resource2 = generateResourceForTesting(2);
    Resource* resource3 = generateResourceForTesting(1);
    Resource* resource4 = generateResourceForTesting(2);

    ResIterable* resIter1 = MPLACED(ResIterable);
    ResIterable* resIter2 = MPLACED(ResIterable);
    ResIterable* resIter3 = MPLACED(ResIterable);
    ResIterable* resIter4 = MPLACED(ResIterable);
    resIter1->mData = resource1;
    resIter2->mData = resource2;
    resIter3->mData = resource3;
    resIter4->mData = resource4;

    Request* firstRequest = MPLACED(Request);
    firstRequest->setRequestType(REQ_RESOURCE_TUNING);
    firstRequest->setHandle(103);
    firstRequest->setDuration(-1);
    firstRequest->setPriority(REQ_PRIORITY_HIGH);
    firstRequest->setClientPID(321);
    firstRequest->setClientTID(321);
    firstRequest->addResource(resIter1);
    firstRequest->addResource(resIter2);
    firstRequest->setBackgroundProcessing(false);

    Request* secondRequest = MPLACED(Request);
    secondRequest->setRequestType(REQ_RESOURCE_TUNING);
    secondRequest->setHandle(108);
    secondRequest->setDuration(-1);
    secondRequest->setPriority(REQ_PRIORITY_HIGH);
    secondRequest->setClientPID(321);
    secondRequest->setClientTID(321);
    secondRequest->addResource(resIter3);
    secondRequest->addResource(resIter4);
    secondRequest->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(firstRequest->getClientPID(), firstRequest->getClientTID())) {
        clientDataManager->createNewClient(firstRequest->getClientPID(), firstRequest->getClientTID());
    }

    int8_t resultFirst = requestMap->shouldRequestBeAdded(firstRequest);
    if(resultFirst) {
        requestMap->addRequest(firstRequest);
    }

    int8_t resultSecond = requestMap->shouldRequestBeAdded(secondRequest);
    if(resultSecond) {
        requestMap->addRequest(secondRequest);
    }

    C_ASSERT((resultFirst == true));
    C_ASSERT((resultSecond == false));

    requestMap->removeRequest(firstRequest);
    requestMap->removeRequest(secondRequest);

    clientDataManager->deleteClientPID(firstRequest->getClientPID());
    clientDataManager->deleteClientTID(firstRequest->getClientTID());

    Request::cleanUpRequest(firstRequest);
    Request::cleanUpRequest(secondRequest);
}

// For 2 requests to be considered duplicate, each and every one of their
// attributes should match. Else the request should be accepted
static void TestDuplicateRequestScenario3_1() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    std::vector<Request*> requestsCreated;

    for(int32_t index = 0; index < 10; index++) {
        Resource* resource = generateResourceForTesting(0);

        // Slight modification
        resource->setValueAt(0, 8 + index);
        ResIterable* resIter = MPLACED(ResIterable);
        resIter->mData = resource;

        Request* request = MPLACED(Request);
        request->setRequestType(REQ_RESOURCE_TUNING);
        request->setHandle(112 + index);
        request->setDuration(-1);
        request->setPriority(REQ_PRIORITY_HIGH);
        request->setClientPID(321);
        request->setClientTID(321);
        request->addResource(resIter);
        request->setBackgroundProcessing(false);

        if(!clientDataManager->clientExists(request->getClientPID(), request->getClientTID())) {
            clientDataManager->createNewClient(request->getClientPID(), request->getClientTID());
        }

        requestsCreated.push_back(request);

        int8_t requestCheck = requestMap->shouldRequestBeAdded(request);
        C_ASSERT((requestCheck == true));
        requestMap->addRequest(request);
    }

    clientDataManager->deleteClientPID(321);
    clientDataManager->deleteClientTID(321);

    for(int32_t i = 0; i < requestsCreated.size(); i++) {
        requestMap->removeRequest(requestsCreated[i]);
        Request::cleanUpRequest(requestsCreated[i]);
    }
}

// Duplicate Verification check, where the number of resources itself is different
// in the second request, hence it should be accepted.
static void TestDuplicateRequestScenario3_2() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource *resource1, *resource2, *duplicateResource1;
    try {
        resource1 = generateResourceForTesting(1);
        duplicateResource1 = generateResourceForTesting(1);
        resource2 = generateResourceForTesting(2);
    } catch(const std::bad_alloc& e) {
        return;
    }

    ResIterable* resIter1 = MPLACED(ResIterable);
    resIter1->mData = resource1;

    ResIterable* resIter2 = MPLACED(ResIterable);
    resIter2->mData = duplicateResource1;

    ResIterable* resIter3 = MPLACED(ResIterable);
    resIter3->mData = resource2;

    Request* firstRequest = MPLACED(Request);
    firstRequest->setRequestType(REQ_RESOURCE_TUNING);
    firstRequest->setHandle(245);
    firstRequest->setDuration(-1);
    firstRequest->setPriority(REQ_PRIORITY_HIGH);
    firstRequest->setClientPID(321);
    firstRequest->setClientTID(321);
    firstRequest->addResource(resIter1);
    firstRequest->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(firstRequest->getClientPID(), firstRequest->getClientTID())) {
        if(!clientDataManager->createNewClient(firstRequest->getClientPID(), firstRequest->getClientTID())) {
            return;
        }
    }

    int8_t resultFirst = requestMap->shouldRequestBeAdded(firstRequest);
    if(resultFirst) {
        requestMap->addRequest(firstRequest);
    }

    Request* secondRequest;
    try {
        secondRequest = new (GetBlock<Request>()) Request;
    } catch(const std::bad_alloc& e) {
        return;
    }

    secondRequest->setRequestType(REQ_RESOURCE_TUNING);
    secondRequest->setHandle(300);
    secondRequest->setDuration(-1);
    secondRequest->setPriority(REQ_PRIORITY_HIGH);
    secondRequest->setClientPID(321);
    secondRequest->setClientTID(321);
    secondRequest->addResource(resIter2);
    secondRequest->addResource(resIter3);
    secondRequest->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(secondRequest->getClientPID(), secondRequest->getClientTID())) {
        if(!clientDataManager->createNewClient(secondRequest->getClientPID(), secondRequest->getClientTID())) {
            return;
        }
    }

    int8_t resultSecond = requestMap->shouldRequestBeAdded(secondRequest);
    if(resultSecond) {
        requestMap->addRequest(secondRequest);
    }

    C_ASSERT((resultFirst == true));
    C_ASSERT((resultSecond == true));

    requestMap->removeRequest(firstRequest);
    requestMap->removeRequest(secondRequest);

    clientDataManager->deleteClientPID(firstRequest->getClientPID());
    clientDataManager->deleteClientTID(firstRequest->getClientTID());

    Request::cleanUpRequest(firstRequest);
    Request::cleanUpRequest(secondRequest);

}

// Add requests from the same client with multiple resources, where not all resources are identical.
// Both requests should be accepted
static void TestDuplicateRequestScenario4() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource* resource1;
    Resource* duplicateResource1;
    Resource* resource2;
    Resource* resource3;

    try {
        resource1 = generateResourceForTesting(1);
        duplicateResource1 = generateResourceForTesting(1);
        resource2 = generateResourceForTesting(2);
        resource3 = generateResourceForTesting(3);
    } catch(const std::bad_alloc& e) {
        return;
    }

    std::vector<Resource*>* resources1;
    try {
        resources1 = new (GetBlock<std::vector<Resource*>>())std::vector<Resource*>;
    } catch(const std::bad_alloc& e) {
        return;
    }

    std::vector<Resource*>* resources2;
    try {
        resources2 = new (GetBlock<std::vector<Resource*>>())std::vector<Resource*>;
    } catch(const std::bad_alloc& e) {
        return;
    }

    resources1->push_back(resource1);
    resources1->push_back(resource2);

    resources2->push_back(duplicateResource1);
    resources2->push_back(resource3);

    Request* firstRequest;
    try {
        firstRequest = new (GetBlock<Request>()) Request();
    } catch(const std::bad_alloc &e) {
        return;
    }

    firstRequest->setRequestType(REQ_RESOURCE_TUNING);
    firstRequest->setHandle(320);
    firstRequest->setDuration(-1);
    // firstRequest->setNumResources(2);
    firstRequest->setPriority(REQ_PRIORITY_HIGH);
    firstRequest->setClientPID(321);
    firstRequest->setClientTID(321);
    // firstRequest->setResources(resources1);
    firstRequest->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(firstRequest->getClientPID(), firstRequest->getClientTID())) {
        if(!clientDataManager->createNewClient(firstRequest->getClientPID(), firstRequest->getClientTID())) {
            return;
        }
    }

    int8_t resultFirst = requestMap->shouldRequestBeAdded(firstRequest);
    if(resultFirst) {
        requestMap->addRequest(firstRequest);
    }

    Request* secondRequest;
    try {
        secondRequest = new (GetBlock<Request>()) Request();
    } catch(const std::bad_alloc &e) {
        return;
    }

    secondRequest->setRequestType(REQ_RESOURCE_TUNING);
    secondRequest->setHandle(334);
    secondRequest->setDuration(-1);
    // secondRequest->setNumResources(2);
    secondRequest->setPriority(REQ_PRIORITY_HIGH);
    secondRequest->setClientPID(321);
    secondRequest->setClientTID(321);
    // secondRequest->setResources(resources2);
    secondRequest->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(secondRequest->getClientPID(), secondRequest->getClientTID())) {
        if(!clientDataManager->createNewClient(secondRequest->getClientPID(), secondRequest->getClientTID())) {
            return;
        }
    }

    int8_t resultSecond = requestMap->shouldRequestBeAdded(secondRequest);
    if(resultSecond) {
        requestMap->addRequest(secondRequest);
    }

    C_ASSERT(resultFirst == true);
    C_ASSERT(resultSecond == true);

    requestMap->removeRequest(firstRequest);
    requestMap->removeRequest(secondRequest);

    clientDataManager->deleteClientPID(firstRequest->getClientPID());
    clientDataManager->deleteClientTID(firstRequest->getClientTID());

    Request::cleanUpRequest(firstRequest);
    Request::cleanUpRequest(secondRequest);
}

// Multiple clients try to add requests which are duplicates of each other in
// terms of resources.
// All request should still be accepted, since clients are different
static void TestMultipleClientsScenario5() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    std::vector<Resource*> resource1;
    std::vector<Resource*> resource2;

    try {
        resource1.push_back(generateResourceForTesting(1));
        resource1.push_back(generateResourceForTesting(1));
        resource1.push_back(generateResourceForTesting(1));

        resource2.push_back(generateResourceForTesting(2));
        resource2.push_back(generateResourceForTesting(2));
        resource2.push_back(generateResourceForTesting(2));

    } catch(const std::bad_alloc& e) {
        return;
    }

    std::vector<Resource*>* resources1;
    try {
        resources1 = new (GetBlock<std::vector<Resource*>>())std::vector<Resource*>;
    } catch(const std::bad_alloc& e) {
        return;
    }

    std::vector<Resource*>* resources2;
    try {
        resources2 = new (GetBlock<std::vector<Resource*>>())std::vector<Resource*>;
    } catch(const std::bad_alloc& e) {
        return;
    }

    std::vector<Resource*>* resources3;
    try {
        resources3 = new (GetBlock<std::vector<Resource*>>())std::vector<Resource*>;
    } catch(const std::bad_alloc& e) {
        return;
    }

    resources1->push_back(resource1[0]);
    resources1->push_back(resource2[0]);

    resources2->push_back(resource1[1]);
    resources2->push_back(resource2[1]);

    resources3->push_back(resource1[2]);
    resources3->push_back(resource2[2]);

    Request* firstRequest;
    Request* secondRequest;
    Request* thirdRequest;

    try {
        firstRequest = new (GetBlock<Request>()) Request();
        secondRequest = new (GetBlock<Request>()) Request();
        thirdRequest = new (GetBlock<Request>()) Request();

    } catch(const std::bad_alloc &e) {
        return;
    }

    firstRequest->setRequestType(REQ_RESOURCE_TUNING);
    firstRequest->setHandle(133);
    firstRequest->setDuration(-1);
    firstRequest->setPriority(REQ_PRIORITY_HIGH);
    // firstRequest->setNumResources(1);
    firstRequest->setClientPID(321);
    firstRequest->setClientTID(321);
    // firstRequest->setResources(resources1);
    firstRequest->setBackgroundProcessing(false);

    secondRequest->setRequestType(REQ_RESOURCE_TUNING);
    secondRequest->setHandle(144);
    secondRequest->setDuration(-1);
    secondRequest->setPriority(REQ_PRIORITY_HIGH);
    // secondRequest->setNumResources(1);
    secondRequest->setClientPID(354);
    secondRequest->setClientTID(354);
    // secondRequest->setResources(resources2);
    secondRequest->setBackgroundProcessing(false);

    thirdRequest->setRequestType(REQ_RESOURCE_TUNING);
    thirdRequest->setHandle(155);
    thirdRequest->setDuration(-1);
    thirdRequest->setPriority(REQ_PRIORITY_HIGH);
    // thirdRequest->setNumResources(1);
    thirdRequest->setClientPID(100);
    thirdRequest->setClientTID(127);
    // thirdRequest->setResources(resources3);
    thirdRequest->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(firstRequest->getClientPID(), firstRequest->getClientTID())) {
        if(!clientDataManager->createNewClient(firstRequest->getClientPID(), firstRequest->getClientTID())) {
            return;
        }
    }

    if(!clientDataManager->clientExists(secondRequest->getClientPID(), secondRequest->getClientTID())) {
        if(!clientDataManager->createNewClient(secondRequest->getClientPID(), secondRequest->getClientTID())) {
            return;
        }
    }

    if(!clientDataManager->clientExists(thirdRequest->getClientPID(), thirdRequest->getClientTID())) {
        if(!clientDataManager->createNewClient(thirdRequest->getClientPID(), thirdRequest->getClientTID())) {
            return;
        }
    }

    int8_t resultFirst = requestMap->shouldRequestBeAdded(firstRequest);
    if(resultFirst) {
        requestMap->addRequest(firstRequest);
    }

    int8_t resultSecond = requestMap->shouldRequestBeAdded(secondRequest);
    if(resultSecond) {
        requestMap->addRequest(secondRequest);
    }

    int8_t resultThird = requestMap->shouldRequestBeAdded(thirdRequest);
    if(resultThird) {
        requestMap->addRequest(thirdRequest);
    }

    C_ASSERT(resultFirst == true);
    C_ASSERT(resultSecond == true);
    C_ASSERT(resultThird == true);

    requestMap->removeRequest(firstRequest);
    requestMap->removeRequest(secondRequest);
    requestMap->removeRequest(thirdRequest);

    clientDataManager->deleteClientPID(firstRequest->getClientPID());
    clientDataManager->deleteClientPID(secondRequest->getClientPID());
    clientDataManager->deleteClientPID(thirdRequest->getClientPID());
    clientDataManager->deleteClientTID(firstRequest->getClientTID());
    clientDataManager->deleteClientTID(secondRequest->getClientTID());
    clientDataManager->deleteClientTID(thirdRequest->getClientTID());

    Request::cleanUpRequest(firstRequest);
    Request::cleanUpRequest(secondRequest);
    Request::cleanUpRequest(thirdRequest);
}

// For retune / untune APIs, request with specified handle should be
// present in the RequestMap
static void TestRequestWithHandleExists1() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource* resource;
    try {
        resource = generateResourceForTesting(1);
    } catch(const std::bad_alloc& e) {
        return;
    }

    ResIterable* resIterable = MPLACED(ResIterable);
    resIterable->mData = resource;

    Request* request;
    try {
        request = new (GetBlock<Request>()) Request();
    } catch(const std::bad_alloc& e) {
        return;
    }

    request->setRequestType(REQ_RESOURCE_TUNING);
    request->setHandle(20);
    request->setDuration(-1);
    request->addResource(resIterable);
    request->setPriority(REQ_PRIORITY_HIGH);
    request->setClientTID(321);
    request->setClientTID(321);
    request->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(request->getClientPID(), request->getClientTID())) {
        if(!clientDataManager->createNewClient(request->getClientPID(), request->getClientTID())) {
            return;
        }
    }

    int8_t requestCheck = requestMap->shouldRequestBeAdded(request);
    if(requestCheck) {
        requestMap->addRequest(request);
    }
    C_ASSERT(requestCheck == true);

    int8_t result = requestMap->verifyHandle(20);
    C_ASSERT(result == true);

    requestMap->removeRequest(request);

    clientDataManager->deleteClientPID(request->getClientPID());
    clientDataManager->deleteClientTID(request->getClientTID());

    Request::cleanUpRequest(request);
}

static void TestRequestWithHandleExists2() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource* resource;
    try {
        resource = generateResourceForTesting(1);
    } catch(const std::bad_alloc& e) {
        return;
    }

    ResIterable* resIterable = MPLACED(ResIterable);
    resIterable->mData = resource;

    Request* request;
    try {
        request = new (GetBlock<Request>()) Request();
    } catch(const std::bad_alloc& e) {
        return;
    }

    request->setRequestType(REQ_RESOURCE_TUNING);
    request->setHandle(20);
    request->setDuration(-1);
    request->addResource(resIterable);
    request->setPriority(REQ_PRIORITY_HIGH);
    request->setClientTID(321);
    request->setClientTID(321);
    request->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(request->getClientPID(), request->getClientTID())) {
        if(!clientDataManager->createNewClient(request->getClientPID(), request->getClientTID())) {
            return;
        }
    }

    int8_t requestCheck = requestMap->shouldRequestBeAdded(request);
    if(requestCheck) {
        requestMap->addRequest(request);
    }
    C_ASSERT(requestCheck == true);

    int8_t result = requestMap->verifyHandle(64);
    C_ASSERT(result == false);

    requestMap->removeRequest(request);

    clientDataManager->deleteClientPID(request->getClientPID());
    clientDataManager->deleteClientTID(request->getClientTID());

    Request::cleanUpRequest(request);
}

// Add a request to the map
// Check if a request with that handle exists
// free the request from the RequestMap
// Verify that no request with that handle exists in the map now.
static void TestRequestDeletion1() {
    int32_t testClientPID = 321;
    int32_t testClientTID = 321;

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource* resource;
    try {
        resource = generateResourceFromMemoryPoolForTesting(1);

    } catch(const std::bad_alloc& e) {
        return;
    }

    ResIterable* resIterable = MPLACED(ResIterable);
    resIterable->mData = resource;

    Request* request;
    try {
        request = new(GetBlock<Request>()) Request();
    } catch(const std::bad_alloc& e) {
        return;
    }

    request->setRequestType(REQ_RESOURCE_TUNING);
    request->setHandle(25);
    request->setDuration(-1);
    request->setPriority(REQ_PRIORITY_HIGH);
    request->addResource(resIterable);
    request->setClientPID(testClientPID);
    request->setClientTID(testClientTID);
    request->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(request->getClientPID(), request->getClientTID())) {
        if(!clientDataManager->createNewClient(request->getClientPID(), request->getClientTID())) {
            return;
        }
    }

    int8_t requestCheck = requestMap->shouldRequestBeAdded(request);
    if(requestCheck) {
        requestMap->addRequest(request);
    }
    C_ASSERT(requestCheck == true);

    C_ASSERT(requestMap->verifyHandle(25) == true);
    requestMap->removeRequest(request);
    C_ASSERT(requestMap->verifyHandle(25) == false);

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);

    Request::cleanUpRequest(request);
}

// Add a request R from client C, verify it's added successfully
// Try adding R again, the operation should fail
// free(Request R from the map
// Now try adding R back to the RequestMap, the operation should succeed.
static void TestRequestDeletion2() {
    int32_t testClientPID = 321;
    int32_t testClientTID = 321;

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource* resource = generateResourceFromMemoryPoolForTesting(1);
    Resource* duplicateResource = generateResourceFromMemoryPoolForTesting(1);

    ResIterable* resIter1 = MPLACED(ResIterable);
    resIter1->mData = resource;

    ResIterable* resIter2 = MPLACED(ResIterable);
    resIter2->mData = duplicateResource;

    Request* request = MPLACED(Request);
    Request* duplicateRequest = MPLACED(Request);

    request->setRequestType(REQ_RESOURCE_TUNING);
    request->setHandle(25);
    request->setDuration(-1);
    request->setPriority(REQ_PRIORITY_HIGH);
    request->setClientPID(testClientPID);
    request->setClientTID(testClientTID);
    request->addResource(resIter1);
    request->setBackgroundProcessing(false);

    duplicateRequest->setRequestType(REQ_RESOURCE_TUNING);
    duplicateRequest->setHandle(25);
    duplicateRequest->setDuration(-1);
    duplicateRequest->setPriority(REQ_PRIORITY_HIGH);
    duplicateRequest->setClientPID(testClientPID);
    duplicateRequest->setClientTID(testClientTID);
    duplicateRequest->addResource(resIter2);
    duplicateRequest->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(request->getClientPID(), request->getClientTID())) {
        if(!clientDataManager->createNewClient(request->getClientPID(), request->getClientTID())) {
            return;
        }
    }

    int8_t requestCheck = requestMap->shouldRequestBeAdded(request);
    if(requestCheck) {
        requestMap->addRequest(request);
    }
    C_ASSERT(requestCheck == true);

    requestCheck = requestMap->shouldRequestBeAdded(duplicateRequest);
    if(requestCheck) {
        requestMap->addRequest(duplicateRequest);
    }
    C_ASSERT(requestCheck == false);

    requestMap->removeRequest(request);

    requestCheck = requestMap->shouldRequestBeAdded(duplicateRequest);
    if(requestCheck) {
        requestMap->addRequest(duplicateRequest);
    }
    C_ASSERT(requestCheck == true);

    requestMap->removeRequest(duplicateRequest);

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);

    Request::cleanUpRequest(request);
    Request::cleanUpRequest(duplicateRequest);
}

// Corner cases
// These tests cover the cases of null requests and requests
// with one or more resources being null.
// For such cases, RequestMap rejects the request,
// No need to futher process such a malformed request.
static void TestNullRequestAddition() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    C_ASSERT(requestMap->shouldRequestBeAdded(nullptr) == false);
}

static void TestRequestWithNullResourcesAddition() {
    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    ResIterable* resIterable = MPLACED(ResIterable);
    resIterable->mData = nullptr;

    Request *request;
    try {
        request = new(GetBlock<Request>()) Request();
    } catch(const std::bad_alloc& e) {
        return;
    }

    request->setRequestType(REQ_RESOURCE_TUNING);
    request->setHandle(25);
    request->setDuration(-1);
    request->addResource(resIterable);
    request->setPriority(REQ_PRIORITY_HIGH);
    request->setClientPID(321);
    request->setClientTID(321);
    request->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(request->getClientPID(), request->getClientTID())) {
        if(!clientDataManager->createNewClient(request->getClientPID(), request->getClientTID())) {
            return;
        }
    }

    int8_t requestCheck = requestMap->shouldRequestBeAdded(request);
    if(requestCheck) {
        requestMap->addRequest(request);
    }
    C_ASSERT(requestCheck == false);

    clientDataManager->deleteClientPID(request->getClientPID());
    clientDataManager->deleteClientTID(request->getClientTID());

    Request::cleanUpRequest(request);
}

static void TestGetRequestFromMap() {
    int32_t testClientPID = 321;
    int32_t testClientTID = 321;

    std::shared_ptr<ClientDataManager> clientDataManager = ClientDataManager::getInstance();
    std::shared_ptr<RequestManager> requestMap = RequestManager::getInstance();

    Resource* resource;

    try {
        resource = generateResourceFromMemoryPoolForTesting(1);
    } catch(const std::bad_alloc& e) {
        return;
    }

    resource->setResCode(15564);
    resource->setOptionalInfo(4445);
    resource->setNumValues(1);
    resource->setValueAt(0, 42);

    ResIterable* resIterable = MPLACED(ResIterable);
    resIterable->mData = resource;

    Request *request;
    try {
        request = new(GetBlock<Request>()) Request();
    } catch(const std::bad_alloc& e) {
        return;
    }

    request->setRequestType(REQ_RESOURCE_TUNING);
    request->setHandle(325);
    request->setDuration(-1);
    request->addResource(resIterable);
    request->setPriority(REQ_PRIORITY_HIGH);
    request->setClientPID(testClientPID);
    request->setClientTID(testClientTID);
    request->setBackgroundProcessing(false);

    if(!clientDataManager->clientExists(request->getClientPID(), request->getClientTID())) {
        if(!clientDataManager->createNewClient(request->getClientPID(), request->getClientTID())) {
            return;
        }
    }

    int8_t result = requestMap->shouldRequestBeAdded(request);
    if(result) {
        requestMap->addRequest(request);
    }

    C_ASSERT((result == true));

    // Retrieve request and check it's integrity
    Request* fetchedRequest = requestMap->getRequestFromMap(325).first;

    C_ASSERT((fetchedRequest != nullptr));
    C_ASSERT((fetchedRequest->getDuration() == -1));
    C_ASSERT((fetchedRequest->getClientPID() == testClientPID));
    C_ASSERT((fetchedRequest->getClientTID() == testClientTID));
    C_ASSERT((fetchedRequest->getResourcesCount() == 1));

    requestMap->removeRequest(request);

    fetchedRequest = requestMap->getRequestFromMap(325).first;
    C_ASSERT((fetchedRequest == nullptr));

    clientDataManager->deleteClientPID(testClientPID);
    clientDataManager->deleteClientTID(testClientTID);

    Request::cleanUpRequest(request);
}
