// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


// ParserTests_mtest.cpp

#define MTEST_NO_MAIN
#include "../framework/mini.hpp"

// ParserTests.cpp (ported to mini.hpp with assert adaptation + no lambdas in concurrent test)
#include "ErrCodes.h"
#include "TestUtils.h"
#include "RestuneParser.h"
#include "ResourceRegistry.h"
#include "SignalRegistry.h"
#include "Extensions.h"
#include "Utils.h"
#include "RestuneInternal.h"
#include "PropertiesRegistry.h"
#include "TargetRegistry.h"
#include "AppConfigs.h"
#include "TestAggregator.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <thread>
#include <cstring>
#include <iostream>


using namespace mtest;

// ----------------------------------------------------------------------------
// ResourceParsingTests
// ----------------------------------------------------------------------------
namespace ResourceParsingTests {
    std::string __testGroupName = "ResourceParsingTests";

    static ErrCode parsingStatus = RC_SUCCESS;

    static void Init() {
        RestuneParser configProcessor;
        // Use single-argument overload to avoid signature mismatch
        parsingStatus = configProcessor.parseResourceConfigs("/etc/urm/tests/configs/ResourcesConfig.yaml");
    }

    static void EnsureInit() {
        static bool done = false;
        if (!done) { Init(); done = true; }
    }

    MT_TEST(ResourceParsingTests, ResourceRestuneParserYAMLDataIntegrity1, "component-serial") {
        EnsureInit();
        MT_REQUIRE(ctx, ResourceRegistry::getInstance() != nullptr);
        MT_REQUIRE_EQ(ctx, parsingStatus, RC_SUCCESS);
    }

    MT_TEST(ResourceParsingTests, ResourceRestuneParserYAMLDataIntegrity3_1, "component-serial") {
        EnsureInit();
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x80ff0000);

        MT_REQUIRE(ctx, resourceConfigInfo != nullptr);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResType, 0xff);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResID, 0);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourceName.data(), "TEST_RESOURCE_1"), 0);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/etc/urm/tests/nodes/sched_util_clamp_min.txt"), 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mHighThreshold, 1024);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mLowThreshold, 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPolicy, HIGHER_BETTER);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPermissions, PERMISSION_THIRD_PARTY);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mModes, (MODE_RESUME | MODE_DOZE));
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mApplyType, ResourceApplyType::APPLY_GLOBAL);
    }

    MT_TEST(ResourceParsingTests, ResourceRestuneParserYAMLDataIntegrity3_2, "component-serial") {
        EnsureInit();
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x80ff0001);

        MT_REQUIRE(ctx, resourceConfigInfo != nullptr);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResType, 0xff);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResID, 1);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourceName.data(), "TEST_RESOURCE_2"), 0);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/etc/urm/tests/nodes/sched_util_clamp_max.txt"), 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mHighThreshold, 1024);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mLowThreshold, 512);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPolicy, HIGHER_BETTER);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPermissions, PERMISSION_THIRD_PARTY);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mModes, (MODE_RESUME | MODE_DOZE));
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mApplyType, ResourceApplyType::APPLY_GLOBAL);
    }

    MT_TEST(ResourceParsingTests, ResourceRestuneParserYAMLDataIntegrity3_3, "component-serial") {
        EnsureInit();
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x80ff0005);

        MT_REQUIRE(ctx, resourceConfigInfo != nullptr);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResType, 0xff);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResID, 5);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourceName.data(), "TEST_RESOURCE_6"), 0);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/etc/urm/tests/nodes/target_test_resource2.txt"), 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mHighThreshold, 6500);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mLowThreshold, 50);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPolicy, HIGHER_BETTER);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPermissions, PERMISSION_THIRD_PARTY);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mModes, MODE_RESUME);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mApplyType, ResourceApplyType::APPLY_CORE);
    }
} // namespace ResourceParsingTests

// ----------------------------------------------------------------------------
// SignalParsingTests
// ----------------------------------------------------------------------------
namespace SignalParsingTests {
    std::string __testGroupName = "SignalParsingTests";

    static ErrCode parsingStatus = RC_SUCCESS;

    static void Init() {
        RestuneParser configProcessor;
        parsingStatus = configProcessor.parseSignalConfigs("/etc/urm/tests/configs/SignalsConfig.yaml");
    }

    static void EnsureInit() {
        static bool done = false;
        if (!done) { Init(); done = true; }
    }

    MT_TEST(SignalParsingTests, RestuneParserYAMLDataIntegrity1, "component-serial") {
        EnsureInit();
        MT_REQUIRE(ctx, SignalRegistry::getInstance() != nullptr);
        MT_REQUIRE_EQ(ctx, parsingStatus, RC_SUCCESS);
    }

    MT_TEST(SignalParsingTests, RestuneParserYAMLDataIntegrity3_1, "component-serial") {
        EnsureInit();
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigById(0x0000000d);

        MT_REQUIRE(ctx, signalInfo != nullptr);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalID, 0);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalCategory, 0x0d);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_1"), 0);
        MT_REQUIRE_EQ(ctx, signalInfo->mTimeout, 4000);

        MT_REQUIRE(ctx, signalInfo->mPermissions != nullptr);
        MT_REQUIRE(ctx, signalInfo->mDerivatives != nullptr);
        MT_REQUIRE(ctx, signalInfo->mSignalResources != nullptr);

        MT_REQUIRE_EQ(ctx, (int)signalInfo->mPermissions->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mDerivatives->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mSignalResources->size(), 1);

        MT_REQUIRE_EQ(ctx, signalInfo->mPermissions->at(0), PERMISSION_THIRD_PARTY);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mDerivatives->at(0).data(), "solar"), 0);

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        MT_REQUIRE(ctx, resource1 != nullptr);
        MT_REQUIRE_EQ(ctx, resource1->getResCode(), 2147549184);
        MT_REQUIRE_EQ(ctx, resource1->getValuesCount(), 1);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(0), 700);
        MT_REQUIRE_EQ(ctx, resource1->getResInfo(), 0);
    }

    MT_TEST(SignalParsingTests, RestuneParserYAMLDataIntegrity3_2, "component-serial") {
        EnsureInit();
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigById(0x0000010d);

        MT_REQUIRE(ctx, signalInfo != nullptr);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalID, 1);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalCategory, 0x0d);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_2"), 0);
        MT_REQUIRE_EQ(ctx, signalInfo->mTimeout, 5000);

        MT_REQUIRE(ctx, signalInfo->mPermissions != nullptr);
        MT_REQUIRE(ctx, signalInfo->mDerivatives != nullptr);
        MT_REQUIRE(ctx, signalInfo->mSignalResources != nullptr);

        MT_REQUIRE_EQ(ctx, (int)signalInfo->mPermissions->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mDerivatives->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mSignalResources->size(), 2);

        MT_REQUIRE_EQ(ctx, signalInfo->mPermissions->at(0), PERMISSION_SYSTEM);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mDerivatives->at(0).data(), "derivative_v2"), 0);

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        MT_REQUIRE_EQ(ctx, resource1->getResCode(), 8);
        MT_REQUIRE_EQ(ctx, resource1->getValuesCount(), 1);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(0), 814);
        MT_REQUIRE_EQ(ctx, resource1->getResInfo(), 0);

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        MT_REQUIRE_EQ(ctx, resource2->getResCode(), 15);
        MT_REQUIRE_EQ(ctx, resource2->getValuesCount(), 2);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(0), 23);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(1), 90);
        MT_REQUIRE_EQ(ctx, resource2->getResInfo(), 256);
    }

    MT_TEST(SignalParsingTests, RestuneParserYAMLDataIntegrity3_3, "component-serial") {
        EnsureInit();
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigById(0x0000030d);
        MT_REQUIRE_EQ(ctx, signalInfo, (SignalInfo*)nullptr);
    }

    MT_TEST(SignalParsingTests, RestuneParserYAMLDataIntegrity3_4, "component-serial") {
        EnsureInit();
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigById(0x0000070d);

        MT_REQUIRE(ctx, signalInfo != nullptr);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalID, 0x0007);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalCategory, 0x0d);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_8"), 0);
        MT_REQUIRE_EQ(ctx, signalInfo->mTimeout, 5500);

        MT_REQUIRE(ctx, signalInfo->mPermissions != nullptr);
        MT_REQUIRE_EQ(ctx, signalInfo->mDerivatives, (decltype(signalInfo->mDerivatives))nullptr);
        MT_REQUIRE(ctx, signalInfo->mSignalResources != nullptr);

        MT_REQUIRE_EQ(ctx, (int)signalInfo->mPermissions->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mSignalResources->size(), 2);

        MT_REQUIRE_EQ(ctx, signalInfo->mPermissions->at(0), PERMISSION_THIRD_PARTY);

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        MT_REQUIRE_EQ(ctx, resource1->getResCode(), 0x000900aa);
        MT_REQUIRE_EQ(ctx, resource1->getValuesCount(), 3);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(0), -1);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(1), -1);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(2), 68);
        MT_REQUIRE_EQ(ctx, resource1->getResInfo(), 0);

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        MT_REQUIRE_EQ(ctx, resource2->getResCode(), 0x000900dc);
        MT_REQUIRE_EQ(ctx, resource2->getValuesCount(), 4);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(0), -1);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(1), -1);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(2), 50);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(3), 512);
        MT_REQUIRE_EQ(ctx, resource2->getResInfo(), 0);
    }
} // namespace SignalParsingTests

// ----------------------------------------------------------------------------
// InitConfigParsingTests
// ----------------------------------------------------------------------------
namespace InitConfigParsingTests {
    std::string __testGroupName = "InitConfigParsingTests";

    static ErrCode parsingStatus = RC_SUCCESS;

    static void Init() {
        RestuneParser configProcessor;
        parsingStatus = configProcessor.parseInitConfigs("/etc/urm/tests/configs/InitConfig.yaml");
    }

    static void EnsureInit() {
        static bool done = false;
        if (!done) { Init(); done = true; }
    }

    MT_TEST(InitConfigParsingTests, InitRestuneParserYAMLDataIntegrity1, "component-serial") {
        EnsureInit();
        MT_REQUIRE(ctx, (TargetRegistry::getInstance() != nullptr));
        MT_REQUIRE_EQ(ctx, parsingStatus, RC_SUCCESS);
    }

    MT_TEST(InitConfigParsingTests, InitRestuneParserYAMLDataIntegrity2, "component-serial") {
        EnsureInit();
        std::cout << "Count of Cgroups created: " << TargetRegistry::getInstance()->getCreatedCGroupsCount() << std::endl;
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getCreatedCGroupsCount(), 3);
    }

    MT_TEST(InitConfigParsingTests, InitRestuneParserYAMLDataIntegrity3, "component-serial") {
        EnsureInit();
        std::vector<std::string> cGroupNames;
        TargetRegistry::getInstance()->getCGroupNames(cGroupNames);
        std::vector<std::string> expectedNames = {"camera-cgroup", "audio-cgroup", "video-cgroup"};

        MT_REQUIRE_EQ(ctx, (int)cGroupNames.size(), 3);

        std::unordered_set<std::string> expectedNamesSet;
        for (int32_t i = 0; i < (int)cGroupNames.size(); i++) {
            expectedNamesSet.insert(cGroupNames[i]);
        }

        for (int32_t i = 0; i < (int)expectedNames.size(); i++) {
            MT_REQUIRE(ctx, expectedNamesSet.find(expectedNames[i]) != expectedNamesSet.end());
        }
    }

    MT_TEST(InitConfigParsingTests, InitRestuneParserYAMLDataIntegrity4, "component-serial") {
        EnsureInit();
        CGroupConfigInfo* cameraConfig = TargetRegistry::getInstance()->getCGroupConfig(801);
        MT_REQUIRE(ctx, cameraConfig != nullptr);
        MT_REQUIRE_EQ(ctx, cameraConfig->mCgroupName, std::string("camera-cgroup"));
        MT_REQUIRE_EQ(ctx, cameraConfig->mIsThreaded, false);

        CGroupConfigInfo* videoConfig = TargetRegistry::getInstance()->getCGroupConfig(803);
        MT_REQUIRE(ctx, videoConfig != nullptr);
        MT_REQUIRE_EQ(ctx, videoConfig->mCgroupName, std::string("video-cgroup"));
        MT_REQUIRE_EQ(ctx, videoConfig->mIsThreaded, true);
    }

    MT_TEST(InitConfigParsingTests, InitRestuneParserYAMLDataIntegrity5, "component-serial") {
        EnsureInit();
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getCreatedMpamGroupsCount(), 3);
    }

    MT_TEST(InitConfigParsingTests, InitRestuneParserYAMLDataIntegrity6, "component-serial") {
        EnsureInit();
        std::vector<std::string> mpamGroupNames;
        TargetRegistry::getInstance()->getMpamGroupNames(mpamGroupNames);
        std::vector<std::string> expectedNames = {"camera-mpam-group", "audio-mpam-group", "video-mpam-group"};

        MT_REQUIRE_EQ(ctx, (int)mpamGroupNames.size(), 3);

        std::unordered_set<std::string> expectedNamesSet;
        for (int32_t i = 0; i < (int)mpamGroupNames.size(); i++) {
            expectedNamesSet.insert(mpamGroupNames[i]);
        }

        for (int32_t i = 0; i < (int)expectedNames.size(); i++) {
            MT_REQUIRE(ctx, expectedNamesSet.find(expectedNames[i]) != expectedNamesSet.end());
        }
    }

    MT_TEST(InitConfigParsingTests, InitRestuneParserYAMLDataIntegrity7, "component-serial") {
        EnsureInit();
        MpamGroupConfigInfo* cameraConfig = TargetRegistry::getInstance()->getMpamGroupConfig(0);
        MT_REQUIRE(ctx, cameraConfig != nullptr);
        MT_REQUIRE_EQ(ctx, cameraConfig->mMpamGroupName, std::string("camera-mpam-group"));
        MT_REQUIRE_EQ(ctx, cameraConfig->mMpamGroupInfoID, 0);
        MT_REQUIRE_EQ(ctx, cameraConfig->mPriority, 0);

        MpamGroupConfigInfo* videoConfig = TargetRegistry::getInstance()->getMpamGroupConfig(2);
        MT_REQUIRE(ctx, videoConfig != nullptr);
        MT_REQUIRE_EQ(ctx, videoConfig->mMpamGroupName, std::string("video-mpam-group"));
        MT_REQUIRE_EQ(ctx, videoConfig->mMpamGroupInfoID, 2);
        MT_REQUIRE_EQ(ctx, videoConfig->mPriority, 2);
    }
} // namespace InitConfigParsingTests

// ----------------------------------------------------------------------------
// PropertyParsingTests  (concurrent test converted to NO LAMBDAS)
// ----------------------------------------------------------------------------
namespace PropertyParsingTests {
    std::string __testGroupName = "PropertyParsingTests";

    static ErrCode parsingStatus = RC_SUCCESS;

    static void Init() {
        RestuneParser configProcessor;
        parsingStatus = configProcessor.parsePropertiesConfigs("/etc/urm/tests/configs/PropertiesConfig.yaml");
    }

    static void EnsureInit() {
        static bool done = false;
        if (!done) { Init(); done = true; }
    }

    MT_TEST(PropertyParsingTests, SysRestuneParserYAMLDataIntegrity1, "component-serial") {
        EnsureInit();
        MT_REQUIRE(ctx, PropertiesRegistry::getInstance() != nullptr);
        MT_REQUIRE_EQ(ctx, parsingStatus, RC_SUCCESS);
    }

    MT_TEST(PropertyParsingTests, SysConfigGetPropSimpleRetrieval1, "component-serial") {
        EnsureInit();
        std::string resultBuffer;
        int8_t propFound = submitPropGetRequest("test.debug.enabled", resultBuffer, "false");

        MT_REQUIRE_EQ(ctx, propFound, true);
        MT_REQUIRE_EQ(ctx, std::strcmp(resultBuffer.c_str(), "true"), 0);
    }

    MT_TEST(PropertyParsingTests, SysConfigGetPropSimpleRetrieval2, "component-serial") {
        EnsureInit();
        std::string resultBuffer;
        int8_t propFound = submitPropGetRequest("test.current.worker_thread.count", resultBuffer, "false");

        MT_REQUIRE_EQ(ctx, propFound, true);
        MT_REQUIRE_EQ(ctx, std::strcmp(resultBuffer.c_str(), "125"), 0);
    }

    MT_TEST(PropertyParsingTests, SysConfigGetPropSimpleRetrievalInvalidProperty, "component-serial") {
        EnsureInit();
        std::string resultBuffer;
        int8_t propFound = submitPropGetRequest("test.historic.worker_thread.count", resultBuffer, "5");

        MT_REQUIRE_EQ(ctx, propFound, false);
        MT_REQUIRE_EQ(ctx, std::strcmp(resultBuffer.c_str(), "5"), 0);
    }

    // --- Concurrent retrieval (NO lambdas): free functions receive TestContext* ---
    static void PropGet_CurrentWorkers(mtest::TestContext* tctx) {
        std::string resultBuffer;
        int8_t propFound = submitPropGetRequest("test.current.worker_thread.count", resultBuffer, "false");
        MT_REQUIRE_EQ((*tctx), propFound, true);
        MT_REQUIRE_EQ((*tctx), std::strcmp(resultBuffer.c_str(), "125"), 0);
    }
    static void PropGet_DebugEnabled(mtest::TestContext* tctx) {
        std::string resultBuffer;
        int8_t propFound = submitPropGetRequest("test.debug.enabled", resultBuffer, "false");
        MT_REQUIRE_EQ((*tctx), propFound, true);
        MT_REQUIRE_EQ((*tctx), std::strcmp(resultBuffer.c_str(), "true"), 0);
    }
    static void PropGet_DocBuildMode(mtest::TestContext* tctx) {
        std::string resultBuffer;
        int8_t propFound = submitPropGetRequest("test.doc.build.mode.enabled", resultBuffer, "false");
        MT_REQUIRE_EQ((*tctx), propFound, true);
        MT_REQUIRE_EQ((*tctx), std::strcmp(resultBuffer.c_str(), "false"), 0);
    }

    MT_TEST(PropertyParsingTests, SysConfigGetPropConcurrentRetrieval, "component-serial") {
        EnsureInit();
        std::thread th1(PropGet_CurrentWorkers, &ctx);
        std::thread th2(PropGet_DebugEnabled, &ctx);
        std::thread th3(PropGet_DocBuildMode, &ctx);
        th1.join(); th2.join(); th3.join();
    }
} // namespace PropertyParsingTests

// ----------------------------------------------------------------------------
// TargetRestuneParserTests
// ----------------------------------------------------------------------------
namespace TargetRestuneParserTests {
    std::string __testGroupName = "TargetRestuneParserTests";

    static ErrCode parsingStatus = RC_SUCCESS;

    static void Init() {
        UrmSettings::targetConfigs.targetName = "TestDevice";
        RestuneParser configProcessor;
        parsingStatus = configProcessor.parseTargetConfigs("/etc/urm/tests/configs/TargetConfigDup.yaml");
    }

    static void EnsureInit() {
        static bool done = false;
        if (!done) { Init(); done = true; }
    }

    MT_TEST(TargetRestuneParserTests, TargetRestuneParserYAMLDataIntegrity1, "component-serial") {
        EnsureInit();
        MT_REQUIRE(ctx, TargetRegistry::getInstance() != nullptr);
        MT_REQUIRE_EQ(ctx, parsingStatus, RC_SUCCESS);
    }

    MT_TEST(TargetRestuneParserTests, TargetRestuneParserYAMLDataIntegrity2, "component-serial") {
        EnsureInit();
        std::cout << "Determined Cluster Count = " << UrmSettings::targetConfigs.mTotalClusterCount << std::endl;
        MT_REQUIRE_EQ(ctx, UrmSettings::targetConfigs.mTotalClusterCount, 4);
    }

    MT_TEST(TargetRestuneParserTests, TargetRestuneParserYAMLDataIntegrity3, "component-serial") {
        EnsureInit();
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getPhysicalClusterId(0), 4);
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getPhysicalClusterId(1), 0);
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getPhysicalClusterId(2), 9);
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getPhysicalClusterId(3), 7);
    }

    MT_TEST(TargetRestuneParserTests, TargetRestuneParserYAMLDataIntegrity4, "component-serial") {
        EnsureInit();
        // Core distribution checks
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getPhysicalCoreId(1, 3), 2);
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getPhysicalCoreId(0, 2), 5);
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getPhysicalCoreId(3, 1), 7);
        MT_REQUIRE_EQ(ctx, TargetRegistry::getInstance()->getPhysicalCoreId(2, 1), 9);
    }
} // namespace TargetRestuneParserTests

// ----------------------------------------------------------------------------
// ExtFeaturesParsingTests
// ----------------------------------------------------------------------------
namespace ExtFeaturesParsingTests {
    std::string __testGroupName = "ExtFeaturesParsingTests";

    static ErrCode parsingStatus = RC_SUCCESS;

    static void Init() {
        RestuneParser configProcessor;
        parsingStatus = configProcessor.parseExtFeaturesConfigs("/etc/urm/tests/configs/ExtFeaturesConfig.yaml");
    }

    static void EnsureInit() {
        static bool done = false;
        if (!done) { Init(); done = true; }
    }

    MT_TEST(ExtFeaturesParsingTests, ExtFeatRestuneParserYAMLDataIntegrity1, "component-serial") {
        EnsureInit();
        MT_REQUIRE(ctx, ExtFeaturesRegistry::getInstance() != nullptr);
        MT_REQUIRE_EQ(ctx, parsingStatus, RC_SUCCESS);
    }

    MT_TEST(ExtFeaturesParsingTests, ExtFeatRestuneParserYAMLDataIntegrity3, "component-serial") {
        EnsureInit();
        ExtFeatureInfo* feature =
            ExtFeaturesRegistry::getInstance()->getExtFeatureConfigById(0x00000001);

        MT_REQUIRE(ctx, feature != nullptr);
        MT_REQUIRE_EQ(ctx, feature->mFeatureId, 0x00000001);
        MT_REQUIRE_EQ(ctx, feature->mFeatureName, std::string("FEAT-1"));
        MT_REQUIRE_EQ(ctx, feature->mFeatureLib, std::string("/usr/lib/libtesttuner.so"));

        MT_REQUIRE(ctx, feature->mSignalsSubscribedTo != nullptr);
        MT_REQUIRE_EQ(ctx, (int)feature->mSignalsSubscribedTo->size(), 2);
        MT_REQUIRE_EQ(ctx, (*feature->mSignalsSubscribedTo)[0], 0x000dbbca);
        MT_REQUIRE_EQ(ctx, (*feature->mSignalsSubscribedTo)[1], 0x000a00ff);
    }

    MT_TEST(ExtFeaturesParsingTests, ExtFeatRestuneParserYAMLDataIntegrity4, "component-serial") {
        EnsureInit();
        ExtFeatureInfo* feature =
            ExtFeaturesRegistry::getInstance()->getExtFeatureConfigById(0x00000002);

        MT_REQUIRE(ctx, (feature != nullptr));
        MT_REQUIRE_EQ(ctx, feature->mFeatureId, 0x00000002);
        MT_REQUIRE_EQ(ctx, feature->mFeatureName, std::string("FEAT-2"));
        MT_REQUIRE_EQ(ctx, feature->mFeatureLib, std::string("/usr/lib/libpropagate.so"));

        MT_REQUIRE(ctx, (feature->mSignalsSubscribedTo != nullptr));
        MT_REQUIRE_EQ(ctx, (int)feature->mSignalsSubscribedTo->size(), 2);
        MT_REQUIRE_EQ(ctx, (*feature->mSignalsSubscribedTo)[0], 0x80a105ea);
        MT_REQUIRE_EQ(ctx, (*feature->mSignalsSubscribedTo)[1], 0x800ccca5);
    }
} // namespace ExtFeaturesParsingTests

// ----------------------------------------------------------------------------
// ResourceParsingTestsAddOn
// ----------------------------------------------------------------------------
namespace ResourceParsingTestsAddOn {
    std::string __testGroupName = "ResourceParsingTestsAddOn";

    static ErrCode parsingStatus = RC_SUCCESS;

    static void Init() {
        RestuneParser configProcessor;
        std::string additionalResources = "/etc/urm/tests/configs/ResourcesConfigAddOn.yaml";

        if (RC_IS_OK(parsingStatus)) {
            // Single-argument overload for compatibility
            parsingStatus = configProcessor.parseResourceConfigs(additionalResources);
        }
    }

    static void EnsureInit() {
        static bool done = false;
        if (!done) {
            // Make sure base resources are parsed first (reuse ResourceParsingTests)
            ResourceParsingTests::EnsureInit();
            Init();
            done = true;
        }
    }

    MT_TEST(ResourceParsingTestsAddOn, ResourceParsingSanity, "component-serial") {
        EnsureInit();
        MT_REQUIRE(ctx, ResourceRegistry::getInstance() != nullptr);
        MT_REQUIRE_EQ(ctx, parsingStatus, RC_SUCCESS);
    }

    MT_TEST(ResourceParsingTestsAddOn, ResourceParsingResourcesMerged1, "component-serial") {
        EnsureInit();
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x80ff000b);

        MT_REQUIRE(ctx, (resourceConfigInfo != nullptr));
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResType, 0xff);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResID, 0x000b);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourceName.data(), "OVERRIDE_RESOURCE_1"), 0);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/etc/resouce-tuner/tests/Configs/pathB/overwrite"), 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mHighThreshold, 220);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mLowThreshold, 150);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPolicy, LOWER_BETTER);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPermissions, PERMISSION_SYSTEM);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mModes, (MODE_RESUME | MODE_DOZE));
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mApplyType, ResourceApplyType::APPLY_CORE);
    }

    MT_TEST(ResourceParsingTestsAddOn, ResourceParsingResourcesMerged2, "component-serial") {
        EnsureInit();
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x80ff1000);

        MT_REQUIRE(ctx, (resourceConfigInfo != nullptr));
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResType, 0xff);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResID, 0x1000);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourceName.data(), "CUSTOM_SCALING_FREQ"), 0);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/usr/local/customfreq/node"), 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mHighThreshold, 90);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mLowThreshold, 80);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPolicy, LAZY_APPLY);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPermissions, PERMISSION_THIRD_PARTY);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mModes, MODE_DOZE);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mApplyType, ResourceApplyType::APPLY_CORE);
    }

    MT_TEST(ResourceParsingTestsAddOn, ResourceParsingResourcesMerged3, "component-serial") {
        EnsureInit();
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x80ff1001);

        MT_REQUIRE(ctx, resourceConfigInfo != nullptr);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResType, 0xff);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResID, 0x1001);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourceName.data(), "CUSTOM_RESOURCE_ADDED_BY_BU"), 0);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/some/bu/specific/node/path/customized_to_usecase"), 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mHighThreshold, 512);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mLowThreshold, 128);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPolicy, LOWER_BETTER);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPermissions, PERMISSION_SYSTEM);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mModes, MODE_RESUME);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mApplyType, ResourceApplyType::APPLY_GLOBAL);
    }

    MT_TEST(ResourceParsingTestsAddOn, ResourceParsingResourcesMerged4, "component-serial") {
        EnsureInit();
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x80ff000c);

        MT_REQUIRE(ctx, (resourceConfigInfo != nullptr));
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResType, 0xff);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResID, 0x000c);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourceName.data(), "OVERRIDE_RESOURCE_2"), 0);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/proc/kernel/tid/kernel/uclamp.tid.sched/rt"), 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mHighThreshold, 100022);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mLowThreshold, 87755);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPolicy, INSTANT_APPLY);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPermissions, PERMISSION_THIRD_PARTY);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mModes, (MODE_RESUME | MODE_DOZE));
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mApplyType, ResourceApplyType::APPLY_GLOBAL);
    }

    MT_TEST(ResourceParsingTestsAddOn, ResourceParsingResourcesDefaultValuesCheck, "component-serial") {
        EnsureInit();
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x80ff0009);

        MT_REQUIRE(ctx, (resourceConfigInfo != nullptr));
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResType, 0xff);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mResourceResID, 0x0009);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourceName.data(), "DEFAULT_VALUES_TEST"), 0);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)resourceConfigInfo->mResourcePath.data(), ""), 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mHighThreshold, -1);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mLowThreshold, -1);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPolicy, LAZY_APPLY);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mPermissions, PERMISSION_THIRD_PARTY);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mModes, 0);
        MT_REQUIRE_EQ(ctx, resourceConfigInfo->mApplyType, ResourceApplyType::APPLY_GLOBAL);
    }
} // namespace ResourceParsingTestsAddOn

// ----------------------------------------------------------------------------
// SignalParsingTestsAddOn
// ----------------------------------------------------------------------------
namespace SignalParsingTestsAddOn {
    std::string __testGroupName = "SignalParsingTestsAddOn";

    static ErrCode parsingStatus = RC_SUCCESS;

    static void Init() {
        RestuneParser configProcessor;
        std::string signalsClassA = "/etc/urm/tests/configs/SignalsConfig.yaml";
        std::string signalsClassB = "/etc/urm/tests/configs/SignalsConfigAddOn.yaml";

        parsingStatus = configProcessor.parseSignalConfigs(signalsClassA);
        if (RC_IS_OK(parsingStatus)) {
            // Single-argument overload for compatibility
            parsingStatus = configProcessor.parseSignalConfigs(signalsClassB);
        }
    }

    static void EnsureInit() {
        static bool done = false;
        if (!done) {
            // Ensure base signal set parsed first
            SignalParsingTests::EnsureInit();
            Init();
            done = true;
        }
    }

    MT_TEST(SignalParsingTestsAddOn, SignalParsingSanity, "component-serial") {
        EnsureInit();
        MT_REQUIRE(ctx, SignalRegistry::getInstance() != nullptr);
        MT_REQUIRE_EQ(ctx, parsingStatus, RC_SUCCESS);
    }

    MT_TEST(SignalParsingTestsAddOn, SignalParsingSignalsMerged1, "component-serial") {
        EnsureInit();
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigById(0x80aaddde);

        MT_REQUIRE(ctx, (signalInfo != nullptr));
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalID, 0xaadd);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalCategory, 0xde);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mSignalName.data(), "OVERRIDE_SIGNAL_1"), 0);
        MT_REQUIRE_EQ(ctx, signalInfo->mTimeout, 14500);

        MT_REQUIRE(ctx, (signalInfo->mPermissions != nullptr));
        MT_REQUIRE(ctx, (signalInfo->mDerivatives != nullptr));
        MT_REQUIRE(ctx, (signalInfo->mSignalResources != nullptr));

        MT_REQUIRE_EQ(ctx, (int)signalInfo->mPermissions->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mDerivatives->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mSignalResources->size(), 1);

        MT_REQUIRE_EQ(ctx, signalInfo->mPermissions->at(0), PERMISSION_SYSTEM);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mDerivatives->at(0).data(), "test-derivative"), 0);

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        MT_REQUIRE_EQ(ctx, resource1->getResCode(), 0x80dbaaa0);
        MT_REQUIRE_EQ(ctx, resource1->getValuesCount(), 1);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(0), 887);
        MT_REQUIRE_EQ(ctx, resource1->getResInfo(), 0x000776aa);
    }

    MT_TEST(SignalParsingTestsAddOn, SignalParsingSignalsMerged2, "component-serial") {
        EnsureInit();
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigById(0x0000070d);

        MT_REQUIRE(ctx, signalInfo != nullptr);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalID, 0x0007);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalCategory, 0x0d);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_8"), 0);
        MT_REQUIRE_EQ(ctx, signalInfo->mTimeout, 5500);

        MT_REQUIRE(ctx, signalInfo->mPermissions != nullptr);
        MT_REQUIRE_EQ(ctx, signalInfo->mDerivatives, (decltype(signalInfo->mDerivatives))nullptr);
        MT_REQUIRE(ctx, signalInfo->mSignalResources != nullptr);

        MT_REQUIRE_EQ(ctx, (int)signalInfo->mPermissions->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mSignalResources->size(), 2);

        MT_REQUIRE_EQ(ctx, signalInfo->mPermissions->at(0), PERMISSION_THIRD_PARTY);

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        MT_REQUIRE_EQ(ctx, resource1->getResCode(), 0x000900aa);
        MT_REQUIRE_EQ(ctx, resource1->getValuesCount(), 3);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(0), -1);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(1), -1);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(2), 68);
        MT_REQUIRE_EQ(ctx, resource1->getResInfo(), 0);

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        MT_REQUIRE_EQ(ctx, resource2->getResCode(), 0x000900dc);
        MT_REQUIRE_EQ(ctx, resource2->getValuesCount(), 4);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(0), -1);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(1), -1);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(2), 50);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(3), 512);
        MT_REQUIRE_EQ(ctx, resource2->getResInfo(), 0);
    }

    MT_TEST(SignalParsingTestsAddOn, SignalParsingSignalsMerged3, "component-serial") {
        EnsureInit();
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigById(0x8000ab1e);

        MT_REQUIRE(ctx, signalInfo != nullptr);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalID, 0x00ab);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalCategory, 0x1e);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mSignalName.data(), "CUSTOM_SIGNAL_1"), 0);
        MT_REQUIRE_EQ(ctx, signalInfo->mTimeout, 6700);

        MT_REQUIRE(ctx, signalInfo->mPermissions != nullptr);
        MT_REQUIRE(ctx, signalInfo->mDerivatives != nullptr);
        MT_REQUIRE(ctx, signalInfo->mSignalResources != nullptr);

        MT_REQUIRE_EQ(ctx, (int)signalInfo->mPermissions->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mDerivatives->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mSignalResources->size(), 2);

        MT_REQUIRE_EQ(ctx, signalInfo->mPermissions->at(0), PERMISSION_THIRD_PARTY);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mDerivatives->at(0).data(), "derivative-device1"), 0);

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        MT_REQUIRE_EQ(ctx, resource1->getResCode(), 0x80f10000);
        MT_REQUIRE_EQ(ctx, resource1->getValuesCount(), 1);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(0), 665);
        MT_REQUIRE_EQ(ctx, resource1->getResInfo(), 0x0a00f000);

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        MT_REQUIRE_EQ(ctx, resource2->getResCode(), 0x800100d0);
        MT_REQUIRE_EQ(ctx, resource2->getValuesCount(), 2);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(0), 679);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(1), 812);
        MT_REQUIRE_EQ(ctx, resource2->getResInfo(), 0x00100112);
    }

    MT_TEST(SignalParsingTestsAddOn, SignalParsingSignalsMerged4, "component-serial") {
        EnsureInit();
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigById(0x00000008);
        MT_REQUIRE_EQ(ctx, (signalInfo), (SignalInfo*)nullptr);
    }

    MT_TEST(SignalParsingTestsAddOn, SignalParsingSignalsMerged5, "component-serial") {
        EnsureInit();
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigById(0x80ffcfce);

        MT_REQUIRE(ctx, (signalInfo != nullptr));
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalID, 0xffcf);
        MT_REQUIRE_EQ(ctx, signalInfo->mSignalCategory, 0xce);
        MT_REQUIRE_EQ(ctx, std::strcmp((const char*)signalInfo->mSignalName.data(), "CAMERA_OPEN_CUSTOM"), 0);
        MT_REQUIRE_EQ(ctx, signalInfo->mTimeout, 1);

        MT_REQUIRE(ctx, (signalInfo->mPermissions != nullptr));
        MT_REQUIRE_EQ(ctx, signalInfo->mDerivatives, (decltype(signalInfo->mDerivatives))nullptr);
        MT_REQUIRE(ctx, (signalInfo->mSignalResources != nullptr));

        MT_REQUIRE_EQ(ctx, (int)signalInfo->mPermissions->size(), 1);
        MT_REQUIRE_EQ(ctx, (int)signalInfo->mSignalResources->size(), 2);

        MT_REQUIRE_EQ(ctx, signalInfo->mPermissions->at(0), PERMISSION_SYSTEM);

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        MT_REQUIRE_EQ(ctx, resource1->getResCode(), 0x80d9aa00);
        MT_REQUIRE_EQ(ctx, resource1->getValuesCount(), 2);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(0), 1);
        MT_REQUIRE_EQ(ctx, resource1->getValueAt(1), 556);
        MT_REQUIRE_EQ(ctx, resource1->getResInfo(), 0);

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        MT_REQUIRE_EQ(ctx, resource2->getResCode(), 0x80c6500f);
        MT_REQUIRE_EQ(ctx, resource2->getValuesCount(), 3);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(0), 1);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(1), 900);
        MT_REQUIRE_EQ(ctx, resource2->getValueAt(2), 965);
        MT_REQUIRE_EQ(ctx, resource2->getResInfo(), 0);
    }
} // namespace SignalParsingTestsAddOn

// ----------------------------------------------------------------------------
// AppConfigParserTests
// ----------------------------------------------------------------------------
namespace AppConfigParserTests {
    std::string __testGroupName = "AppConfigParserTests";

    static ErrCode parsingStatus = RC_SUCCESS;

    static void Init() {
        RestuneParser configProcessor;
        std::string perAppConfPath = "/etc/urm/tests/configs/PerApp.yaml";

        if (RC_IS_OK(parsingStatus)) {
            parsingStatus = configProcessor.parsePerAppConfigs(perAppConfPath);
        }
    }

    static void EnsureInit() {
        static bool done = false;
        if (!done) { Init(); done = true; }
    }

    MT_TEST(AppConfigParserTests, AppConfigParsingSanity, "component-serial") {
        EnsureInit();
        MT_REQUIRE(ctx, AppConfigs::getInstance() != nullptr);
        MT_REQUIRE_EQ(ctx, parsingStatus, RC_SUCCESS);
    }

    MT_TEST(AppConfigParserTests, AppConfigParsingIntegrity1, "component-serial") {
        EnsureInit();
        AppConfig* appConfigInfo = AppConfigs::getInstance()->getAppConfig("gst-launch-");

        MT_REQUIRE_EQ(ctx, appConfigInfo->mAppName, std::string("gst-launch-"));
        MT_REQUIRE_EQ(ctx, appConfigInfo->mNumThreads, 2);

        MT_REQUIRE(ctx, (appConfigInfo->mThreadNameList != nullptr));
        MT_REQUIRE(ctx, (appConfigInfo->mCGroupIds != nullptr));

        MT_REQUIRE_EQ(ctx, appConfigInfo->mNumSignals, 0);
        MT_REQUIRE_EQ(ctx, appConfigInfo->mSignalCodes, (decltype(appConfigInfo->mSignalCodes))nullptr);
    }

    MT_TEST(AppConfigParserTests, AppConfigParsingIntegrity2, "component-serial") {
        EnsureInit();
        AppConfig* appConfigInfo = AppConfigs::getInstance()->getAppConfig("chrome");

        MT_REQUIRE_EQ(ctx, appConfigInfo->mAppName, std::string("chrome"));
        MT_REQUIRE_EQ(ctx, appConfigInfo->mNumThreads, 1);

        MT_REQUIRE(ctx, (appConfigInfo->mThreadNameList != nullptr));
        MT_REQUIRE(ctx, (appConfigInfo->mCGroupIds != nullptr));

        MT_REQUIRE_EQ(ctx, appConfigInfo->mNumSignals, 1);
        MT_REQUIRE(ctx, (appConfigInfo->mSignalCodes != nullptr));
    }
} // namespace AppConfigParserTests

// NOTE: No RunTests() / REGISTER_TEST() needed — mini.hpp auto-registers tests
// and provides main() unless compiled with -DMTEST_NO_MAIN.
//
// Run serially:
//   ./RestuneMiniTests --tag=component-serial --threads=1

