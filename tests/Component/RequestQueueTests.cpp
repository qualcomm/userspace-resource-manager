// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// RequestQueueTests_mtest.cpp
#include <atomic>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <memory>

#define MTEST_NO_MAIN
#include "../framework/mini.hpp"

#include "RequestQueue.h"
#include "TestUtils.h"
#include "TestAggregator.h"

// ---------- Init (unchanged) ----------
static void Init() {
    MakeAlloc<Message>(30);
}

// ---------- helper sleep (same semantics as original sleep(1)) ----------
static void sleep_s(int sec) {
    std::this_thread::sleep_for(std::chrono::seconds(sec));
}

// ---------- Thread functors / functions (replacing lambdas) ----------

// TestRequestQueueSingleTaskPickup1: consumer routine
struct ConsumerPickup1 {
    std::shared_ptr<RequestQueue>* rq;

    explicit ConsumerPickup1(std::shared_ptr<RequestQueue>* q) : rq(q) {}

    void operator()() {
        (*rq)->wait();

        while ((*rq)->hasPendingTasks()) {
            Request* req = (Request*)(*rq)->pop();

            // original assertions preserved
            C_ASSERT(req->getRequestType() == REQ_RESOURCE_TUNING);
            C_ASSERT(req->getHandle()      == 200);
            C_ASSERT(req->getClientPID()   == 321);
            C_ASSERT(req->getClientTID()   == 2445);
            C_ASSERT(req->getDuration()    == -1);

            delete req;
        }
    }
};

// TestRequestQueueSingleTaskPickup2: consumer routine with CV
struct ConsumerPickup2Ctx {
    std::shared_ptr<RequestQueue>* rq;
    std::atomic<int32_t>* requestsProcessed;
    int8_t* taskCondition;
    std::mutex* taskLock;
    std::condition_variable* taskCV;
};

static void ConsumerPickup2(ConsumerPickup2Ctx ctx) {
    std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);
    int8_t terminateProducer = false;

    while (true) {
        if (terminateProducer) return;

        while (!*ctx.taskCondition) {
            ctx.taskCV->wait(uniqueLock);
        }

        while ((*ctx.rq)->hasPendingTasks()) {
            Request* req = (Request*)(*ctx.rq)->pop();
            ctx.requestsProcessed->fetch_add(1);

            if (req->getHandle() == 0) {
                delete req;
                terminateProducer = true;
                break;
            }

            delete req;
        }
    }
}

// TestRequestQueueSingleTaskPickup2: producer routine
static void ProducerPickup2(ConsumerPickup2Ctx ctx) {
    const std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);

    Request* req = new Request;
    req->setRequestType(REQ_RESOURCE_TUNING);
    req->setDuration(-1);
    req->setHandle(0);
    req->setProperties(0);
    req->setClientPID(321);
    req->setClientTID(2445);
    (*ctx.rq)->addAndWakeup(req);

    *ctx.taskCondition = true;
    ctx.taskCV->notify_one();
}

// TestRequestQueueMultipleTaskPickup: consumer with sentinel (-1)
struct ConsumerMultiPickupCtx {
    std::shared_ptr<RequestQueue>* rq;
    std::atomic<int32_t>* requestsProcessed;
    int8_t* taskCondition;
    std::mutex* taskLock;
    std::condition_variable* taskCV;
};

static void ConsumerMultiPickup(ConsumerMultiPickupCtx ctx) {
    std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);
    int8_t terminateProducer = false;

    while (true) {
        if (terminateProducer) return;

        while (!*ctx.taskCondition) {
            ctx.taskCV->wait(uniqueLock);
        }

        while ((*ctx.rq)->hasPendingTasks()) {
            Request* req = (Request*)(*ctx.rq)->pop();
            ctx.requestsProcessed->fetch_add(1);

            if (req->getHandle() == -1) {
                delete req;
                terminateProducer = true;
                break;
            }

            delete req;
        }
    }
}

// TestRequestQueueMultipleTaskPickup: producer pushes 3 + 1 exit
static void ProducerMultiPickup(ConsumerMultiPickupCtx ctx) {
    std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);

    Request* req1 = new Request();
    req1->setRequestType(REQ_RESOURCE_TUNING);
    req1->setHandle(200);
    req1->setDuration(-1);
    req1->setProperties(0);
    req1->setClientPID(321);
    req1->setClientTID(2445);

    Request* req2 = new Request();
    req2->setRequestType(REQ_RESOURCE_TUNING);
    req2->setHandle(112);
    req2->setDuration(-1);
    req2->setProperties(0);
    req2->setClientPID(344);
    req2->setClientTID(2378);

    Request* req3 = new Request();
    req3->setRequestType(REQ_RESOURCE_TUNING);
    req3->setHandle(44);
    req3->setDuration(6500);
    req3->setProperties(0);
    req3->setClientPID(32180);
    req3->setClientTID(67770);

    (*ctx.rq)->addAndWakeup(req1);
    (*ctx.rq)->addAndWakeup(req2);
    (*ctx.rq)->addAndWakeup(req3);

    // exit/sentinel
    Request* exitReq = new Request();
    exitReq->setRequestType(REQ_RESOURCE_TUNING);
    exitReq->setHandle(-1);
    exitReq->setDuration(-1);
    exitReq->setProperties(1);
    exitReq->setClientPID(554);
    exitReq->setClientTID(3368);
    (*ctx.rq)->addAndWakeup(exitReq);

    *ctx.taskCondition = true;
    ctx.taskCV->notify_one();
}

// TestRequestQueueMultipleTaskAndProducersPickup: producer routine with count
struct ProducerManyCtx {
    std::shared_ptr<RequestQueue>* rq;
    int32_t count;
};

static void ProducerMany(ProducerManyCtx ctx) {
    Request* req = new Request();
    req->setRequestType(REQ_RESOURCE_TUNING);
    req->setHandle(ctx.count);
    req->setDuration(-1);
    req->setProperties(0);
    req->setClientPID(321 + ctx.count);
    req->setClientTID(2445 + ctx.count);
    (*ctx.rq)->addAndWakeup(req);
}

// terminate thread routine (adds sentinel and notifies)
struct TerminatorCtx {
    std::shared_ptr<RequestQueue>* rq;
    int8_t* taskCondition;
    std::mutex* taskLock;
    std::condition_variable* taskCV;
};

static void Terminator(TerminatorCtx ctx) {
    std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);

    Request* exitReq = new Request();
    exitReq->setRequestType(REQ_RESOURCE_TUNING);
    exitReq->setHandle(-1);
    exitReq->setDuration(-1);
    exitReq->setProperties(1);
    exitReq->setClientPID(100);
    exitReq->setClientTID(1155);
    (*ctx.rq)->addAndWakeup(exitReq);

    *ctx.taskCondition = true;
    ctx.taskCV->notify_one();
}

// Consumer for multiple-producers variant
struct ConsumerManyCtx {
    std::shared_ptr<RequestQueue>* rq;
    std::atomic<int32_t>* requestsProcessed;
    int8_t* taskCondition;
    std::mutex* taskLock;
    std::condition_variable* taskCV;
};

static void ConsumerMany(ConsumerManyCtx ctx) {
    std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);
    int8_t terminateProducer = false;

    while (true) {
        if (terminateProducer) return;

        while (!*ctx.taskCondition) {
            ctx.taskCV->wait(uniqueLock);
        }

        while ((*ctx.rq)->hasPendingTasks()) {
            Request* req = (Request*)(*ctx.rq)->pop();
            ctx.requestsProcessed->fetch_add(1);

            if (req->getHandle() == -1) {
                delete req;
                terminateProducer = true;
                break;
            }

            delete req;
        }
    }
}

// Comparator functor for priority sorting (replaces lambda)
struct ReqPriorityLess {
    bool operator()(Request* first, Request* second) const {
        return first->getPriority() < second->getPriority();
    }
};

// Consumer for priority test 1
struct ConsumerPriority1Ctx {
    std::shared_ptr<RequestQueue>* rq;
    std::vector<Request*>* requests; // sorted vector for verifying order
    int32_t* requestsIndex;
    int8_t* taskCondition;
    std::mutex* taskLock;
    std::condition_variable* taskCV;
};

static void ConsumerPriority1(ConsumerPriority1Ctx ctx) {
    std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);
    int8_t terminateProducer = false;

    while (true) {
        if (terminateProducer) return;

        while (!*ctx.taskCondition) {
            ctx.taskCV->wait(uniqueLock);
        }

        while ((*ctx.rq)->hasPendingTasks()) {
            Request* req = (Request*)(*ctx.rq)->pop();

            if (req->getHandle() == -1) {
                delete req;
                terminateProducer = true;
                break;
            }

            // Verify the order matches the sorted 'requests' vector
            C_ASSERT(req->getClientPID() == (*ctx.requests)[*ctx.requestsIndex]->getClientPID());
            C_ASSERT(req->getClientTID() == (*ctx.requests)[*ctx.requestsIndex]->getClientTID());
            C_ASSERT(req->getPriority()  == (*ctx.requests)[*ctx.requestsIndex]->getPriority());
            (*ctx.requestsIndex)++;

            delete req;
        }
    }
}

// Producer for priority test 1
struct ProducerPriority1Ctx {
    std::shared_ptr<RequestQueue>* rq;
    std::vector<Request*>* requests; // already sorted
    int8_t* taskCondition;
    std::mutex* taskLock;
    std::condition_variable* taskCV;
};

static void ProducerPriority1(ProducerPriority1Ctx ctx) {
    std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);

    for (Request* request : *ctx.requests) {
        (*ctx.rq)->addAndWakeup(request);
    }

    *ctx.taskCondition = true;
    ctx.taskCV->notify_one();
}

// Consumer for priority test 2 (fixed expectations)
struct ConsumerPriority2Ctx {
    std::shared_ptr<RequestQueue>* rq;
    int32_t* requestsIndex;
    int8_t* taskCondition;
    std::mutex* taskLock;
    std::condition_variable* taskCV;
};

static void ConsumerPriority2(ConsumerPriority2Ctx ctx) {
    std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);
    int8_t terminateProducer = false;

    while (true) {
        if (terminateProducer) return;

        while (!*ctx.taskCondition) {
            ctx.taskCV->wait(uniqueLock);
        }

        while ((*ctx.rq)->hasPendingTasks()) {
            Request* req = (Request*)(*ctx.rq)->pop();

            if (req->getHandle() == -1) {
                delete req;
                terminateProducer = true;
                break;
            }

            if (*ctx.requestsIndex == 0) {
                C_ASSERT(req->getClientPID() == 321);
                C_ASSERT(req->getClientTID() == 2445);
                C_ASSERT(req->getHandle()    == 11);
            } else if (*ctx.requestsIndex == 1) {
                C_ASSERT(req->getClientPID() == 234);
                C_ASSERT(req->getClientTID() == 5566);
                C_ASSERT(req->getHandle()    == 23);
            }
            (*ctx.requestsIndex)++;

            delete req;
        }
    }
}

// Producer for priority test 2
struct ProducerPriority2Ctx {
    std::shared_ptr<RequestQueue>* rq;
    Request* systemPermissionRequest;
    Request* thirdPartyPermissionRequest;
    Request* exitReq;
    int8_t* taskCondition;
    std::mutex* taskLock;
    std::condition_variable* taskCV;
};

static void ProducerPriority2(ProducerPriority2Ctx ctx) {
    std::unique_lock<std::mutex> uniqueLock(*ctx.taskLock);

    (*ctx.rq)->addAndWakeup(ctx.systemPermissionRequest);
    (*ctx.rq)->addAndWakeup(ctx.thirdPartyPermissionRequest);
    (*ctx.rq)->addAndWakeup(ctx.exitReq);

    *ctx.taskCondition = true;
    ctx.taskCV->notify_one();
}

// ---------- Tests (ported one-to-one) ----------

MT_TEST(RequestQueue, TestRequestQueueTaskEnqueue, "component-serial") {
    Init();
    std::shared_ptr<RequestQueue> queue = RequestQueue::getInstance();
    int32_t requestCount = 8;
    int32_t requestsProcessed = 0;

    for (int32_t count = 0; count < requestCount; count++) {
        Message* message = new (GetBlock<Message>()) Message;
        message->setDuration(9000);
        queue->addAndWakeup(message);
    }

    while (queue->hasPendingTasks()) {
        requestsProcessed++;
        Message* message = (Message*)queue->pop();
        FreeBlock<Message>(static_cast<void*>(message));
    }

    MT_REQUIRE_EQ(ctx, requestsProcessed, requestCount);
}

MT_TEST(RequestQueue, TestRequestQueueSingleTaskPickup1, "component-serial") {
    Init();
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();

    ConsumerPickup1 consumer(&requestQueue);
    std::thread consumerThread(consumer);

    // Producer (unchanged logic)
    Request* req = new Request();
    req->setRequestType(REQ_RESOURCE_TUNING);
    req->setHandle(200);
    req->setDuration(-1);
    req->setClientPID(321);
    req->setClientTID(2445);
    req->setProperties(0);
    requestQueue->addAndWakeup(req);

    consumerThread.join();
}

MT_TEST(RequestQueue, TestRequestQueueSingleTaskPickup2, "component-serial") {
    Init();
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();
    std::atomic<int32_t> requestsProcessed(0);
    int8_t taskCondition = false;
    std::mutex taskLock;
    std::condition_variable taskCV;

    ConsumerPickup2Ctx cctx{ &requestQueue, &requestsProcessed, &taskCondition, &taskLock, &taskCV };
    std::thread consumerThread(ConsumerPickup2, cctx);
    std::thread producerThread(ProducerPickup2, cctx);

    producerThread.join();
    consumerThread.join();

    MT_REQUIRE_EQ(ctx, requestsProcessed.load(), 1);
}

MT_TEST(RequestQueue, TestRequestQueueMultipleTaskPickup, "component-serial") {
    Init();
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();
    std::atomic<int32_t> requestsProcessed(0);
    int8_t taskCondition = false;
    std::mutex taskLock;
    std::condition_variable taskCV;

    ConsumerMultiPickupCtx mctx{ &requestQueue, &requestsProcessed, &taskCondition, &taskLock, &taskCV };
    std::thread consumerThread(ConsumerMultiPickup, mctx);
    std::thread producerThread(ProducerMultiPickup, mctx);

    consumerThread.join();
    producerThread.join();

    MT_REQUIRE_EQ(ctx, requestsProcessed.load(), 4);
}

MT_TEST(RequestQueue, TestRequestQueueMultipleTaskAndProducersPickup, "component-serial") {
    Init();
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();
    std::atomic<int32_t> requestsProcessed(0);
    int32_t totalNumberOfThreads = 10;
    int8_t taskCondition = false;
    std::mutex taskLock;
    std::condition_variable taskCV;

    ConsumerManyCtx cc{ &requestQueue, &requestsProcessed, &taskCondition, &taskLock, &taskCV };
    std::thread consumerThread(ConsumerMany, cc);

    std::vector<std::thread> producerThreads;
    producerThreads.reserve(totalNumberOfThreads);

    // Create multiple producer threads (lambda replaced with functor-per-count)
    for (int32_t count = 0; count < totalNumberOfThreads; count++) {
        ProducerManyCtx pc{ &requestQueue, count };
        producerThreads.emplace_back(ProducerMany, pc);
    }
    for (std::size_t i = 0; i < producerThreads.size(); i++) {
        producerThreads[i].join();
    }

    sleep_s(1); // original sleep(1) logic

    TerminatorCtx tc{ &requestQueue, &taskCondition, &taskLock, &taskCV };
    std::thread terminateThread(Terminator, tc);

    consumerThread.join();
    terminateThread.join();

    MT_REQUIRE_EQ(ctx, requestsProcessed.load(), totalNumberOfThreads + 1);
}

MT_TEST(RequestQueue, TestRequestQueueEmptyPoll, "component-serial") {
    Init();
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();
    MT_REQUIRE(ctx, requestQueue->pop() == nullptr);
}

MT_TEST(RequestQueue, TestRequestQueuePollingPriority1, "component-serial") {
    Init();
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();
    int8_t taskCondition = false;
    std::mutex taskLock;
    std::condition_variable taskCV;

    std::vector<Request*> requests = {
        new Request(), new Request(), new Request(), new Request(), new Request(), new Request()
    };

    // Initialize as original
    requests[0]->setRequestType(REQ_RESOURCE_TUNING);
    requests[0]->setHandle(11);
    requests[0]->setDuration(-1);
    requests[0]->setProperties(SYSTEM_HIGH);
    requests[0]->setClientPID(321);
    requests[0]->setClientTID(2445);

    requests[1]->setRequestType(REQ_RESOURCE_TUNING);
    requests[1]->setHandle(23);
    requests[1]->setDuration(-1);
    requests[1]->setProperties(SYSTEM_LOW);
    requests[1]->setClientPID(234);
    requests[1]->setClientTID(5566);

    requests[2]->setRequestType(REQ_RESOURCE_TUNING);
    requests[2]->setHandle(38);
    requests[2]->setDuration(-1);
    requests[2]->setProperties(THIRD_PARTY_HIGH);
    requests[2]->setClientPID(522);
    requests[2]->setClientTID(8889);

    requests[3]->setRequestType(REQ_RESOURCE_TUNING);
    requests[3]->setHandle(55);
    requests[3]->setDuration(-1);
    requests[3]->setProperties(THIRD_PARTY_LOW);
    requests[3]->setClientPID(455);
    requests[3]->setClientTID(2547);

    requests[4]->setRequestType(REQ_RESOURCE_TUNING);
    requests[4]->setHandle(87);
    requests[4]->setDuration(-1);
    requests[4]->setProperties(10);
    requests[4]->setClientPID(770);
    requests[4]->setClientTID(7774);

    requests[5]->setRequestType(REQ_RESOURCE_TUNING);
    requests[5]->setHandle(-1);
    requests[5]->setDuration(-1);
    requests[5]->setProperties(15);
    requests[5]->setClientPID(115);
    requests[5]->setClientTID(1211);

    // Sort by priority (functor replaces lambda)
    std::sort(requests.begin(), requests.end(), ReqPriorityLess());

    // Producer (lambda replaced)
    ProducerPriority1Ctx pc{ &requestQueue, &requests, &taskCondition, &taskLock, &taskCV };
    std::thread producerThread(ProducerPriority1, pc);

    sleep_s(1);
    int32_t requestsIndex = 0;

    // Consumer (lambda replaced)
    ConsumerPriority1Ctx cc{ &requestQueue, &requests, &requestsIndex, &taskCondition, &taskLock, &taskCV };
    std::thread consumerThread(ConsumerPriority1, cc);

    producerThread.join();
    consumerThread.join();
}

MT_TEST(RequestQueue, TestRequestQueuePollingPriority2, "component-serial") {
    Init();
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();
    int8_t taskCondition = false;
    std::mutex taskLock;
    std::condition_variable taskCV;

    Request* systemPermissionRequest = new Request();
    systemPermissionRequest->setRequestType(REQ_RESOURCE_TUNING);
    systemPermissionRequest->setHandle(11);
    systemPermissionRequest->setDuration(-1);
    systemPermissionRequest->setProperties(SYSTEM_HIGH);
    systemPermissionRequest->setClientPID(321);
    systemPermissionRequest->setClientTID(2445);

    Request* thirdPartyPermissionRequest = new Request();
    thirdPartyPermissionRequest->setRequestType(REQ_RESOURCE_TUNING);
    thirdPartyPermissionRequest->setHandle(23);
    thirdPartyPermissionRequest->setDuration(-1);
    thirdPartyPermissionRequest->setProperties(THIRD_PARTY_HIGH);
    thirdPartyPermissionRequest->setClientPID(234);
    thirdPartyPermissionRequest->setClientTID(5566);

    Request* exitReq = new Request();
    exitReq->setRequestType(REQ_RESOURCE_TUNING);
    exitReq->setHandle(-1);
    exitReq->setDuration(-1);
    exitReq->setProperties(THIRD_PARTY_LOW);
    exitReq->setClientPID(102);
    exitReq->setClientTID(1220);

    ProducerPriority2Ctx pc{ &requestQueue, systemPermissionRequest, thirdPartyPermissionRequest,
                             exitReq, &taskCondition, &taskLock, &taskCV };
    std::thread producerThread(ProducerPriority2, pc);

    sleep_s(1);
    int32_t requestsIndex = 0;

    ConsumerPriority2Ctx cc{ &requestQueue, &requestsIndex, &taskCondition, &taskLock, &taskCV };
    std::thread consumerThread(ConsumerPriority2, cc);

    consumerThread.join();
    producerThread.join();
}

MT_TEST(RequestQueue, TestRequestQueueInvalidPriority, "component-serial") {
    Init();
    std::shared_ptr<RequestQueue> requestQueue = RequestQueue::getInstance();

    Request* invalidRequest = new Request();
    invalidRequest->setRequestType(REQ_RESOURCE_TUNING);
    invalidRequest->setHandle(11);
    invalidRequest->setDuration(-1);
    invalidRequest->setProperties(-15);
    invalidRequest->setClientPID(321);
    invalidRequest->setClientTID(2445);

    MT_REQUIRE(ctx, requestQueue->addAndWakeup(invalidRequest) == false);
}

