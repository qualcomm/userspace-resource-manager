// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


// TimerTests_mtest.cpp
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include "Timer.h"
#include "ThreadPool.h"
#include "TestUtils.h"


#define MTEST_NO_MAIN
#include "../framework/mini.h"


// ---- Shared elements from your original suite ----
// (We keep the same pool size and allocation count.)
static std::shared_ptr<ThreadPool> tpoolInstance{ new ThreadPool(4, 5) };
static std::atomic<int8_t> isFinished;

// Callback signature matches your Timer: void(*)(void*)
static void afterTimer(void*) {
    isFinished.store(true);
}

// Equivalent to your Init()
static void Init() {
    Timer::mTimerThreadPool = tpoolInstance.get();
    MakeAlloc<Timer>(10);
}

// Busy-wait with short sleeps until callback sets isFinished
static void simulateWork() {
    while (!isFinished.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Small helper for "near" checks with readable failure messages.
static void REQUIRE_NEAR(mtest::TestContext& ctx,
                         long long got, long long expected, long long tol,
                         const char* what) {
    long long diff = std::llabs(got - expected);
    if (diff > tol) {
        std::ostringstream oss;
        oss << what << " near check failed: got=" << got
            << " expected=" << expected << " tol=" << tol
            << " (|diff|=" << diff << ")";
        MT_FAIL(ctx, oss.str().c_str());
    }
}

// ----------------- Tests -----------------

// BaseCase: one-shot 200ms
MT_TEST(Component, BaseCase, "timer") {
    Init();
    Timer* timer = new Timer(afterTimer);
    isFinished.store(false);

    MT_REQUIRE(ctx, timer != nullptr);

    auto t0 = std::chrono::high_resolution_clock::now();
    timer->startTimer(200);
    simulateWork();
    auto t1 = std::chrono::high_resolution_clock::now();

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    REQUIRE_NEAR(ctx, dur, 200, 25, "BaseCase");   // ±25ms tolerance
    delete timer;
}

// Kill before completion at ~100ms
MT_TEST(Component, killBeforeCompletion, "timer") {
    Init();
    Timer* timer = new Timer(afterTimer);
    isFinished.store(false);

    MT_REQUIRE(ctx, timer != nullptr);

    auto t0 = std::chrono::high_resolution_clock::now();
    timer->startTimer(200);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    timer->killTimer();
    auto t1 = std::chrono::high_resolution_clock::now();

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    REQUIRE_NEAR(ctx, dur, 100, 25, "killBeforeCompletion");
    delete timer;
}

// Kill after completion (~300ms)
MT_TEST(Component, killAfterCompletion, "timer") {
    Init();
    Timer* timer = new Timer(afterTimer);
    isFinished.store(false);

    MT_REQUIRE(ctx, timer != nullptr);

    auto t0 = std::chrono::high_resolution_clock::now();
    timer->startTimer(200);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    timer->killTimer();
    auto t1 = std::chrono::high_resolution_clock::now();

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    REQUIRE_NEAR(ctx, dur, 300, 25, "killAfterCompletion");
    delete timer;
}

// Recurring timer: expect ~600ms for 3 cycles
MT_TEST(Component, RecurringTimer, "timer") {
    Init();
    Timer* recurringTimer = new Timer(afterTimer, true);
    isFinished.store(false);

    MT_REQUIRE(ctx, recurringTimer != nullptr);

    auto t0 = std::chrono::high_resolution_clock::now();
    recurringTimer->startTimer(200);

    simulateWork();      // 1st
    isFinished.store(false);
    simulateWork();      // 2nd
    isFinished.store(false);
    simulateWork();      // 3rd

    recurringTimer->killTimer();
    auto t1 = std::chrono::high_resolution_clock::now();

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    REQUIRE_NEAR(ctx, dur, 600, 25, "RecurringTimer");
    delete recurringTimer;
}

// Recurring + premature kill: ~500ms (2 cycles + ~100ms)
MT_TEST(Component, RecurringTimerPreMatureKill, "timer") {
    Init();
    Timer* recurringTimer = new Timer(afterTimer, true);
    isFinished.store(false);

    MT_REQUIRE(ctx, recurringTimer != nullptr);

    auto t0 = std::chrono::high_resolution_clock::now();
    recurringTimer->startTimer(200);

    simulateWork();      // 1st
    isFinished.store(false);
    simulateWork();      // 2nd
    isFinished.store(false);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    recurringTimer->killTimer();

    auto t1 = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    REQUIRE_NEAR(ctx, dur, 500, 25, "RecurringTimerPreMatureKill");
    delete recurringTimer;
}

// -------- Parameterized variant for BaseCase (try multiple targets) --------

MT_TEST_P(Component, BaseCaseParam, "timer", int,
          (std::initializer_list<int>{100, 200, 300})) { 

    Init();
    Timer* timer = new Timer(afterTimer);
    isFinished.store(false);

    MT_REQUIRE(ctx, timer != nullptr);

    auto t0 = std::chrono::high_resolution_clock::now();
    timer->startTimer(param);     // param is the target duration
    simulateWork();
    auto t1 = std::chrono::high_resolution_clock::now();

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    REQUIRE_NEAR(ctx, dur, param, 25, "BaseCaseParam");
    delete timer;
}

