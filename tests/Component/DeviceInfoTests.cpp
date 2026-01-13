// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


// DeviceInfoTests.cpp
#include "ErrCodes.h"
#include "TestUtils.h"
#include "TestBaseline.h"
#include "TargetRegistry.h"
#include "Extensions.h"
#include "Utils.h"
#include "TestAggregator.h"

#include <cstdint>
#include <iostream>

#define MTEST_NO_MAIN
#include "../framework/mini.hpp"


using namespace mtest;

// Keep a single baseline instance as in original
static TestBaseline baseline;

// ---- One-time initialization via fixture ----
static void InitOnce() {
    // Fetch expected baseline and read target info from the registry
    baseline.fetchBaseline();

    // If getInstance returns shared_ptr<TargetRegistry>, use 'auto' and call via ->
    auto tr = TargetRegistry::getInstance();
    tr->readTargetInfo();
}

struct DeviceInfoFixture : mtest::Fixture {
    static bool initialized;
    void setup(mtest::TestContext&) override {
        if (!initialized) {
            InitOnce();
            initialized = true;
        }
    }
    void teardown(mtest::TestContext&) override {}
};
bool DeviceInfoFixture::initialized = false;

// ---------------------------
// Suite: DeviceInfoTests
// Tag:  component-serial
// ---------------------------

MT_TEST_F(DeviceInfoTests, ClusterCountMatchesBaseline, "component-serial", DeviceInfoFixture) {
    int32_t clusterCount         = UrmSettings::targetConfigs.mTotalClusterCount;
    int32_t expectedClusterCount = baseline.getExpectedClusterCount();

    std::cout << "Determined Cluster Count: " << clusterCount << std::endl;
    std::cout << "Expected Cluster Count: "  << expectedClusterCount << std::endl;

    // Baseline unavailable => skip silently (early return)
    if (expectedClusterCount == -1) {
        // If you prefer an explicit message, use:
        // MT_FAIL(ctx, "Baseline could not be fetched for the Target, skipping");
        return;
    }

    MT_REQUIRE_EQ(ctx, clusterCount, expectedClusterCount);
}
MT_TEST_F_END

MT_TEST_F(DeviceInfoTests, CoreCountMatchesBaseline, "component-serial", DeviceInfoFixture) {
    int32_t coreCount         = UrmSettings::targetConfigs.mTotalCoreCount;
    int32_t expectedCoreCount = baseline.getExpectedCoreCount();

    std::cout << "Determined Core Count: " << coreCount << std::endl;
    std::cout << "Expected Core Count: "  << expectedCoreCount << std::endl;

    if (expectedCoreCount == -1) {
        // MT_FAIL(ctx, "Baseline could not be fetched for the Target, skipping");
        return;
    }

    MT_REQUIRE_EQ(ctx, coreCount, expectedCoreCount);
}
MT_TEST_F_END

MT_TEST_F(DeviceInfoTests, ClusterLogicalToPhysicalMapping, "component-serial", DeviceInfoFixture) {
    auto tr = TargetRegistry::getInstance();

    // Check Lgc 0..3 if baseline provides them (!= -1)
    for (int lgc = 0; lgc <= 3; ++lgc) {
        int32_t expectedPhy = baseline.getExpectedPhysicalCluster(lgc);
        if (expectedPhy != -1) {
            int32_t phyID = tr->getPhysicalClusterId(lgc);
            std::cout << "Lgc " << lgc << " maps to physical ID: "
                      << phyID << ", Expected: [" << expectedPhy << "]" << std::endl;

            MT_REQUIRE_EQ(ctx, expectedPhy, phyID);
        }
    }
}
MT_TEST_F_END

// No RunTests()/REGISTER_TEST() needed — mini.hpp auto-registers tests and provides main()
// Build and run serially:
//   g++ -std=gnu++17 -O2 -pthread DeviceInfoTests.cpp -o device_info_tests
//   ./device_info_tests --tag=component-serial --threads=1

