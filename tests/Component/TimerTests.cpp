// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <cmath>

#include "TestUtils.h"
#include "Timer.h"
#include "URMTests.h"

#define TEST_CLASS "COMPONENT"

static std::shared_ptr<ThreadPool> tpoolInstance = std::shared_ptr<ThreadPool> (new ThreadPool(4, 5));
static std::atomic<int8_t> isFinished;

static void afterTimer(void*) {
    isFinished.store(true);
}

static void Init() {
    static int8_t initDone = false;
    if(!initDone) {
        initDone = true;
        Timer::mTimerThreadPool = tpoolInstance.get();
        MakeAlloc<Timer>(10);
    }
}

static void simulateWork() {
    while(!isFinished.load()){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

URM_TEST(BaseCase, {
    Init();
    Timer* timer = new Timer(afterTimer);
    isFinished.store(false);

    E_ASSERT((timer != nullptr));
    auto start = std::chrono::high_resolution_clock::now();
    timer->startTimer(200);
    simulateWork();
    auto finish = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    E_ASSERT_NEAR(dur, 200, 25); //some tolerance
    delete timer;
})

URM_TEST(KillBeforeCompletion, {
    Init();
    Timer* timer = new Timer(afterTimer);
    isFinished.store(false);

    E_ASSERT((timer != nullptr));
    auto start = std::chrono::high_resolution_clock::now();
    timer->startTimer(200);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    timer->killTimer();
    auto finish = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    E_ASSERT_NEAR(dur, 100, 25); //some tolerance
    delete timer;
})

URM_TEST(KillAfterCompletion, {
    Init();
    Timer* timer = new Timer(afterTimer);
    isFinished.store(false);

    E_ASSERT((timer != nullptr));
    auto start = std::chrono::high_resolution_clock::now();
    timer->startTimer(200);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    timer->killTimer();
    auto finish = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    E_ASSERT_NEAR(dur, 300, 25); //some tolerance
    delete timer;
})

URM_TEST(RecurringTimer, {
    Init();
    Timer* recurringTimer = new Timer(afterTimer, true);
    isFinished.store(false);

    E_ASSERT((recurringTimer != nullptr));

    auto start = std::chrono::high_resolution_clock::now();
    recurringTimer->startTimer(200);
    simulateWork();
    isFinished.store(false);
    simulateWork();
    isFinished.store(false);
    simulateWork();
    recurringTimer->killTimer();
    auto finish = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    E_ASSERT_NEAR(dur, 600, 25); //some tolerance
    delete recurringTimer;
})

URM_TEST(RecurringTimerPreMatureKill, {
    Init();
    Timer* recurringTimer = new Timer(afterTimer, true);
    isFinished.store(false);

    E_ASSERT((recurringTimer != nullptr));

    auto start = std::chrono::high_resolution_clock::now();
    recurringTimer->startTimer(200);
    simulateWork();
    isFinished.store(false);
    simulateWork();
    isFinished.store(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    recurringTimer->killTimer();
    auto finish = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    E_ASSERT_NEAR(dur, 500, 25); //some tolerance
})
