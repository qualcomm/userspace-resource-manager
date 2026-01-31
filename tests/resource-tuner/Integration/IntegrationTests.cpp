// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#include <thread> 
#include "ErrCodes.h"
#include "UrmPlatformAL.h"
#include "Utils.h"
#include "TestUtils.h"
#include "TestBaseline.h"
#include "UrmAPIs.h"
//#define MTEST_NO_MAIN
#include "../framework/mini.h"

static TestBaseline baseline;                // local instance for this TU
// Fetch baseline once for all tests (your original code did this in main)

static std::once_flag __baseline_once;
static void __ensure_baseline() {
    std::call_once(__baseline_once, []{
        baseline.fetchBaseline();
    });
}


// --------------- Two tests wrapped with mini.hpp ------------

// Wrap as a mini.hpp test: Handle generation should return positive handles and continue across calls.
MT_TEST(Integration, TestHandleGeneration, "handlegeneration") {
    LOG_START
    // If this test depends on baseline, uncomment the next line:
    // __ensure_baseline();

    // 1st tune
    SysResource resource{};           // value-initialize instead of memset
    resource.mResCode = 0x80ff0002;
    resource.mNumValues = 1;
    resource.mResValue.value = 554;

    int64_t handle = tuneResources(2000, 0, 1, &resource);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE(ctx, handle > 0);      // fatal if fails (per-test), runner continues to next case

    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 2nd tune
    resource = SysResource{};         // re-init cleanly
    resource.mResCode = 0x80ff0002;
    resource.mNumValues = 1;
    resource.mResValue.value = 667;

    handle = tuneResources(2000, 0, 1, &resource);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE(ctx, handle > 0);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 3rd tune
    resource = SysResource{};
    resource.mResCode = 0x800f0002;
    resource.mNumValues = 1;
    resource.mResValue.value = 701;

    handle = tuneResources(2000, 0, 1, &resource);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE(ctx, handle > 0);

    LOG_END
}

// Wrap as a mini.hpp test: invalid request verification (duration=0 should fail).
MT_TEST(Integration, TestNullOrInvalidRequestVerification1, "requestverification") {
    LOG_START
    // __ensure_baseline(); // uncomment if required

    int64_t handle = tuneResources(0, RequestPriority::REQ_PRIORITY_HIGH, 0, nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    // Expect the client-side pre-check to fail and return RC_REQ_SUBMISSION_FAILURE
    MT_REQUIRE_EQ(ctx, handle, RC_REQ_SUBMISSION_FAILURE);

    LOG_END
}


MT_TEST(Integration, TestNullOrInvalidRequestVerification2, "requestverification") {
    LOG_START
    // duration = -1 is allowed; but num resources = 0 and list = nullptr ⇒ client-side rejection
    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 0, nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE_EQ(ctx, handle, RC_REQ_SUBMISSION_FAILURE);
    LOG_END
}


MT_TEST(Integration, TestNullOrInvalidRequestVerification3, "requestverification") {
    LOG_START
    SysResource r{};           // invalid basic params
    r.mResCode = -1;
    r.mResInfo = -1;
    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE_EQ(ctx, handle, RC_REQ_SUBMISSION_FAILURE);
    LOG_END
}


MT_TEST(Integration, TestClientPriorityAcquisitionVerification, "requestverification") {
    LOG_START
    // Read original
    std::string node = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE(ctx, original == 107);

    // Try invalid priority value = 2
    SysResource r{};
    r.mResCode = 0x80ff0002;
    r.mNumValues = 1;
    r.mResValue.value = 554;

    int64_t handle = tuneResources(-1, /*invalid*/2, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    // Give the system time; value should not change
    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, original);

    LOG_END
}


MT_TEST(Integration, TestInvalidResourceTuning, "requestverification") {
    LOG_START
    std::string validNode = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(validNode));
    MT_REQUIRE(ctx, original == 107);

        SysResource* resourceList = new SysResource[2];
        memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = 0x80ff0002;
        resourceList[0].mNumValues = 1;
        resourceList[0].mResValue.value = 554;

        // No Resource with this ID exists
        memset(&resourceList[1], 0, sizeof(SysResource));
        resourceList[1].mResCode = 12000;
        resourceList[1].mNumValues = 1;
        resourceList[1].mResValue.value = 597;

    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 2, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(validNode));
    MT_REQUIRE_EQ(ctx, now, original);
    delete resourceList;
    LOG_END
}



MT_TEST(Integration, TestOutOfBoundsResourceTuning, "requestverification") {
    LOG_START
    std::string node = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE(ctx, original == 107);

    SysResource r{};
    r.mResCode = 0x80ff0002;
    r.mNumValues = 1;
    r.mResValue.value = 1200;     // deliberately out of configured bounds

    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, original);

    LOG_END
}



MT_TEST(Integration, TestResourceLogicalToPhysicalTranslationVerification1, "requestverification") {
    LOG_START
    std::string node = "/etc/urm/tests/nodes/target_test_resource2.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE(ctx, original == 333);

    SysResource r{};
    r.mResCode = 0x80ff0005;
    r.mNumValues = 1;
    r.mResValue.value = 2300;
    r.mResInfo = 0;
    // set invalid logical cluster/core to force translation failure
    r.mResInfo = SET_RESOURCE_CLUSTER_VALUE(r.mResInfo, 2);
    r.mResInfo = SET_RESOURCE_CORE_VALUE(r.mResInfo, 27);

    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, original);

    LOG_END
}


MT_TEST(Integration, TestResourceLogicalToPhysicalTranslationVerification2, "requestverification") {
    LOG_START

    std::string node = "/etc/urm/tests/nodes/target_test_resource2.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE(ctx, original == 333);

    SysResource r{};
    r.mResCode = 0x80ff0005;
    r.mNumValues = 1;
    r.mResValue.value = 2300;
    r.mResInfo = 0;
    // Force invalid translation
    r.mResInfo = SET_RESOURCE_CLUSTER_VALUE(r.mResInfo, 8);
    r.mResInfo = SET_RESOURCE_CORE_VALUE(r.mResInfo, 2);

    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, original);

    LOG_END
}


MT_TEST(Integration, TestResourceLogicalToPhysicalTranslationVerification3, "requestverification") {
    LOG_START
    // If your platform map is needed, call SetUp() or __ensure_baseline();
    // __ensure_baseline();

    // If your test device doesn’t have logical cluster 0 → skip (optional).
    // (If you already consumed baseline elsewhere, you can derive this check similarly.)

    std::string node = "/etc/urm/tests/nodes/target_test_resource2.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE(ctx, original == 333);

    SysResource r{};
    r.mResCode = 0x80ff0005;
    r.mNumValues = 1;
    r.mResValue.value = 2300;
    r.mResInfo = 0;
    // Valid mapping for your env (logical cluster 0, core 1)
    r.mResInfo = SET_RESOURCE_CLUSTER_VALUE(r.mResInfo, 0);
    r.mResInfo = SET_RESOURCE_CORE_VALUE(r.mResInfo, 1);

    int64_t handle = tuneResources(5000, RequestPriority::REQ_PRIORITY_HIGH, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, 2300);

    std::this_thread::sleep_for(std::chrono::seconds(5));
    now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, original);

    LOG_END
}


MT_TEST(Integration, TestResourceLogicalToPhysicalTranslationVerification4, "requestverification") {
    LOG_START

    std::string node = "/etc/urm/tests/nodes/target_test_resource2.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE(ctx, original == 333);

    SysResource r{};
    r.mResCode = 0x80ff0005;
    r.mNumValues = 1;
    r.mResValue.value = 2300;
    r.mResInfo = 0;
    // Another invalid mapping
    r.mResInfo = SET_RESOURCE_CLUSTER_VALUE(r.mResInfo, 1);
    r.mResInfo = SET_RESOURCE_CORE_VALUE(r.mResInfo, 87);

    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, original);

    LOG_END
}


MT_TEST(Integration, TestNonSupportedResourceTuningVerification, "requestverification") {
    LOG_START

    std::string node = "/etc/urm/tests/nodes/target_test_resource4.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE(ctx, original == 516);

    SysResource r{};
    r.mResCode = 0x80ff0007;  // non-supported by config
    r.mNumValues = 1;
    r.mResValue.value = 653;

    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, original);

    LOG_END
}


MT_TEST(Integration, TestResourceOperationModeVerification, "requestverification") {
    LOG_START

    std::string node = "/etc/urm/tests/nodes/target_test_resource3.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE(ctx, original == 4400);

    SysResource r{};
    r.mResCode = 0x80ff0006;    // allowed only in specific mode
    r.mNumValues = 1;
    r.mResValue.value = 4670;

    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, original);

    LOG_END
}



MT_TEST(Integration, TestClientPermissionChecksVerification, "requestverification") {
    LOG_START

    std::string node = "/etc/urm/tests/nodes/target_test_resource1.txt";
    int32_t original = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE(ctx, original == 240);

    SysResource r{};
    r.mResCode = 0x80ff0004;  // system-permission required
    r.mNumValues = 1;
    r.mResValue.value = 460;

    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, &r);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    int32_t now = C_STOI(AuxRoutines::readFromFile(node));
    MT_REQUIRE_EQ(ctx, now, original);

    LOG_END
}




//...............................................................................................................


MT_TEST(Integration, SignalTestNullOrInvalidRequestVerification, "signal-verification") {
    LOG_START
    int64_t handle = tuneSignal(1, 0, -2, RequestPriority::REQ_PRIORITY_HIGH, "app-name", "scenario-zip", 0, nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE_EQ(ctx, handle, RC_REQ_SUBMISSION_FAILURE);
}


MT_TEST(Integration, SignalTestClientPermissionChecksVerification, "signal-verification") {
    LOG_START
    std::string testResourceName = "/etc/urm/tests/nodes/sched_util_clamp_max.txt";
    int32_t testResourceOriginalValue = 684;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);

    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    int64_t handle =
    tuneSignal(
        CUSTOM(CONSTRUCT_RES_CODE(0x0001, 0x0d)),
        DEFAULT_SIGNAL_TYPE,
        5000,
        RequestPriority::REQ_PRIORITY_HIGH,
        "app-name",
        "scenario-zip",
        0, nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    LOG_END
}


MT_TEST(Integration, SignalTestOutOfBoundsResourceTuning, "signal-verification") {
    LOG_START
    std::string testResourceName = "/etc/urm/tests/nodes/sched_util_clamp_min.txt";
    int32_t testResourceOriginalValue = 300;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    int64_t handle =
    tuneSignal(
        CUSTOM(CONSTRUCT_SIG_CODE(0x0d, 0x0002)),
        DEFAULT_SIGNAL_TYPE,
        5000,
        RequestPriority::REQ_PRIORITY_HIGH,
        "app-name",
        "scenario-zip",
        0, nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);

    LOG_END
}


MT_TEST(Integration, SignalTestTargetCompatabilityVerificationChecks, "signal-verification") {
    LOG_START
    std::string testResourceName = "/etc/urm/tests/nodes/sched_util_clamp_min.txt";
    int32_t testResourceOriginalValue = 300;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    int64_t handle = tuneSignal(
        CUSTOM(CONSTRUCT_SIG_CODE(0x0d, 0x0000)),
        DEFAULT_SIGNAL_TYPE,
        5000,
        RequestPriority::REQ_PRIORITY_HIGH,
        "app-name",
        "scenario-zip",
        0, nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);

    LOG_END
}


MT_TEST(Integration, SignalTestNonSupportedSignalProvisioningVerification, "signal-verification") {
    LOG_START
    std::string testResourceName = "/etc/urm/tests/nodes/sched_util_clamp_min.txt";
    int32_t testResourceOriginalValue = 300;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    int64_t handle = tuneSignal(
        CUSTOM(CONSTRUCT_SIG_CODE(0x0d, 0x0003)),
        DEFAULT_SIGNAL_TYPE,
        5000,
        RequestPriority::REQ_PRIORITY_HIGH,
        "app-name",
        "scenario-zip",
        0, nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);

    LOG_END
}


// Wrap as a mini.hpp test: Single client, single resource apply & reset
MT_TEST(Integration, TestSingleClientTuneRequest, "request-application") {
    LOG_START
    std::string testResourceName = "/etc/urm/tests/nodes/sched_util_clamp_min.txt";
    int32_t testResourceOriginalValue = 300;

    // Check the original value for the Resource
    std::string value = AuxRoutines::readFromFile(testResourceName);
    int32_t originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    SysResource* resourceList = new SysResource[1];
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0000));
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 980;

    int64_t handle = tuneResources(5000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    
    
    MT_CHECK(ctx, handle > 0);
    if (ctx.failed) return; // bail out to avoid cascading errors


    std::this_thread::sleep_for(std::chrono::seconds(1));
    // Check if the new value was successfully written to the node
    value = AuxRoutines::readFromFile(testResourceName);
    int32_t newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 980);

    std::this_thread::sleep_for(std::chrono::seconds(6));
    // Wait for the Request to expire, check if the value resets
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, originalValue);

    delete[] resourceList;
    LOG_END
}



MT_TEST(Integration, TestSingleClientTuneRequestMultipleResources, "request-application") {
    LOG_START

    // File paths
    const std::string max_freq_path  = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    const std::string min_freq_path  = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    const std::string clamp_max_path = "/etc/urm/tests/nodes/sched_util_clamp_max.txt";

    // Expected originals
    const int32_t max_freq_orig  = 114;
    const int32_t min_freq_orig  = 107;
    const int32_t clamp_max_orig = 684;

    // Read originals
    int32_t originalValue[3];
    std::string value;

    value = AuxRoutines::readFromFile(max_freq_path);
    originalValue[0] = C_STOI(value);
    std::cout << LOG_BASE << max_freq_path << " Original Value: " << originalValue[0] << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue[0], max_freq_orig);

    value = AuxRoutines::readFromFile(min_freq_path);
    originalValue[1] = C_STOI(value);
    std::cout << LOG_BASE << min_freq_path << " Original Value: " << originalValue[1] << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue[1], min_freq_orig);

    value = AuxRoutines::readFromFile(clamp_max_path);
    originalValue[2] = C_STOI(value);
    std::cout << LOG_BASE << clamp_max_path << " Original Value: " << originalValue[2] << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue[2], clamp_max_orig);

    // Prepare resources (codes must match names!)
    SysResource* resourceList = new SysResource[3];

    // scaling_max_freq -> 765
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0003)); // max_freq
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 765;

    // sched_util_clamp_max -> 889
    memset(&resourceList[1], 0, sizeof(SysResource));
    resourceList[1].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0001)); // clamp_max
    resourceList[1].mNumValues = 1;
    resourceList[1].mResValue.value = 889;

    // scaling_min_freq -> 617
    memset(&resourceList[2], 0, sizeof(SysResource));
    resourceList[2].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0002)); // min_freq
    resourceList[2].mNumValues = 1;
    resourceList[2].mResValue.value = 617;

    // Apply
    int64_t handle = tuneResources(6000, RequestPriority::REQ_PRIORITY_HIGH, 3, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    if (handle <= 0) {
        std::ostringstream oss;
        oss << "tuneResources invalid handle: " << handle
            << " (errno=" << errno << " - " << std::strerror(errno) << ")";
        delete[] resourceList;
        MT_FAIL(ctx, oss.str());
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Verify configured
    int32_t newValue;

    value = AuxRoutines::readFromFile(max_freq_path);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << max_freq_path << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 765);

    value = AuxRoutines::readFromFile(min_freq_path);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << min_freq_path << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 617);

    value = AuxRoutines::readFromFile(clamp_max_path);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << clamp_max_path << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 889);

    std::this_thread::sleep_for(std::chrono::seconds(6));

    // Verify reset (add small grace window for min_freq)
    value = AuxRoutines::readFromFile(max_freq_path);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << max_freq_path << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, originalValue[0]);

    // min_freq reset can be sticky — poll briefly
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
        do {
            value = AuxRoutines::readFromFile(min_freq_path);
            newValue = C_STOI(value);
            if (newValue == originalValue[1]) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } while (std::chrono::steady_clock::now() < deadline);
    }
    std::cout << LOG_BASE << min_freq_path << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, originalValue[1]);

    value = AuxRoutines::readFromFile(clamp_max_path);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << clamp_max_path << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, originalValue[2]);

    delete[] resourceList;
    LOG_END
}


// Wrap as a mini.hpp test: Two clients; "Higher-Is-Better" → higher value applied, then reset
MT_TEST(Integration, TestMultipleClientsHigherIsBetterPolicy1, "request-application") {
    LOG_START
    // Check the original value for the Resource
    std::string testResourceName = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    int32_t testResourceOriginalValue = 114;
    std::string value = AuxRoutines::readFromFile(testResourceName);
    int32_t originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;

    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    int32_t rc = fork();
    if (rc == 0) {
        SysResource* resourceList = new SysResource[1];
        memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0003));
        resourceList[0].mNumValues = 1;
        resourceList[0].mResValue.value = 315;
        int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        delete resourceList;
        exit(EXIT_SUCCESS);
    } else if (rc > 0) {
        wait(nullptr);
        SysResource* resourceList = new SysResource[1];
        memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0003));
        resourceList[0].mNumValues = 1;
        resourceList[0].mResValue.value = 209;
        int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Check if the new value was successfully written to the node
        value = AuxRoutines::readFromFile(testResourceName);
        int32_t newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
        // Higher value (315) should be in effect
       

        MT_REQUIRE_EQ(ctx, newValue, 315);

        std::this_thread::sleep_for(std::chrono::seconds(8));
        // Wait for the Request to expire, check if the value resets
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
        
        MT_REQUIRE_EQ(ctx, newValue, originalValue);

        delete[] resourceList;
    }
    LOG_END
}


// Wrap as a mini.hpp test: Two clients, different durations; "Higher-Is-Better" timeline
MT_TEST(Integration, TestMultipleClientsHigherIsBetterPolicy2, "request-application") {
    LOG_START
    // Check the original value for the Resource
    std::string testResourceName = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    int32_t testResourceOriginalValue = 114;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
   
    
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    int32_t rc1 = fork();
    if (rc1 == 0) {
        SysResource* resourceList = new SysResource[1];
        memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0003));
        resourceList[0].mNumValues = 1;
        resourceList[0].mResValue.value = 1176;
        int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        delete resourceList;
        exit(EXIT_SUCCESS);
    } else if (rc1 > 0) {
        wait(nullptr);
        SysResource* resourceList = new SysResource[1];
        memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0003));
        resourceList[0].mNumValues = 1;
        resourceList[0].mResValue.value = 823;
        int64_t handle = tuneResources(14000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        // Higher (1176) should be applied first
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
       
        MT_REQUIRE_EQ(ctx, newValue, 1176);

        std::this_thread::sleep_for(std::chrono::seconds(6));
        // After first expires, the second value (823) should take effect
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
       
       MT_REQUIRE_EQ(ctx, newValue, 823);

        std::this_thread::sleep_for(std::chrono::seconds(10));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Reset Value: " << originalValue << std::endl;
        
        MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

        delete[] resourceList;
    }
    LOG_END
}


// Wrap as a mini.hpp test: Four clients; "Lower-Is-Better" → smallest value applied across equal durations
MT_TEST(Integration, TestMultipleClientsLowerIsBetterPolicy, "request-application") {
    LOG_START
    // Check the original value for the Resource
    std::string testResourceName = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    int32_t testResourceOriginalValue = 107;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;

    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    int64_t handle;
    int32_t rc1 = fork();
    if (rc1 == 0) {
        SysResource* resourceList = new SysResource[1];
        memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0002));
        resourceList[0].mNumValues = 1;
        resourceList[0].mResValue.value = 578;
        handle = tuneResources(15000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
        // Give time for Resource Tuner to read process status, else the Request will be dropped
        std::this_thread::sleep_for(std::chrono::seconds(3));
        delete resourceList;
        exit(EXIT_SUCCESS);
    } else if (rc1 > 0) {
        wait(nullptr);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;

        MT_REQUIRE_EQ(ctx, newValue, 578);

        int32_t rc2 = fork();
        if (rc2 == 0) {
            SysResource* resourceList = new SysResource[1];
            memset(&resourceList[0], 0, sizeof(SysResource));
            resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0002));
            resourceList[0].mNumValues = 1;
            resourceList[0].mResValue.value = 445;
            handle = tuneResources(15000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
            std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            delete resourceList;
            exit(EXIT_SUCCESS);
        } else if (rc2 > 0) {
            wait(nullptr);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            value = AuxRoutines::readFromFile(testResourceName);
            newValue = C_STOI(value);
            std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;

            MT_REQUIRE_EQ(ctx, newValue, 445);

            int32_t rc3 = fork();
            if (rc3 == 0) {
                SysResource* resourceList = new SysResource[1];
                memset(&resourceList[0], 0, sizeof(SysResource));
                resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0002));
                resourceList[0].mNumValues = 1;
                resourceList[0].mResValue.value = 412;
                handle = tuneResources(15000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
                std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(3));
                delete resourceList;
                exit(EXIT_SUCCESS);
            } else if (rc3 > 0) {
                wait(nullptr);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                value = AuxRoutines::readFromFile(testResourceName);
                newValue = C_STOI(value);
                std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;

                MT_REQUIRE_EQ(ctx, newValue, 412);

                SysResource* resourceList = new SysResource[1];
                memset(&resourceList[0], 0, sizeof(SysResource));
                resourceList[0].mResCode = CUSTOM(CONSTRUCT_RES_CODE(0xff, 0x0002));
                resourceList[0].mNumValues = 1;
                resourceList[0].mResValue.value = 378;
                handle = tuneResources(15000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
                std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(1));
                value = AuxRoutines::readFromFile(testResourceName);
                newValue = C_STOI(value);
                std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;

                MT_REQUIRE_EQ(ctx, newValue, 378);

                std::this_thread::sleep_for(std::chrono::seconds(30));
                value = AuxRoutines::readFromFile(testResourceName);
                newValue = C_STOI(value);
                std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
                
               MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);

                delete resourceList;
            }
        }
    }
    LOG_END
}

// Wrap as a mini.hpp test: Two clients; "Lazy-Apply" policy applies in FIFO order
MT_TEST(integration, TestMultipleClientsLazyApplyPolicy, "request-application") {
    (void)ctx; // silence unused parameter warning
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/target_test_resource5.txt";
    int32_t testResourceOriginalValue = 17;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    int64_t handle;
    int32_t rc1 = fork();
    if (rc1 == 0) {
        SysResource* resourceList = new SysResource[1];
        memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = 0x80ff0008;
        resourceList[0].mNumValues = 1;
        resourceList[0].mResValue.value = 15;
        handle = tuneResources(12000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        delete resourceList;
        exit(EXIT_SUCCESS);
    } else if (rc1 > 0) {
        wait(nullptr);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        SysResource* resourceList = new SysResource[1];
        memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = 0x80ff0008;
        resourceList[0].mNumValues = 1;
        resourceList[0].mResValue.value = 18;
        handle = tuneResources(15000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
        // The first request (15) should remain applied until it expires (FIFO/Lazy-Apply).
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
        
        MT_REQUIRE_EQ(ctx, newValue, 15);

        std::this_thread::sleep_for(std::chrono::seconds(3));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
        MT_REQUIRE_EQ(ctx, newValue, 15);

        std::this_thread::sleep_for(std::chrono::seconds(8));
        // After the first (15) expires, the second (18) should take effect.
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
        MT_REQUIRE_EQ(ctx, newValue, 18);

        std::this_thread::sleep_for(std::chrono::seconds(12));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
        MT_REQUIRE_EQ(ctx, newValue, originalValue);

        delete[] resourceList;
    }

    LOG_END
}


// Wrap as a mini.hpp test: Single client issues sequential requests; higher takes effect, then resets
MT_TEST(Integration, TestSingleClientSequentialRequests, "request-application") {
    (void)ctx; // silence unused parameter warning
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    int32_t testResourceOriginalValue = 114;
    int64_t handle;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);
    SysResource* resourceList1 = new SysResource[1];
    memset(&resourceList1[0], 0, sizeof(SysResource));
    resourceList1[0].mResCode = 0x80ff0003;
    resourceList1[0].mNumValues = 1;
    resourceList1[0].mResValue.value = 889;

    SysResource* resourceList2 = new SysResource[1];
    memset(&resourceList2[0], 0, sizeof(SysResource));
    resourceList2[0].mResCode = 0x80ff0003;
    resourceList2[0].mNumValues = 1;
    resourceList2[0].mResValue.value = 917;

    handle = tuneResources(6000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList1);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    handle = tuneResources(6000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList2);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 917);
    std::this_thread::sleep_for(std::chrono::seconds(6));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    delete[] resourceList1;
    delete[] resourceList2;

    LOG_END
}

MT_TEST(Integration, TestMultipleClientTIDsConcurrentRequests, "request-application") {
    (void)ctx; // silence unused parameter warning
    LOG_START

    // --- Test configuration ---
    const std::string testResourceName = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    const int32_t expectedOriginalValue = 114;
    const int32_t valueThread = 664;
    const int32_t valueMain   = 702;     // Expect "higher wins" => 702
    const auto    reqTtlMs    = 6000;    // tuneResources TTL
    const auto    applyWait   = std::chrono::seconds(5);
    const auto    resetWait   = std::chrono::seconds(8);

    // --- Helpers ---
    auto read_int = [&](const std::string& path) -> int32_t {
        std::string s = AuxRoutines::readFromFile(path);
        return C_STOI(s);
    };

    auto poll_until = [&](int32_t expected,
                          std::chrono::milliseconds timeout,
                          std::chrono::milliseconds interval = std::chrono::milliseconds(100)) -> bool
    {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout) {
            int32_t v = read_int(testResourceName);
            if (v == expected) return true;
            std::this_thread::sleep_for(interval);
        }
        return false;
    };

    // --- Capture original ---
    int32_t originalValue = read_int(testResourceName);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue, expectedOriginalValue);

    // --- Start concurrent request in worker thread ---
    std::thread th([&]{
        SysResource* resourceList1 = new SysResource[1];
        std::memset(&resourceList1[0], 0, sizeof(SysResource));
        resourceList1[0].mResCode = 0x80ff0003;
        resourceList1[0].mOptionalInfo = 0;
        resourceList1[0].mNumValues = 1;
        resourceList1[0].mResValue.value = valueThread;

        int64_t handle1 = tuneResources(reqTtlMs, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList1);
        std::cout << LOG_BASE << "Handle Returned (thread): " << handle1 << std::endl;

        delete[] resourceList1;   // FIX: correct array delete
    });

    // --- Main thread request ---
    SysResource* resourceList2 = new SysResource[1];
    std::memset(&resourceList2[0], 0, sizeof(SysResource));
    resourceList2[0].mResCode = 0x80ff0003;
    resourceList2[0].mOptionalInfo = 0;
    resourceList2[0].mNumValues = 1;
    resourceList2[0].mResValue.value = valueMain;

    int64_t handle2 = tuneResources(reqTtlMs, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList2);
    std::cout << LOG_BASE << "Handle Returned (main): " << handle2 << std::endl;

    delete[] resourceList2;

    // --- Ensure thread is joined BEFORE any assertion that might exit early ---
    if (th.joinable()) th.join();

    // --- Verify "higher wins" (expect 702) with a poll to avoid flakiness ---
    bool configuredOk = poll_until(valueMain, std::chrono::duration_cast<std::chrono::milliseconds>(applyWait));
    int32_t configuredValue = read_int(testResourceName);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << configuredValue << std::endl;

    // If poll failed, assert on the last observed value so the log shows it
    MT_REQUIRE_EQ(ctx, configuredOk ? valueMain : configuredValue, valueMain);

    // --- Verify reset after TTL (6s) with some buffer ---
    bool resetOk = poll_until(expectedOriginalValue, std::chrono::duration_cast<std::chrono::milliseconds>(resetWait));
    int32_t resetValue = read_int(testResourceName);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << resetValue << std::endl;
    MT_REQUIRE_EQ(ctx, resetOk ? expectedOriginalValue : resetValue, expectedOriginalValue);

    LOG_END
}

// Wrap as a mini.hpp test: Infinite duration tune, then valid untune → node resets
MT_TEST(Integration, TestInfiniteDurationTuneRequestAndValidUntuning, "request-application") {
    (void)ctx; // silence unused parameter warning
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    int32_t testResourceOriginalValue = 107;
    int64_t handle;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    SysResource* resourceList = new SysResource[1];
    SysResource resource = {0};
    memset(&resource, 0, sizeof(SysResource));
    resource.mResCode = 0x80ff0002;
    resource.mNumValues = 1;
    resource.mResValue.value = 245;
    resourceList[0] = resource;

    handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 245);
    int8_t status = untuneResources(handle);
    std::cout << LOG_BASE << " Untune Status: " << (int32_t)status << std::endl;
    MT_REQUIRE_EQ(ctx, status, 0);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Untuned Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    delete[] resourceList;
    LOG_END
}



// Wrap as a mini.hpp test: Higher priority request should take effect over lower priority
MT_TEST(Integration, TestPriorityBasedResourceAcquisition1, "request-application") {
    (void)ctx; // silence unused parameter warning
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    int32_t testResourceOriginalValue = 107;
    int64_t handle;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);
    SysResource* resourceList1 = new SysResource[1];
    memset(&resourceList1[0], 0, sizeof(SysResource));
    resourceList1[0].mResCode = 0x80ff0002;
    resourceList1[0].mNumValues = 1;
    resourceList1[0].mResValue.value = 515;

    handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_LOW, 1, resourceList1);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    SysResource* resourceList2 = new SysResource[1];
    memset(&resourceList2[0], 0, sizeof(SysResource));
    resourceList2[0].mResCode = 0x80ff0002;
    resourceList2[0].mNumValues = 1;
    resourceList2[0].mResValue.value = 559;

    handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList2);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 559);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    delete[] resourceList1;
    delete[] resourceList2;

    LOG_END
}


// Higher priority request should override lower priority; timeline across staggered arrivals
MT_TEST(Integration, TestPriorityBasedResourceAcquisition2, "request-application") {
    (void)ctx;
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    int32_t testResourceOriginalValue = 107;
    int64_t handle;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);
    SysResource* resourceList1 = new SysResource[1];
    memset(&resourceList1[0], 0, sizeof(SysResource));
    resourceList1[0].mResCode = 0x80ff0002;
    resourceList1[0].mNumValues = 1;
    resourceList1[0].mResValue.value = 515;
    handle = tuneResources(12000, RequestPriority::REQ_PRIORITY_LOW, 1, resourceList1);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 515);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    SysResource* resourceList2 = new SysResource[1];
    memset(&resourceList2[0], 0, sizeof(SysResource));
    resourceList2[0].mResCode = 0x80ff0002;
    resourceList2[0].mNumValues = 1;
    resourceList2[0].mResValue.value = 559;
    handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList2);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 559);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    delete[] resourceList1;
    delete[] resourceList2;

    LOG_END
}


// Higher priority (P1) keeps lower priority (P2) from taking effect even if V2 > V1
MT_TEST(Integration, TestPriorityBasedResourceAcquisition3, "request-application") {
    (void)ctx;
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    int32_t testResourceOriginalValue = 114;
    int64_t handle;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);
    SysResource* resourceList1 = new SysResource[1];
    memset(&resourceList1[0], 0, sizeof(SysResource));
    resourceList1[0].mResCode = 0x80ff0003;
    resourceList1[0].mOptionalInfo = 0;
    resourceList1[0].mNumValues = 1;
    resourceList1[0].mResValue.value = 645;
    handle = tuneResources(10000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList1);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 645);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SysResource* resourceList2 = new SysResource[1];
    memset(&resourceList2[0], 0, sizeof(SysResource));
    resourceList2[0].mResCode = 0x80ff0003;
    resourceList2[0].mNumValues = 1;
    resourceList2[0].mResValue.value = 716;
    handle = tuneResources(5000, RequestPriority::REQ_PRIORITY_LOW, 1, resourceList2);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 645);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    delete[] resourceList1;
    delete[] resourceList2;

    LOG_END
}


// Valid retune extends duration; value remains applied until extended expiry, then resets
MT_TEST(Integration, TestRequestValidRetuning, "request-application") {
    (void)ctx;
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    int32_t testResourceOriginalValue = 114;
    int64_t handle;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);
    SysResource* resourceList = new SysResource[1];
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x80ff0003;
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 778;

    handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(4));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 778);
    // Extend duration to 15 seconds
    int8_t status = retuneResources(handle, 15000);
    std::cout << LOG_BASE << "Retune Status: " << (int32_t)status << std::endl;
    MT_REQUIRE_EQ(ctx, status, 0);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 778);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    delete[] resourceList;

    LOG_END
}


// Invalid retune (decreasing duration) should be rejected; original expiry still applies
MT_TEST(Integration, TestRequestInvalidRetuning1, "request-application") {
    (void)ctx;
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    int32_t testResourceOriginalValue = 114;
    int64_t handle;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);
    SysResource* resourceList = new SysResource[1];
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x80ff0003;
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 778;

    handle = tuneResources(12000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 778);
    // Attempt to decrease duration → server should reject; value stays applied until original expiry
    int8_t status = retuneResources(handle, 6000);
    std::cout << LOG_BASE << "Retune Status: " << (int32_t)status << std::endl;
    MT_REQUIRE_EQ(ctx, status, 0);
    std::this_thread::sleep_for(std::chrono::seconds(7));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 778);
    std::this_thread::sleep_for(std::chrono::seconds(6));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    delete[] resourceList;

    LOG_END
}



// Wrap as a mini.hpp test: Cluster-type resource, logical cluster 0 → apply & reset
MT_TEST(Integration, TestClusterTypeResourceTuneRequest1, "request-application") {
    (void)ctx; // silence unused parameter warning
    LOG_START

    __ensure_baseline(); // ensure baseline fetch for cluster mapping

    int32_t physicalClusterID = baseline.getExpectedPhysicalCluster(0);
    if (physicalClusterID == -1) {
        LOG_SKIP("Logical Cluster: 0 not found on test device, Skipping Test Case")
        return;
    }

    std::string nodePath = "/etc/urm/tests/nodes/cluster_type_resource_%d_cluster_id.txt";
    char path[128];
    snprintf(path, sizeof(path), nodePath.c_str(), physicalClusterID);
    std::string testResourceName = std::string(path);
    int32_t testResourceOriginalValue = 180;

    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);
    SysResource* resourceList = new SysResource[1];
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x80ff000a;
    resourceList[0].mResInfo = SET_RESOURCE_CLUSTER_VALUE(resourceList[0].mResInfo, 0);
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 440;

    int64_t handle = tuneResources(7000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 440);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    delete[] resourceList;
    LOG_END
}


// Wrap as a mini.hpp test: Cluster-type resource, logical cluster 2 → apply & reset
MT_TEST(Integration, TestClusterTypeResourceTuneRequest2, "request-application") {
    (void)ctx; // silence unused parameter warning
    LOG_START

    __ensure_baseline(); // ensure baseline fetch for cluster mapping

    int32_t physicalClusterID = baseline.getExpectedPhysicalCluster(2);
    if (physicalClusterID == -1) {
        LOG_SKIP("Logical Cluster: 2 not found on test device, Skipping Test Case")
        return;
    }

    std::string nodePath = "/etc/urm/tests/nodes/cluster_type_resource_%d_cluster_id.txt";
    char path[128];
    snprintf(path, sizeof(path), nodePath.c_str(), physicalClusterID);
    std::string testResourceName = std::string(path);
    int32_t testResourceOriginalValue = 180;

    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);   
    SysResource* resourceList = new SysResource[1];
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x80ff000a;
    resourceList[0].mResInfo = SET_RESOURCE_CLUSTER_VALUE(resourceList[0].mResInfo, 2);
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 440;

    int64_t handle = tuneResources(7000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 440);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    delete[] resourceList;
    LOG_END
}


MT_TEST(Integration, TestSingleClientTuneSignal1, "signal-application") {
    (void)ctx;
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/sched_util_clamp_min.txt";
    int32_t testResourceOriginalValue = 300;
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    int64_t handle =
    tuneSignal(
        CUSTOM(CONSTRUCT_SIG_CODE(0x0d, 0x0004)),
        DEFAULT_SIGNAL_TYPE,
        5000,
        RequestPriority::REQ_PRIORITY_HIGH,
        "app-name",
        "scenario-zip",
        0,
        nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, 917);
    std::this_thread::sleep_for(std::chrono::seconds(8));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);
    LOG_END
}


MT_TEST(Integration, TestSingleClientTuneSignal2, "signal-application") {
    (void)ctx;
    LOG_START

    std::string testResourceName1 = "/etc/urm/tests/nodes/sched_util_clamp_min.txt";
    std::string testResourceName2 = "/etc/urm/tests/nodes/sched_util_clamp_max.txt";
    std::string testResourceName3 = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    int32_t originalValues[] = {300, 684, 114};
    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName1);
    originalValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, originalValue, originalValues[0]);
    value = AuxRoutines::readFromFile(testResourceName2);
    originalValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, originalValue, originalValues[1]);
    value = AuxRoutines::readFromFile(testResourceName3);
    originalValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, originalValue, originalValues[2]);
    int64_t handle =
    tuneSignal(
        CUSTOM(CONSTRUCT_SIG_CODE(0x0d, 0x0005)),
        DEFAULT_SIGNAL_TYPE,
        5000,
        RequestPriority::REQ_PRIORITY_HIGH,
        "app-name",
        "scenario-zip",
        0,
        nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName1);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, 883);
    value = AuxRoutines::readFromFile(testResourceName2);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, 920);
    value = AuxRoutines::readFromFile(testResourceName3);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, 1555);
    std::this_thread::sleep_for(std::chrono::seconds(8));
    value = AuxRoutines::readFromFile(testResourceName1);
    originalValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, originalValue, originalValues[0]); 
    value = AuxRoutines::readFromFile(testResourceName2);
    originalValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, originalValue, originalValues[1]);
    value = AuxRoutines::readFromFile(testResourceName3);
    originalValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, originalValue, originalValues[2]);
    LOG_END
}


MT_TEST(Integration, TestSignalUntuning, "signal-application") {
    LOG_START

    const std::string testResourceName = "/etc/urm/tests/nodes/sched_util_clamp_min.txt";
    const int32_t testResourceOriginalValue = 300;

    std::string value;
    int32_t originalValue = 0, newValue = 0;

    // Read original
    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    // Optional: enforce baseline if you want strict start state
    MT_REQUIRE_EQ(ctx, originalValue, testResourceOriginalValue);

    // Apply signal
    int64_t handle =
        tuneSignal(
            CUSTOM(CONSTRUCT_SIG_CODE(0x0d, 0x0004)),
            DEFAULT_SIGNAL_TYPE,
            -1,
            RequestPriority::REQ_PRIORITY_HIGH,
            "app-name",
            "scenario-zip",
            0,
            nullptr);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE(ctx, handle > 0);

    // Wait and verify configured value
    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 917);

    // Ensure still tuned before untune
    std::this_thread::sleep_for(std::chrono::seconds(8));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, 917);

    // Untune and verify status
    int8_t status = untuneSignal(handle);
    MT_REQUIRE_EQ(ctx, status, 0);

    // Confirm reset to original
    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, testResourceOriginalValue);

    LOG_END
}

MT_TEST(Integration, TestObservationSignal, "signal-application") {
    LOG_START

    std::vector<std::string> keys = {
        "/sys/fs/cgroup/camera-cgroup/cgroup.procs",
        "/sys/fs/cgroup/user.slice/cgroup.procs",
        "/sys/fs/cgroup/system.slice/cpu.weight",
        "/sys/fs/cgroup/system.slice/cpuset.cpus",
        "/sys/fs/cgroup/user.slice/cpuset.cpus",
        "/sys/fs/cgroup/user.slice/cpu.weight",
        "/sys/fs/cgroup/user.slice/memory.high",
        "/sys/fs/cgroup/camera-cgroup/cpuset.cpus",
        "/sys/fs/cgroup/camera-cgroup/cpu.weight",
        "/sys/fs/cgroup/camera-cgroup/cpu.weight.nice",
        "/sys/fs/cgroup/camera-cgroup/memory.low",
        "/sys/fs/cgroup/camera-cgroup/memory.min",
        "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq",
        "/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq",
        "/sys/devices/system/cpu/cpufreq/policy7/scaling_max_freq",
    };

    std::cout << "Initial" << std::endl;
    for (const std::string& key : keys) {
        std::cout << key << ": [" << AuxRoutines::readFromFile(key) << "]" << std::endl;
    }

    // Prepare PID list
    uint32_t* list = (uint32_t*) std::calloc(3, sizeof(uint32_t));
    list[0] = static_cast<uint32_t>(getpid());
    list[1] = static_cast<uint32_t>(getppid());
    list[2] = 2010;

    // Apply observation signal
    int64_t handle = tuneSignal(
        CUSTOM(CONSTRUCT_SIG_CODE(0x0d, 0x0009)),
        DEFAULT_SIGNAL_TYPE,
        0, 0, "", "",
        3,
        list);

    // Replace raw assert with framework macro
    MT_REQUIRE(ctx, handle > 0);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "Request Applied" << std::endl;
    for (const std::string& key : keys) {
        std::cout << key << ": [" << AuxRoutines::readFromFile(key) << "]" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(60));
    std::cout << "Request Reset" << std::endl;
    for (const std::string& key : keys) {
        std::cout << key << ": [" << AuxRoutines::readFromFile(key) << "]" << std::endl;
    }

    // Optional hygiene: free allocated list
    std::free(list);

    LOG_END
}


static std::string encodeCluster(const std::string& nodePath, int32_t physicalClusterID) {
    char path[128];
    std::snprintf(path, sizeof(path), nodePath.c_str(), physicalClusterID);
    return std::string(path);
}

MT_TEST(Integration, TestMultiResourceSignal, "signal-application") {
    LOG_START

    // Cluster resource template
    const std::string clusResource = "/etc/urm/tests/nodes/cluster_type_resource_%d_cluster_id.txt";

    // Baseline queries (assuming `baseline` is available in this TU)
    const int32_t physicalClusterID0 = baseline.getExpectedPhysicalCluster(0);
    const int32_t physicalClusterID1 = baseline.getExpectedPhysicalCluster(1);
    const int32_t physicalClusterID2 = baseline.getExpectedPhysicalCluster(2);

    // Resource set to validate (name + expected tuned value + recorded original)
    struct ResourceHolder {
        std::string name;
        int32_t     expectedValue;
        int32_t     originalValue;
    };

    std::vector<ResourceHolder> tunedResources = {
        { "/etc/urm/tests/nodes/sched_util_clamp_min.txt", 668,  -1 },
        { "/etc/urm/tests/nodes/sched_util_clamp_max.txt", 897,  -1 },
        { "/etc/urm/tests/nodes/target_test_resource1.txt", 231, -1 },
        { "/etc/urm/tests/nodes/scaling_max_freq.txt",     1533, -1 },
        { (physicalClusterID0 == -1) ? "" : encodeCluster(clusResource, physicalClusterID0),
          (physicalClusterID0 == -1) ? -1 : 1976, -1 },
        { (physicalClusterID1 == -1) ? "" : encodeCluster(clusResource, physicalClusterID1),
          (physicalClusterID1 == -1) ? -1 : 1989, -1 },
        { (physicalClusterID2 == -1) ? "" : encodeCluster(clusResource, physicalClusterID2),
          (physicalClusterID2 == -1) ? -1 : 2012, -1 },
        { "/etc/urm/tests/nodes/target_test_resource4.txt", 41128, -1 },
    };

    // Record original values for each non-empty resource
    for (size_t i = 0; i < tunedResources.size(); ++i) {
        if (!tunedResources[i].name.empty()) {
            tunedResources[i].originalValue = C_STOI(AuxRoutines::readFromFile(tunedResources[i].name));
        }
    }

    // Apply the multi-resource signal
    int64_t handle = tuneSignal(
        CUSTOM(CONSTRUCT_SIG_CODE(0x0d, 0x000a)),
        DEFAULT_SIGNAL_TYPE,
        /*target*/ 0,
        /*flags*/  0,
        /*app*/    "",
        /*scenario*/ "",
        /*list_len*/ 0,
        /*list*/     nullptr);

    // Replace raw assert with framework macro
    MT_REQUIRE(ctx, handle > 0);

    // Allow time for application
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Validate tuned values
    for (size_t i = 0; i < tunedResources.size(); ++i) {
        const auto& r = tunedResources[i];
        if (!r.name.empty() && r.expectedValue >= 0) {
            int32_t curValue = C_STOI(AuxRoutines::readFromFile(r.name));
            std::cout << LOG_BASE << r.name << " Configured Value: " << curValue << std::endl;
            std::cout << LOG_BASE << r.name << " Expected Config Val: " << r.expectedValue << std::endl;
            MT_REQUIRE_EQ(ctx, curValue, r.expectedValue);
        }
    }

    // Wait for expiry/reset (adjust if your timeouts differ)
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Validate reset to original values
    for (size_t i = 0; i < tunedResources.size(); ++i) {
        const auto& r = tunedResources[i];
        if (!r.name.empty() && r.originalValue >= 0) {
            int32_t curValue = C_STOI(AuxRoutines::readFromFile(r.name));
            std::cout << LOG_BASE << r.name << " Reset Value: " << curValue << std::endl;
            std::cout << LOG_BASE << r.name << " Expected Reset Val: " << r.originalValue << std::endl;
            MT_REQUIRE_EQ(ctx, curValue, r.originalValue);
        }
    }

    LOG_END
}

MT_TEST(Integration , TestCgroupWriteAndResetBasicCase, "cgroup") {
    LOG_START

    const std::string testResourceName = "/sys/fs/cgroup/audio-cgroup/cpu.uclamp.min";

    std::string originalValueString = AuxRoutines::readFromFile(testResourceName);
    int32_t originalValue = C_STOI(originalValueString);

    SysResource* resourceList = new SysResource[1];
    std::memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x00090007;
    resourceList[0].mNumValues = 2;
    resourceList[0].mResValue.values = new int32_t[resourceList[0].mNumValues];
    resourceList[0].mResValue.values[0] = 802;
    resourceList[0].mResValue.values[1] = 52;

    int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_LOW, 1, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE(ctx, handle > 0);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::string value = AuxRoutines::readFromFile(testResourceName);
    int32_t newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 52);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, originalValue);

    // cleanup
    delete[] resourceList[0].mResValue.values;
    delete[] resourceList;

    LOG_END
}

MT_TEST(Integration, TestCgroupWriteAndReset1, "cgroup") {
    LOG_START

    const std::string testResourceName = "/sys/fs/cgroup/audio-cgroup/cpu.uclamp.min";

    std::string originalValueString = AuxRoutines::readFromFile(testResourceName);
    int32_t originalValue = C_STOI(originalValueString);

    SysResource* resourceList1 = new SysResource[1];
    std::memset(&resourceList1[0], 0, sizeof(SysResource));
    resourceList1[0].mResCode = 0x00090007;
    resourceList1[0].mNumValues = 2;
    resourceList1[0].mResValue.values = new int32_t[2];
    resourceList1[0].mResValue.values[0] = 802;
    resourceList1[0].mResValue.values[1] = 52;

    int64_t handle = tuneResources(25000, RequestPriority::REQ_PRIORITY_LOW, 1, resourceList1);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE(ctx, handle > 0);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    SysResource* resourceList = new SysResource[1];
    std::memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x00090007;
    resourceList[0].mNumValues = 2;
    resourceList[0].mResValue.values = new int32_t[2];
    resourceList[0].mResValue.values[0] = 802;
    resourceList[0].mResValue.values[1] = 57;

    handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE(ctx, handle > 0);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::string value = AuxRoutines::readFromFile(testResourceName);
    int32_t newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, 57);

    std::this_thread::sleep_for(std::chrono::seconds(8));

    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, 52);

    std::this_thread::sleep_for(std::chrono::seconds(20));

    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    MT_REQUIRE_EQ(ctx, newValue, originalValue);

    // cleanup
    delete[] resourceList[0].mResValue.values;
    delete[] resourceList;
    delete[] resourceList1[0].mResValue.values;
    delete[] resourceList1;

    LOG_END
}

MT_TEST(Integration, TestCgroupWriteAndReset2, "cgroup") {
    LOG_START

    const std::string testResourceName = "/sys/fs/cgroup/audio-cgroup/cpu.uclamp.min";

    std::string originalValueString = AuxRoutines::readFromFile(testResourceName);
    int32_t originalValue = C_STOI(originalValueString);

    int32_t rc = fork();
    if (rc == 0) {
        // child: no MT_* macros here
        SysResource* resourceList = new SysResource[1];
        std::memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = 0x00090007;
        resourceList[0].mNumValues = 2;
        resourceList[0].mResValue.values = new int32_t[2];
        resourceList[0].mResValue.values[0] = 802;
        resourceList[0].mResValue.values[1] = 53;

        int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(3));
        delete[] resourceList[0].mResValue.values;
        delete[] resourceList;
        std::exit(EXIT_SUCCESS);

    } else if (rc > 0) {
        SysResource* resourceList = new SysResource[1];
        std::memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = 0x00090007;
        resourceList[0].mNumValues = 2;
        resourceList[0].mResValue.values = new int32_t[2];
        resourceList[0].mResValue.values[0] = 802;
        resourceList[0].mResValue.values[1] = 57;

        int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
        MT_REQUIRE(ctx, handle > 0);

        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::string value = AuxRoutines::readFromFile(testResourceName);
        int32_t newValue = C_STOI(value);
        MT_REQUIRE_EQ(ctx, newValue, 53); // higher (child’s) should win here

        std::this_thread::sleep_for(std::chrono::seconds(10));

        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        MT_REQUIRE_EQ(ctx, newValue, originalValue);

        delete[] resourceList[0].mResValue.values;
        delete[] resourceList;
        wait(nullptr);
    }

    LOG_END
}

MT_TEST(Integration, TestCgroupWriteAndReset3, "cgroup") {
    LOG_START

    const std::string testResourceName = "/sys/fs/cgroup/audio-cgroup/cpu.uclamp.max";

    std::string originalValueString = AuxRoutines::readFromFile(testResourceName);
    std::cout << "[" << __LINE__ << "] cpu.uclamp.max Original Value: " << originalValueString << std::endl;
    int32_t originalValue = C_STOI(originalValueString);

    int32_t rc = fork();
    if (rc == 0) {
        // child: no MT_* macros here
        SysResource* resourceList = new SysResource[1];
        std::memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = 0x00090008;
        resourceList[0].mNumValues = 2;
        resourceList[0].mResValue.values = new int32_t[2];
        resourceList[0].mResValue.values[0] = 802;
        resourceList[0].mResValue.values[1] = 75;

        int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        delete[] resourceList[0].mResValue.values;
        delete[] resourceList;
        std::exit(EXIT_SUCCESS);

    } else if (rc > 0) {
        SysResource* resourceList = new SysResource[1];
        std::memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = 0x00090008;
        resourceList[0].mNumValues = 2;
        resourceList[0].mResValue.values = new int32_t[2];
        resourceList[0].mResValue.values[0] = 802;
        resourceList[0].mResValue.values[1] = 68;

        int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
        MT_REQUIRE(ctx, handle > 0);

        std::string value;
        int32_t newValue;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        MT_REQUIRE_EQ(ctx, newValue, 75);

        std::this_thread::sleep_for(std::chrono::seconds(10));
        value = AuxRoutines::readFromFile(testResourceName);
        std::cout << "[" << __LINE__ << "] cpu.uclamp.max Reset Value: " << value << std::endl;
        newValue = C_STOI(value);
        if (newValue != -1 && originalValue != -1) {
            MT_REQUIRE_EQ(ctx, newValue, originalValue);
        }

        delete[] resourceList[0].mResValue.values;
        delete[] resourceList;
        wait(nullptr);
    }

    LOG_END
}

MT_TEST(Integration, TestCgroupWriteAndReset4, "cgroup") {
    LOG_START

    const std::string testResourceName = "/sys/fs/cgroup/audio-cgroup/memory.max";

    std::string originalValueString = AuxRoutines::readFromFile(testResourceName);
    int32_t originalValue = C_STOI(originalValueString);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;

    int32_t rc = fork();
    if (rc == 0) {
        // child
        SysResource* resourceList = new SysResource[1];
        std::memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = 0x0009000b;
        resourceList[0].mNumValues = 2;
        resourceList[0].mResValue.values = new int32_t[2];
        resourceList[0].mResValue.values[0] = 802;
        resourceList[0].mResValue.values[1] = 1224;

        int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        delete[] resourceList[0].mResValue.values;
        delete[] resourceList;
        std::exit(EXIT_SUCCESS);

    } else if (rc > 0) {
        SysResource* resourceList = new SysResource[1];
        std::memset(&resourceList[0], 0, sizeof(SysResource));
        resourceList[0].mResCode = 0x0009000b;
        resourceList[0].mNumValues = 2;
        resourceList[0].mResValue.values = new int32_t[2];
        resourceList[0].mResValue.values[0] = 802;
        resourceList[0].mResValue.values[1] = 950; // KB as per configs

        int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
        std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
        MT_REQUIRE(ctx, handle > 0);

        std::string value;
        int32_t newValue;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
        MT_REQUIRE(ctx, newValue >  950 * 1024);
        MT_REQUIRE(ctx, newValue <= 1224 * 1024);

        std::this_thread::sleep_for(std::chrono::seconds(10));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        if (newValue != -1 && originalValue != -1) {
            std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
            MT_REQUIRE_EQ(ctx, newValue, originalValue);
        }

        delete[] resourceList[0].mResValue.values;
        delete[] resourceList;
        wait(nullptr);
    }

    LOG_END
}

MT_TEST(Integration, TestCgroupWriteAndReset5, "cgroup") {
    LOG_START

    const std::string testResourceName1 = "/sys/fs/cgroup/camera-cgroup/cpu.uclamp.min";
    const std::string testResourceName2 = "/sys/fs/cgroup/audio-cgroup/cpu.uclamp.min";

    std::string originalValueString = AuxRoutines::readFromFile(testResourceName1);
    int32_t originalValue1 = C_STOI(originalValueString);
    std::cout << LOG_BASE << testResourceName1 << " Original Value: " << originalValue1 << std::endl;

    originalValueString = AuxRoutines::readFromFile(testResourceName2);
    std::cout << "[" << __LINE__ << "] 1) cpu.uclamp.min Original Value: " << originalValueString << std::endl;
    int32_t originalValue2 = C_STOI(originalValueString);
    std::cout << LOG_BASE << testResourceName2 << " Original Value: " << originalValue2 << std::endl;

    SysResource* resourceList = new SysResource[2];

    std::memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x00090007;
    resourceList[0].mNumValues = 2;
    resourceList[0].mResValue.values = new int32_t[2];
    resourceList[0].mResValue.values[0] = 801; // camera
    resourceList[0].mResValue.values[1] = 55;

    std::memset(&resourceList[1], 0, sizeof(SysResource));
    resourceList[1].mResCode = 0x00090007;
    resourceList[1].mNumValues = 2;
    resourceList[1].mResValue.values = new int32_t[2];
    resourceList[1].mResValue.values[0] = 802; // audio
    resourceList[1].mResValue.values[1] = 58;

    int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_LOW, 2, resourceList);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE(ctx, handle > 0);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::string value = AuxRoutines::readFromFile(testResourceName1);
    int32_t newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName1 << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 55);

    value = AuxRoutines::readFromFile(testResourceName2);
    newValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName2 << " Configured Value: " << newValue << std::endl;
    MT_REQUIRE_EQ(ctx, newValue, 58);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    value = AuxRoutines::readFromFile(testResourceName1);
    newValue = C_STOI(value);
    if (newValue != -1 && originalValue1 != -1) {
        std::cout << LOG_BASE << testResourceName1 << " Reset Value: " << newValue << std::endl;
        MT_REQUIRE_EQ(ctx, newValue, originalValue1);
    }

    value = AuxRoutines::readFromFile(testResourceName2);
    newValue = C_STOI(value);
    if (newValue != -1 && originalValue2 != -1) {
        std::cout << LOG_BASE << testResourceName2 << " Reset Value: " << newValue << std::endl;
        MT_REQUIRE_EQ(ctx, newValue, originalValue2);
    }

    // cleanup
    delete[] resourceList[0].mResValue.values;
    delete[] resourceList[1].mResValue.values;
    delete[] resourceList;

    LOG_END
}

MT_TEST(Integration, TestCgroupWriteAndReset6, "cgroup") {
    LOG_START

    const std::string cpusPath = "/sys/fs/cgroup/audio-cgroup/cpuset.cpus";

    // Read original value (as string)
    std::string originalValueString = AuxRoutines::readFromFile(cpusPath);
    // Trim originalValueString (no lambda, inline)
    {
        std::string &s = originalValueString;
        if (!s.empty()) {
            size_t first = s.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                s.clear();
            } else {
                size_t last = s.find_last_not_of(" \t\r\n");
                s = s.substr(first, last - first + 1);
            }
        }
    }

    // Prepare resource list
    SysResource* resourceList1 = new SysResource[1];
    std::memset(&resourceList1[0], 0, sizeof(SysResource));
    resourceList1[0].mResCode = 0x00090002;
    resourceList1[0].mNumValues = 4;
    resourceList1[0].mResValue.values = new int32_t[resourceList1[0].mNumValues];
    resourceList1[0].mResValue.values[0] = 802; // audio-cgroup ID (as per your existing logic)
    resourceList1[0].mResValue.values[1] = 0;
    resourceList1[0].mResValue.values[2] = 1;
    resourceList1[0].mResValue.values[3] = 3;

    int64_t handle = tuneResources(8000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList1);
    std::cout << LOG_BASE << "Handle Returned: " << handle << std::endl;
    MT_REQUIRE(ctx, handle > 0);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Read configured value
    std::string value = AuxRoutines::readFromFile(cpusPath);
    // Trim value (no lambda, inline)
    {
        std::string &s = value;
        if (!s.empty()) {
            size_t first = s.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                s.clear();
            } else {
                size_t last = s.find_last_not_of(" \t\r\n");
                s = s.substr(first, last - first + 1);
            }
        }
    }

    std::cout << LOG_BASE << cpusPath << " Configured Value: " << value << std::endl;
    MT_REQUIRE_EQ(ctx, value, std::string("0-1,3"));

    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Read reset value
    value = AuxRoutines::readFromFile(cpusPath);
    // Trim again (no lambda, inline)
    {
        std::string &s = value;
        if (!s.empty()) {
            size_t first = s.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                s.clear();
            } else {
                size_t last = s.find_last_not_of(" \t\r\n");
                s = s.substr(first, last - first + 1);
            }
        }
    }

    std::cout << LOG_BASE << cpusPath << " Reset Value: " << value << std::endl;
    MT_REQUIRE_EQ(ctx, value, originalValueString);

    // Cleanup
    delete[] resourceList1[0].mResValue.values;
    delete[] resourceList1;

    LOG_END
}
