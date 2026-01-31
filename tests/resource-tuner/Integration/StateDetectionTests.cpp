// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ErrCodes.h"
#include "UrmPlatformAL.h"
#include "Utils.h"
#include "TestUtils.h"
#include "TestBaseline.h"
#include "UrmAPIs.h"

static void simulateSuspension() {
    system("dbus-send --system --type=signal                                        \
            /org/freedesktop/login1 org.freedesktop.login1.Manager.PrepareForSleep  \
            boolean:true");
}

static void simulateResumption() {
    system("dbus-send --system --type=signal                                        \
            /org/freedesktop/login1 org.freedesktop.login1.Manager.PrepareForSleep  \
            boolean:false");
}

static void TestNormalRequestFlow() {
    LOG_START

    std::string testResourceName = "/proc/sys/kernel/sched_util_clamp_min";

    // Check the original value for the Resource
    std::string value = AuxRoutines::readFromFile(testResourceName);
    int32_t originalValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName<<" Original Value: "<<originalValue<<std::endl;

    if(originalValue == -1) {
        // Node does not exist on test device, can't proceed with this test
        std::cout<<LOG_BASE<<testResourceName<<"Node: "<<testResourceName<<" not found on test device, Aborting Test Case"<<std::endl;
        return;
    }

    SysResource* resourceList = new SysResource[1];
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x00030000;
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 980;

    int64_t handle = tuneResources(5000, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
    std::cout<<LOG_BASE<<"Handle Returned: "<<handle<<std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Check if the new value was successfully written to the node
    value = AuxRoutines::readFromFile(testResourceName);
    int32_t newValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName<<" Configured Value: "<<newValue<<std::endl;
    assert(newValue == 980);

    std::this_thread::sleep_for(std::chrono::seconds(6));

    // Wait for the Request to expire, check if the value resets
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName<<" Reset Value: "<<newValue<<std::endl;
    assert(newValue == originalValue);

    delete resourceList;
    LOG_END
}

static void TestRequestSuspension() {
    LOG_START

    // Submit 2 requests, enable background processing for one and not for another
    std::string testResourceName1 = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    int32_t testResourceOriginalValue1 = 107;

    std::string testResourceName2 = "/etc/urm/tests/nodes/sched_util_clamp_min.txt";
    int32_t testResourceOriginalValue2 = 300;

    std::string value;
    int32_t originalValue, newValue;
    int64_t handle;

    value = AuxRoutines::readFromFile(testResourceName1);
    originalValue = C_STOI(value);
    assert(originalValue == testResourceOriginalValue1);

    value = AuxRoutines::readFromFile(testResourceName2);
    originalValue = C_STOI(value);
    assert(originalValue == testResourceOriginalValue2);

    // First Request
    SysResource* resourceList1 = new SysResource[1];
    memset(&resourceList1[0], 0, sizeof(SysResource));
    resourceList1[0].mResCode = 0x80ff0002;
    resourceList1[0].mNumValues = 1;
    resourceList1[0].mResValue.value = 821;

    int32_t properties1 = 0;
    properties1 = SET_REQUEST_PRIORITY(properties1, REQ_PRIORITY_HIGH);

    handle = tuneResources(50000, properties1, 1, resourceList1);
    std::cout<<LOG_BASE<<"Handle Returned: "<<handle<<std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Check if the new value was successfully written to the node
    value = AuxRoutines::readFromFile(testResourceName1);
    newValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName1<<" Configured Value: "<<newValue<<std::endl;
    assert(newValue == 821);

    // Second Request
    SysResource* resourceList2 = new SysResource[1];
    memset(&resourceList2[0], 0, sizeof(SysResource));
    resourceList2[0].mResCode = 0x80ff0000;
    resourceList2[0].mNumValues = 1;
    resourceList2[0].mResValue.value = 885;

    int32_t properties2 = 0;
    properties2 = SET_REQUEST_PRIORITY(properties2, REQ_PRIORITY_HIGH);
    properties2 = ADD_ALLOWED_MODE(properties2, MODE_RESUME);
    properties2 = ADD_ALLOWED_MODE(properties2, MODE_SUSPEND);

    handle = tuneResources(10000, properties2, 1, resourceList2);
    std::cout<<LOG_BASE<<"Handle Returned: "<<handle<<std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Check if the new value was successfully written to the node
    value = AuxRoutines::readFromFile(testResourceName2);
    newValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName2<<" Configured Value: "<<newValue<<std::endl;
    assert(newValue == 885);

    // Issue system suspend
    std::cout<<LOG_BASE<<"Simulating system suspension via dbus-send"<<std::endl;
    simulateSuspension();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Since the first request is not enabled for background processing, it should be untuned
    value = AuxRoutines::readFromFile(testResourceName1);
    newValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName1<<" Reset Value: "<<newValue<<std::endl;
    assert(newValue == testResourceOriginalValue1);

    // Since the second request is enabled for background processing it should not be untuned
    value = AuxRoutines::readFromFile(testResourceName2);
    newValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName2<<" Configured Value: "<<newValue<<std::endl;
    assert(newValue == 885);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    value = AuxRoutines::readFromFile(testResourceName2);
    newValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName2<<" Reset Value: "<<newValue<<std::endl;
    assert(newValue == testResourceOriginalValue2);

    delete resourceList1;
    delete resourceList2;
    LOG_END
}

// This test assumes the server is in suspended mode, which it will be if the previous
// test ran correctly.
static void TestRequestRejectionInSuspendMode() {
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/scaling_max_freq.txt";
    int32_t testResourceOriginalValue = 114;

    std::string value;
    int32_t originalValue, newValue;

    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    assert(originalValue == testResourceOriginalValue);

    SysResource* resourceList = new SysResource[1];
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x80ff0003;
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 764;

    int64_t handle = tuneResources(-1, RequestPriority::REQ_PRIORITY_HIGH, 1, resourceList);
    std::cout<<LOG_BASE<<"Handle Returned: "<<handle<<std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    assert(newValue == testResourceOriginalValue);

    delete resourceList;
    LOG_END
}

static void TestRequestResumptionPostResume() {
    LOG_START

    std::string testResourceName = "/etc/urm/tests/nodes/scaling_min_freq.txt";
    int32_t testResourceOriginalValue = 107;

    std::string value;
    int32_t newValue;

    std::cout<<LOG_BASE<<"Simulating system resume via dbus-send"<<std::endl;
    simulateResumption();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName<<" Configured Value: "<<newValue<<std::endl;
    assert(newValue == 821);

    std::this_thread::sleep_for(std::chrono::seconds(60));

    // Wait for the Request to expire, check if the value resets
    value = AuxRoutines::readFromFile(testResourceName);
    newValue = C_STOI(value);
    std::cout<<LOG_BASE<<testResourceName<<" Reset Value: "<<newValue<<std::endl;
    assert(newValue == testResourceOriginalValue);

    LOG_END
}

int32_t main(int32_t argc, const char* argv[]) {
    RUN_INTEGRATION_TEST(TestNormalRequestFlow);
    RUN_INTEGRATION_TEST(TestRequestSuspension);
    RUN_INTEGRATION_TEST(TestRequestRejectionInSuspendMode);
    RUN_INTEGRATION_TEST(TestRequestResumptionPostResume);

    return 0;
}
