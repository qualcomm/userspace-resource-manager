// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "UrmPlatformAL.h"
#include "TestUtils.h"
#include "MemoryPool.h"
#include "Request.h"
#include "Signal.h"
#include "URMTests.h"

#define TEST_CLASS "COMPONENT"

// Request Cleanup Tests
URM_TEST(TestResourceStructCoreClusterSettingAndExtraction, {
    Resource resource;

    resource.setCoreValue(2);
    resource.setClusterValue(1);

    E_ASSERT((resource.getCoreValue() == 2));
    E_ASSERT((resource.getClusterValue() == 1));
})

URM_TEST(TestResourceStructOps1, {
    int32_t properties = -1;
    properties = SET_REQUEST_PRIORITY(properties, REQ_PRIORITY_HIGH);
    E_ASSERT((properties == -1));
})

URM_TEST(TestResourceStructOps2, {
    int32_t properties = 0;
    properties = SET_REQUEST_PRIORITY(properties, 44);
    E_ASSERT((properties == -1));

    properties = 0;
    properties = SET_REQUEST_PRIORITY(properties, -3);
    E_ASSERT((properties == -1));
})

URM_TEST(TestResourceStructOps3, {
    int32_t properties = 0;
    properties = SET_REQUEST_PRIORITY(properties, REQ_PRIORITY_HIGH);
    int8_t priority = EXTRACT_REQUEST_PRIORITY(properties);
    E_ASSERT((priority == REQ_PRIORITY_HIGH));

    properties = 0;
    properties = SET_REQUEST_PRIORITY(properties, REQ_PRIORITY_LOW);
    priority = EXTRACT_REQUEST_PRIORITY(properties);
    E_ASSERT((priority == REQ_PRIORITY_LOW));
})

URM_TEST(TestResourceStructOps4, {
    int32_t properties = 0;
    properties = ADD_ALLOWED_MODE(properties, MODE_RESUME);
    int8_t allowedModes = EXTRACT_ALLOWED_MODES(properties);
    E_ASSERT((allowedModes == MODE_RESUME));

    properties = 0;
    properties = ADD_ALLOWED_MODE(properties, MODE_RESUME);
    properties = ADD_ALLOWED_MODE(properties, MODE_DOZE);
    allowedModes = EXTRACT_ALLOWED_MODES(properties);
    E_ASSERT((allowedModes == (MODE_RESUME | MODE_DOZE)));
})

URM_TEST(TestResourceStructOps5, {
    int32_t properties = 0;
    properties = ADD_ALLOWED_MODE(properties, 87);
    E_ASSERT((properties == -1));
})

URM_TEST(TestResourceStructOps6, {
    int32_t properties = 0;
    properties = ADD_ALLOWED_MODE(properties, MODE_RESUME);
    properties = ADD_ALLOWED_MODE(properties, MODE_SUSPEND);
    int8_t allowedModes = EXTRACT_ALLOWED_MODES(properties);
    E_ASSERT((allowedModes == (MODE_RESUME | MODE_SUSPEND)));
})

URM_TEST(TestResourceStructOps8, {
    int32_t properties = 0;
    properties = SET_REQUEST_PRIORITY(properties, REQ_PRIORITY_LOW);
    properties = ADD_ALLOWED_MODE(properties, MODE_RESUME);
    properties = ADD_ALLOWED_MODE(properties, MODE_SUSPEND);

    int8_t priority = EXTRACT_REQUEST_PRIORITY(properties);
    int8_t allowedModes = EXTRACT_ALLOWED_MODES(properties);

    E_ASSERT((priority == REQ_PRIORITY_LOW));
    E_ASSERT((allowedModes == (MODE_RESUME | MODE_SUSPEND)));
})

URM_TEST(TestResourceStructOps9, {
    int32_t resInfo = 0;
    resInfo = SET_RESOURCE_MPAM_VALUE(resInfo, 30);
    int8_t mpamValue = EXTRACT_RESOURCE_MPAM_VALUE(resInfo);
    E_ASSERT((mpamValue == 30));
})

URM_TEST(TestHandleGeneration, {
    for(int32_t i = 1; i <= 2e7; i++) {
        int64_t handle = AuxRoutines::generateUniqueHandle();
        E_ASSERT((handle == i));
    }
})

URM_TEST(TestAuxRoutineFileExists, {
    int8_t fileExists = AuxRoutines::fileExists("AuxParserTest.yaml");
    E_ASSERT((fileExists == false));

    fileExists = AuxRoutines::fileExists("/etc/urm/tests/configs/NetworkConfig.yaml");
    E_ASSERT((fileExists == false));

    fileExists = AuxRoutines::fileExists(UrmSettings::mCommonResourcesPath);
    E_ASSERT((fileExists == true));

    fileExists = AuxRoutines::fileExists(UrmSettings::mCommonPropertiesPath);
    E_ASSERT((fileExists == true));

    fileExists = AuxRoutines::fileExists("");
    E_ASSERT((fileExists == false));
})

URM_TEST(TestRequestModeAddition, {
    Request request;
    request.setProperties(0);
    request.addProcessingMode(MODE_RESUME);
    E_ASSERT((request.getProcessingModes() == MODE_RESUME));

    request.setProperties(0);
    request.addProcessingMode(MODE_RESUME);
    request.addProcessingMode(MODE_SUSPEND);
    request.addProcessingMode(MODE_DOZE);
    E_ASSERT((request.getProcessingModes() == (MODE_RESUME | MODE_SUSPEND | MODE_DOZE)));
})
