// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


// ThreadPoolTests_mtest.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <cstdint>
#include <string>
#include <cassert>

#include "TestUtils.h"    // if your tests rely on helpers/macros
#include "ThreadPool.h"   // your thread pool component

#define MTEST_NO_MAIN
#include "../framework/mini.hpp"


// ---- Shared state (same as original) ----
static std::mutex              taskLock;
static std::condition_variable taskCV;

static int32_t     sharedVariable = 0;
static std::string sharedString   = "";
static int8_t      taskCondition  = false;

// ---- Task functions (same semantics as original) ----
static void threadPoolTask(void* arg) {
    assert(arg != nullptr);
    *(int32_t*)arg = 64;
    return;
}

static void threadPoolLongDurationTask(void* arg) {
    std::this_thread::sleep_for(std::chrono::seconds(*(int32_t*)arg));
}

static void helperFunction(void* /*arg*/) {
    for (int32_t i = 0; i < 10000000; i++) { // 1e7
        taskLock.lock();
        sharedVariable++;
        taskLock.unlock();
    }
}

// --- Replacements for the two original lambda tasks (logic unchanged) ---
static void incrementSharedVariableTask(void* /*arg*/) {
    for (int32_t i = 0; i < 10000000; ++i) { // 1e7
        sharedVariable++;
    }
}

static void assignFromArgTask(void* arg) {
    assert(arg != nullptr);                // matches original C_ASSERT inside lambda
    sharedVariable = *(int32_t*)arg;       // same assignment as original
}

// ---- CV coordination tasks (unchanged) ----
static void taskAFunc(void* /*arg*/) {
    std::unique_lock<std::mutex> uniqueLock(taskLock);
    sharedString.push_back('A');
    taskCondition = true;
    taskCV.notify_one();
}

static void taskBFunc(void* /*arg*/) {
    std::unique_lock<std::mutex> uniqueLock(taskLock);
    while (!taskCondition) {
        taskCV.wait(uniqueLock);
    }
    sharedString.push_back('B');
}

// ----------------- Tests (ported to mtest macros) -----------------

static void sleep_seconds(int s) { std::this_thread::sleep_for(std::chrono::seconds(s)); }

// 1) Single worker, single queue: task pickup and value write
MT_TEST(threadpool, TestThreadPoolTaskPickup1, "component-serial") {
    ThreadPool* threadPool = new ThreadPool(1, 1);
    sleep_seconds(1);

    // Preserve original allocation style
    int32_t* ptr = (int32_t*)malloc(sizeof(int32_t));
    *ptr = 49;

    int8_t status = threadPool->enqueueTask(threadPoolTask, (void*)ptr);
    MT_REQUIRE(ctx, status == true);

    sleep_seconds(1);
    MT_REQUIRE_EQ(ctx, *ptr, 64);

    // Preserve original delete style (matches original code semantics)
    delete ptr;
    delete threadPool;
    threadPool = nullptr;
}

// 2) Two workers, two depth: enqueue status and value write
MT_TEST(threadpool, TestThreadPoolEnqueueStatus1, "component-serial") {
    ThreadPool* threadPool = new ThreadPool(2, 2);
    sleep_seconds(1);

    int32_t* ptr = (int32_t*)malloc(sizeof(int32_t));
    *ptr = 14;

    int8_t ret = threadPool->enqueueTask(threadPoolTask, (void*)ptr);
    MT_REQUIRE(ctx, ret == true);

    sleep_seconds(1);
    MT_REQUIRE_EQ(ctx, *ptr, 64);

    delete ptr;
    delete threadPool;
    threadPool = nullptr;
}

// 3) One worker, one depth: enqueue two long tasks
MT_TEST(threadpool, TestThreadPoolEnqueueStatus2_1, "component-serial") {
    ThreadPool* threadPool = new ThreadPool(1, 1);
    sleep_seconds(1);

    int32_t* ptr = (int32_t*)malloc(sizeof(int32_t));
    *ptr = 2;

    int8_t ret1 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);
    int8_t ret2 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);

    MT_REQUIRE(ctx, ret1 == true);
    MT_REQUIRE(ctx, ret2 == true);

    sleep_seconds(8);
    delete ptr;
    delete threadPool;
}

// 4) Two workers, two depth: enqueue three long tasks
MT_TEST(threadpool, TestThreadPoolEnqueueStatus2_2, "component-serial") {
    ThreadPool* threadPool = new ThreadPool(2, 2);
    sleep_seconds(1);

    int32_t* ptr = (int32_t*)malloc(sizeof(int32_t));
    *ptr = 2;

    int8_t ret1 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);
    int8_t ret2 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);
    int8_t ret3 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);

    MT_REQUIRE(ctx, ret1 == true);
    MT_REQUIRE(ctx, ret2 == true);
    MT_REQUIRE(ctx, ret3 == true);

    sleep_seconds(8);
    delete ptr;
    delete threadPool;
}

// 5) Two workers: concurrent processing of two heavy tasks, check sharedVariable
MT_TEST(threadpool, TestThreadPoolTaskProcessing1, "component-serial") {
    sharedVariable = 0;

    ThreadPool* threadPool = new ThreadPool(2, 2);
    sleep_seconds(1);

    int8_t ret1 = threadPool->enqueueTask(helperFunction, nullptr);
    int8_t ret2 = threadPool->enqueueTask(helperFunction, nullptr);

    MT_REQUIRE(ctx, ret1 == true);
    MT_REQUIRE(ctx, ret2 == true);

    // Wait for both tasks to complete
    sleep_seconds(3);

    delete threadPool;

    std::cout << "sharedVariable value = " << sharedVariable << std::endl;
    MT_REQUIRE_EQ(ctx, sharedVariable, 20000000); // 2e7
}

// 6) Single worker: lambda task increments sharedVariable → replaced with named task
MT_TEST(threadpool, TestThreadPoolTaskProcessing2, "component-serial") {
    ThreadPool* threadPool = new ThreadPool(1, 1);
    sleep_seconds(1);
    sharedVariable = 0;

    int8_t ret = threadPool->enqueueTask(incrementSharedVariableTask, nullptr);
    MT_REQUIRE(ctx, ret == true);

    // Wait for task to complete
    sleep_seconds(3);

    MT_REQUIRE_EQ(ctx, sharedVariable, 10000000); // 1e7

    delete threadPool;
}

// 7) Single worker: lambda reading argument → replaced with named task
MT_TEST(threadpool, TestThreadPoolTaskProcessing3, "component-serial") {
    ThreadPool* threadPool = new ThreadPool(1, 1);
    sleep_seconds(1);

    sharedVariable = 0;
    int32_t* callID = (int32_t*)malloc(sizeof(int32_t));
    *callID = 56;

    int8_t ret = threadPool->enqueueTask(assignFromArgTask, (void*)callID);
    MT_REQUIRE(ctx, ret == true);

    // Wait for task to complete
    sleep_seconds(3);

    MT_REQUIRE_EQ(ctx, sharedVariable, *callID);

    delete callID;
    delete threadPool;
}

// 8) Two workers: CV coordination between A and B tasks
MT_TEST(threadpool, TestThreadPoolTaskProcessing4, "component-serial") {
    sharedString.clear();
    taskCondition = false;

    ThreadPool* threadPool = new ThreadPool(2, 2);
    sleep_seconds(1);

    int8_t ret1 = threadPool->enqueueTask(taskAFunc, nullptr);
    int8_t ret2 = threadPool->enqueueTask(taskBFunc, nullptr);

    MT_REQUIRE(ctx, ret1 == true);
    MT_REQUIRE(ctx, ret2 == true);

    // Wait for both tasks to complete
    sleep_seconds(1);

    MT_REQUIRE_EQ(ctx, sharedString, std::string("AB"));

    delete threadPool;
}

// 9) Scaling: expansion 2 -> 3
MT_TEST(threadpool, TestThreadPoolEnqueueStatusWithExpansion1, "component-serial") {
    ThreadPool* threadPool = new ThreadPool(2, 3);
    sleep_seconds(1);

    int32_t* ptr = (int32_t*)malloc(sizeof(int32_t));
    *ptr = 3;

    int8_t ret1 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);
    int8_t ret2 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);
    int8_t ret3 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);

    MT_REQUIRE(ctx, ret1 == true);
    MT_REQUIRE(ctx, ret2 == true);
    MT_REQUIRE(ctx, ret3 == true);

    sleep_seconds(8);
    delete ptr;
    delete threadPool;
}

// 10) Scaling: expansion 2 -> 4
MT_TEST(threadpool, TestThreadPoolEnqueueStatusWithExpansion2, "component-serial") {
    ThreadPool* threadPool = new ThreadPool(2, 4);
    sleep_seconds(1);

    int32_t* ptr = (int32_t*)malloc(sizeof(int32_t));
    *ptr = 3;

    int8_t ret1 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);
    int8_t ret2 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);
    int8_t ret3 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);
    int8_t ret4 = threadPool->enqueueTask(threadPoolLongDurationTask, (void*)ptr);

    MT_REQUIRE(ctx, ret1 == true);
    MT_REQUIRE(ctx, ret2 == true);
    MT_REQUIRE(ctx, ret3 == true);
    MT_REQUIRE(ctx, ret4 == true);

    sleep_seconds(8);
    delete ptr;
    delete threadPool;
}

