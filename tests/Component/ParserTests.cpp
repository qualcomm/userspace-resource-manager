// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Utils.h"
#include "URMTests.h"
#include "ErrCodes.h"
#include "TestUtils.h"
#include "Extensions.h"
#include "RestuneParser.h"
#include "SignalRegistry.h"
#include "RestuneInternal.h"
#include "ResourceRegistry.h"
#include "PropertiesRegistry.h"

#define TEST_CLASS "COMPONENT"
#define TEST_SUBCAT "0_URM_PARSERS"

URM_TEST(ResourceParsingTests, {
    {
        ErrCode parsingStatus = RC_SUCCESS;
        RestuneParser configProcessor;
        parsingStatus = configProcessor.parseResourceConfigs("/etc/urm/tests/configs/ResourcesConfig.yaml");

        E_ASSERT((ResourceRegistry::getInstance() != nullptr));
        E_ASSERT((parsingStatus == RC_SUCCESS));

        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x00ff0000);

        E_ASSERT((resourceConfigInfo != nullptr));
        E_ASSERT((resourceConfigInfo->mResourceResType == 0xff));
        E_ASSERT((resourceConfigInfo->mResourceResID == 0));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourceName.data(), "TEST_RESOURCE_1") == 0));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/etc/urm/tests/nodes/sched_util_clamp_min.txt") == 0));
        E_ASSERT((resourceConfigInfo->mHighThreshold == 1024));
        E_ASSERT((resourceConfigInfo->mLowThreshold == 0));
        E_ASSERT((resourceConfigInfo->mPolicy == HIGHER_BETTER));
        E_ASSERT((resourceConfigInfo->mPermissions == PERMISSION_THIRD_PARTY));
        E_ASSERT((resourceConfigInfo->mModes == (MODE_RESUME | MODE_DOZE)));
        E_ASSERT((resourceConfigInfo->mApplyType == ResourceApplyType::APPLY_GLOBAL));
    }

    {
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x00ff0001);

        E_ASSERT((resourceConfigInfo != nullptr));
        E_ASSERT((resourceConfigInfo->mResourceResType == 0xff));
        E_ASSERT((resourceConfigInfo->mResourceResID == 1));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourceName.data(), "TEST_RESOURCE_2") == 0));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/etc/urm/tests/nodes/sched_util_clamp_max.txt") == 0));
        E_ASSERT((resourceConfigInfo->mHighThreshold == 1024));
        E_ASSERT((resourceConfigInfo->mLowThreshold == 512));
        E_ASSERT((resourceConfigInfo->mPolicy == HIGHER_BETTER));
        E_ASSERT((resourceConfigInfo->mPermissions == PERMISSION_THIRD_PARTY));
        E_ASSERT((resourceConfigInfo->mModes == (MODE_RESUME | MODE_DOZE)));
        E_ASSERT((resourceConfigInfo->mApplyType == ResourceApplyType::APPLY_GLOBAL));
    }

    {
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x00ff0005);

        E_ASSERT((resourceConfigInfo != nullptr));
        E_ASSERT((resourceConfigInfo->mResourceResType == 0xff));
        E_ASSERT((resourceConfigInfo->mResourceResID == 5));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourceName.data(), "TEST_RESOURCE_6") == 0));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/etc/urm/tests/nodes/target_test_resource2.txt") == 0));
        E_ASSERT((resourceConfigInfo->mHighThreshold == 6500));
        E_ASSERT((resourceConfigInfo->mLowThreshold == 50));
        E_ASSERT((resourceConfigInfo->mPolicy == HIGHER_BETTER));
        E_ASSERT((resourceConfigInfo->mPermissions == PERMISSION_THIRD_PARTY));
        E_ASSERT((resourceConfigInfo->mModes == MODE_RESUME));
        E_ASSERT((resourceConfigInfo->mApplyType == ResourceApplyType::APPLY_CORE));
    }
})

URM_TEST(SignalParsingTests, {
    {
        ErrCode parsingStatus = RC_SUCCESS;
        RestuneParser configProcessor;
        parsingStatus = configProcessor.parseSignalConfigs("/etc/urm/tests/configs/SignalsConfig.yaml");

        E_ASSERT((SignalRegistry::getInstance() != nullptr));
        E_ASSERT((parsingStatus == RC_SUCCESS));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x0000), 0
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0));
        E_ASSERT((signalInfo->mSignalCategory == 0x0d));
        E_ASSERT((signalInfo->mSigType == 0));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_1") == 0));
        E_ASSERT((signalInfo->mTimeout == 4000));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mDerivatives != nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 1));
        E_ASSERT((signalInfo->mDerivatives->size() == 1));
        E_ASSERT((signalInfo->mSignalResources->size() == 1));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
        E_ASSERT((strcmp((const char*)signalInfo->mDerivatives->at(0).data(), "solar") == 0));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1 != nullptr));
        E_ASSERT((resource1->getResCode() == 0x00010000));
        E_ASSERT((resource1->getValuesCount() == 1));
        E_ASSERT((resource1->getValueAt(0) == 700));
        E_ASSERT((resource1->getResInfo() == 0));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x0001), 0
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 1));
        E_ASSERT((signalInfo->mSignalCategory == 0x0d));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_2") == 0));
        E_ASSERT((signalInfo->mTimeout == 5000));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mDerivatives != nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 1));
        E_ASSERT((signalInfo->mDerivatives->size() == 1));
        E_ASSERT((signalInfo->mSignalResources->size() == 2));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_SYSTEM));

        E_ASSERT((strcmp((const char*)signalInfo->mDerivatives->at(0).data(), "derivative_v2") == 0));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 8));
        E_ASSERT((resource1->getValuesCount() == 1));
        E_ASSERT((resource1->getValueAt(0) == 814));
        E_ASSERT((resource1->getResInfo() == 0));

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        E_ASSERT((resource2->getResCode() == 15));
        E_ASSERT((resource2->getValuesCount() == 2));
        E_ASSERT((resource2->getValueAt(0) == 23));
        E_ASSERT((resource2->getValueAt(1) == 90));
        E_ASSERT((resource2->getResInfo() == 256));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x0003), 0
        );
        E_ASSERT((signalInfo == nullptr));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x0007), 0
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0x0007));
        E_ASSERT((signalInfo->mSignalCategory == 0x0d));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_8") == 0));
        E_ASSERT((signalInfo->mTimeout == 5500));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mDerivatives == nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 1));
        E_ASSERT((signalInfo->mSignalResources->size() == 2));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 0x000900aa));
        E_ASSERT((resource1->getValuesCount() == 3));
        E_ASSERT((resource1->getValueAt(0) == NSIG_PLACEHOLDER));
        E_ASSERT((resource1->getValueAt(1) == NSIG_PLACEHOLDER));
        E_ASSERT((resource1->getValueAt(2) == 68));
        E_ASSERT((resource1->getResInfo() == 0));

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        E_ASSERT((resource2->getResCode() == 0x000900dc));
        E_ASSERT((resource2->getValuesCount() == 4));
        E_ASSERT((resource2->getValueAt(0) == NSIG_PLACEHOLDER));
        E_ASSERT((resource2->getValueAt(1) == NSIG_PLACEHOLDER));
        E_ASSERT((resource2->getValueAt(2) == 50));
        E_ASSERT((resource2->getValueAt(3) == 512));
        E_ASSERT((resource2->getResInfo() == 0));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x000b), 27
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0x000b));
        E_ASSERT((signalInfo->mSignalCategory == 0x0d));
        E_ASSERT((signalInfo->mSigType == 27));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "SIGNAL_WITH_SIGTYPE") == 0));
        E_ASSERT((signalInfo->mTimeout == 10000));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mDerivatives == nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 2));
        E_ASSERT((signalInfo->mSignalResources->size() == 1));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
        E_ASSERT((signalInfo->mPermissions->at(1) == PERMISSION_SYSTEM));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 0x00ff0004));
        E_ASSERT((resource1->getValuesCount() == 1));
        E_ASSERT((resource1->getValueAt(0) == 231));
        E_ASSERT((resource1->getResInfo() == 0));

        // Signals without ExtraAttrs must default to 0 for all indices
        E_ASSERT((signalInfo->mHasExtraAttrs == false));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_FPS] == 0));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_HEIGHT] == 0));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_WIDTH] == 0));
    }

    {
        uint32_t* extraAttrs = new uint32_t[SIGNAL_EXTRA_ATTRS_COUNT];
        extraAttrs[0] = 60;
        extraAttrs[1] = 1080;
        extraAttrs[2] = 1920;

        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x000c), 0, SIGNAL_EXTRA_ATTRS_COUNT, extraAttrs
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0x000c));
        E_ASSERT((signalInfo->mSignalCategory == 0x0d));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_10") == 0));
        E_ASSERT((signalInfo->mTimeout == 10000));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 2));
        E_ASSERT((signalInfo->mSignalResources->size() == 1));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
        E_ASSERT((signalInfo->mPermissions->at(1) == PERMISSION_SYSTEM));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 0x00ff0004));
        E_ASSERT((resource1->getValuesCount() == 1));
        E_ASSERT((resource1->getValueAt(0) == 231));
        E_ASSERT((resource1->getResInfo() == 0));

        // Verify ExtraAttrs are parsed correctly into the array
        E_ASSERT((signalInfo->mHasExtraAttrs == true));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_FPS] == 60));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_HEIGHT] == 1080));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_WIDTH] == 1920));
    }

    {
        uint32_t* extraAttrs = new uint32_t[SIGNAL_EXTRA_ATTRS_COUNT];
        extraAttrs[0] = 30;
        extraAttrs[1] = 720;
        extraAttrs[2] = 1280;
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x000d), 0, SIGNAL_EXTRA_ATTRS_COUNT, extraAttrs
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0x000d));
        E_ASSERT((signalInfo->mSignalCategory == 0x0d));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_11") == 0));
        E_ASSERT((signalInfo->mTimeout == 8000));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 2));
        E_ASSERT((signalInfo->mSignalResources->size() == 2));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
        E_ASSERT((signalInfo->mPermissions->at(1) == PERMISSION_SYSTEM));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 0x00ff0004));
        E_ASSERT((resource1->getValuesCount() == 1));
        E_ASSERT((resource1->getValueAt(0) == 115));
        E_ASSERT((resource1->getResInfo() == 0));

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        E_ASSERT((resource2->getResCode() == 0x00ff0000));
        E_ASSERT((resource2->getValuesCount() == 1));
        E_ASSERT((resource2->getValueAt(0) == 512));
        E_ASSERT((resource2->getResInfo() == 0));

        E_ASSERT((signalInfo->mHasExtraAttrs == true));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_FPS] == 30));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_HEIGHT] == 720));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_WIDTH] == 1280));
    }

    {
        // Three Versions of the same Signal, where two of them have extra features
        // All Signals should co-exist
        SignalInfo* signalInfo1 = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x000e), 0
        );
        E_ASSERT((signalInfo1 != nullptr));
        E_ASSERT((signalInfo1->mSignalID == 0x000e));
        E_ASSERT((signalInfo1->mSignalCategory == 0x0d));
        E_ASSERT((strcmp((const char*)signalInfo1->mSignalName.data(), "TEST_SIGNAL_12") == 0));

        E_ASSERT((signalInfo1->mPermissions != nullptr));
        E_ASSERT((signalInfo1->mSignalResources != nullptr));

        E_ASSERT((signalInfo1->mPermissions->size() == 2));
        E_ASSERT((signalInfo1->mSignalResources->size() == 1));

        E_ASSERT((signalInfo1->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
        E_ASSERT((signalInfo1->mPermissions->at(1) == PERMISSION_SYSTEM));

        Resource* resource = signalInfo1->mSignalResources->at(0);
        E_ASSERT((resource->getResCode() == 0x00ff0002));
        E_ASSERT((resource->getValuesCount() == 1));
        E_ASSERT((resource->getValueAt(0) == 115));
        E_ASSERT((resource->getResInfo() == 0));

        // Signal With-Features #1
        uint32_t* extraAttrs = new uint32_t[SIGNAL_EXTRA_ATTRS_COUNT];
        extraAttrs[0] = 120;
        extraAttrs[1] = 2160;
        extraAttrs[2] = 3840;
        SignalInfo* signalInfo2 = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x000e), 0, SIGNAL_EXTRA_ATTRS_COUNT, extraAttrs
        );
        E_ASSERT((signalInfo2 != nullptr));
        E_ASSERT((signalInfo2->mSignalID == 0x000e));
        E_ASSERT((signalInfo2->mSignalCategory == 0x0d));
        E_ASSERT((strcmp((const char*)signalInfo2->mSignalName.data(), "TEST_SIGNAL_12_WEF_1") == 0));

        E_ASSERT((signalInfo2->mPermissions != nullptr));
        E_ASSERT((signalInfo2->mSignalResources != nullptr));

        E_ASSERT((signalInfo2->mPermissions->size() == 2));
        E_ASSERT((signalInfo2->mSignalResources->size() == 1));

        E_ASSERT((signalInfo2->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
        E_ASSERT((signalInfo2->mPermissions->at(1) == PERMISSION_SYSTEM));

        resource = signalInfo2->mSignalResources->at(0);
        E_ASSERT((resource->getResCode() == 0x00ff0000));
        E_ASSERT((resource->getValuesCount() == 1));
        E_ASSERT((resource->getValueAt(0) == 512));
        E_ASSERT((resource->getResInfo() == 0));

        // Signal With-Features #2
        uint32_t* extraAttrs2 = new uint32_t[SIGNAL_EXTRA_ATTRS_COUNT];
        extraAttrs[0] = 30;
        extraAttrs[1] = 4320;
        extraAttrs[2] = 7680;
        SignalInfo* signalInfo3 = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x000e), 0, SIGNAL_EXTRA_ATTRS_COUNT, extraAttrs2
        );
        E_ASSERT((signalInfo3 != nullptr));
        E_ASSERT((signalInfo3->mSignalID == 0x000e));
        E_ASSERT((signalInfo3->mSignalCategory == 0x0d));
        E_ASSERT((strcmp((const char*)signalInfo3->mSignalName.data(), "TEST_SIGNAL_12_WEF_2") == 0));

        E_ASSERT((signalInfo3->mPermissions != nullptr));
        E_ASSERT((signalInfo3->mSignalResources != nullptr));

        E_ASSERT((signalInfo3->mPermissions->size() == 2));
        E_ASSERT((signalInfo3->mSignalResources->size() == 1));

        E_ASSERT((signalInfo3->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
        E_ASSERT((signalInfo3->mPermissions->at(1) == PERMISSION_SYSTEM));

        resource = signalInfo3->mSignalResources->at(0);
        E_ASSERT((resource->getResCode() == 0x00ff0005));
        E_ASSERT((resource->getValuesCount() == 1));
        E_ASSERT((resource->getValueAt(0) == 874));
        E_ASSERT((resource->getResInfo() == 0x00000101));
    }
})

URM_TEST(InitConfigParsingTests, {
    std::shared_ptr<TargetRegistry> targetRegistry = TargetRegistry::getInstance();

    ErrCode parsingStatus = RC_SUCCESS;
    RestuneParser configProcessor;
    parsingStatus = configProcessor.parseInitConfigs("/etc/urm/tests/configs/InitConfig.yaml");

    E_ASSERT((targetRegistry != nullptr));
    E_ASSERT((parsingStatus == RC_SUCCESS));

    {
        std::cout<<"Count of Cgroups created: "<<targetRegistry->getCreatedCGroupsCount()<<std::endl;
        E_ASSERT((targetRegistry->getCreatedCGroupsCount() == 3));

        // Note don't rely on order here, since internally CGroup mapping data is stored
        // as an unordered_map.
        std::vector<std::string> cGroupNames;
        targetRegistry->getCGroupNames(cGroupNames);
        std::vector<std::string> expectedNames = {"camera-cgroup", "audio-cgroup", "video-cgroup"};

        E_ASSERT((cGroupNames.size() == 3));

        std::unordered_set<std::string> expectedNamesSet;
        for(int32_t i = 0; i < (int32_t)cGroupNames.size(); i++) {
            expectedNamesSet.insert(cGroupNames[i]);
        }

        for(int32_t i = 0; i < (int32_t)expectedNames.size(); i++) {
            E_ASSERT((expectedNamesSet.find(expectedNames[i]) != expectedNamesSet.end()));
        }

        CGroupConfigInfo* cameraConfig = targetRegistry->getCGroupConfig(801);
        E_ASSERT((cameraConfig != nullptr));
        E_ASSERT((cameraConfig->mCgroupName == "camera-cgroup"));
        E_ASSERT((cameraConfig->mIsThreaded == false));

        CGroupConfigInfo* videoConfig = targetRegistry->getCGroupConfig(803);
        E_ASSERT((videoConfig != nullptr));
        E_ASSERT((videoConfig->mCgroupName == "video-cgroup"));
        E_ASSERT((videoConfig->mIsThreaded == true));
    }

    {
        E_ASSERT((targetRegistry->getCreatedMpamGroupsCount() == 3));

        // Note don't rely on order here, since internally CGroup mapping data is stored
        // as an unordered_map.
        std::vector<std::string> mpamGroupNames;
        targetRegistry->getMpamGroupNames(mpamGroupNames);
        std::vector<std::string> expectedNames =
            {"camera-mpam-group", "audio-mpam-group", "video-mpam-group"};

        E_ASSERT((mpamGroupNames.size() == 3));

        std::unordered_set<std::string> expectedNamesSet;
        for(int32_t i = 0; i < (int32_t)mpamGroupNames.size(); i++) {
            expectedNamesSet.insert(mpamGroupNames[i]);
        }

        for(int32_t i = 0; i < (int32_t)expectedNames.size(); i++) {
            E_ASSERT((expectedNamesSet.find(expectedNames[i]) != expectedNamesSet.end()));
        }

        MpamGroupConfigInfo* cameraConfig = targetRegistry->getMpamGroupConfig(0);
        E_ASSERT((cameraConfig != nullptr));
        E_ASSERT((cameraConfig->mMpamGroupName == "camera-mpam-group"));
        E_ASSERT((cameraConfig->mMpamGroupInfoID == 0));
        E_ASSERT((cameraConfig->mPriority == 0));

        MpamGroupConfigInfo* videoConfig = targetRegistry->getMpamGroupConfig(2);
        E_ASSERT((videoConfig != nullptr));
        E_ASSERT((videoConfig->mMpamGroupName == "video-mpam-group"));
        E_ASSERT((videoConfig->mMpamGroupInfoID == 2));
        E_ASSERT((videoConfig->mPriority == 2));
    }
})

URM_TEST(PropertyParsingTests, {
    {
        ErrCode parsingStatus = RC_SUCCESS;
        RestuneParser configProcessor;

        parsingStatus = configProcessor.parsePropertiesConfigs("/etc/urm/tests/configs/PropertiesConfig.yaml");
        E_ASSERT((PropertiesRegistry::getInstance() != nullptr));
        E_ASSERT((parsingStatus == RC_SUCCESS));
    }

    {
        std::string resultBuffer;

        size_t bytesWritten = submitPropGetRequest("test.debug.enabled", resultBuffer, "false");

        E_ASSERT((bytesWritten > 0));
        E_ASSERT((strcmp(resultBuffer.c_str(), "true") == 0));
    }

    {
        std::string resultBuffer;

        size_t bytesWritten = submitPropGetRequest("test.current.worker_thread.count", resultBuffer, "false");

        E_ASSERT((bytesWritten > 0));
        E_ASSERT((strcmp(resultBuffer.c_str(), "125") == 0));
    }

    {
        std::string resultBuffer;

        size_t bytesWritten = submitPropGetRequest("test.historic.worker_thread.count", resultBuffer, "5");

        E_ASSERT((bytesWritten == 0));
        E_ASSERT((strcmp(resultBuffer.c_str(), "5") == 0));
    }

    {
        std::thread th1([&]{
            std::string resultBuffer;
            size_t bytesWritten = submitPropGetRequest("test.current.worker_thread.count", resultBuffer, "false");

            E_ASSERT((bytesWritten > 0));
            E_ASSERT((strcmp(resultBuffer.c_str(), "125") == 0));
        });

        std::thread th2([&]{
            std::string resultBuffer;
            size_t bytesWritten = submitPropGetRequest("test.debug.enabled", resultBuffer, "false");

            E_ASSERT((bytesWritten > 0));
            E_ASSERT((strcmp(resultBuffer.c_str(), "true") == 0));
        });

        std::thread th3([&]{
            std::string resultBuffer;
            size_t bytesWritten = submitPropGetRequest("test.doc.build.mode.enabled", resultBuffer, "false");

            E_ASSERT((bytesWritten > 0));
            E_ASSERT((strcmp(resultBuffer.c_str(), "false") == 0));
        });

        th1.join();
        th2.join();
        th3.join();
    }
})

URM_TEST(TargetRestuneParserTests, {
    std::shared_ptr<TargetRegistry> targetRegistry = TargetRegistry::getInstance();
    {
        UrmSettings::targetConfigs.targetName = "TestDevice";
        ErrCode parsingStatus = RC_SUCCESS;
        RestuneParser configProcessor;
        parsingStatus = configProcessor.parseTargetConfigs("/etc/urm/tests/configs/TargetConfigDup.yaml");

        E_ASSERT((targetRegistry != nullptr));
        E_ASSERT((parsingStatus == RC_SUCCESS));
    }

    {
        std::cout<<"Determined Cluster Count = "<<UrmSettings::targetConfigs.mTotalClusterCount<<std::endl;
        E_ASSERT((UrmSettings::targetConfigs.mTotalClusterCount == 4));
    }

    {
        E_ASSERT((targetRegistry->getPhysicalClusterId(0) == 4));
        E_ASSERT((targetRegistry->getPhysicalClusterId(1) == 0));
        E_ASSERT((targetRegistry->getPhysicalClusterId(2) == 9));
        E_ASSERT((targetRegistry->getPhysicalClusterId(3) == 7));
    }

    {
        // Distribution of physical clusters
        // 1:0 => 0, 1, 2, 3
        // 0:4 => 4, 5, 6
        // 3:7 => 7, 8
        // 2:9 => 9
        E_ASSERT((targetRegistry->getPhysicalCoreId(1, 3) == 2));

        E_ASSERT((targetRegistry->getPhysicalCoreId(0, 2) == 5));

        E_ASSERT((targetRegistry->getPhysicalCoreId(3, 1) == 7));

        E_ASSERT((targetRegistry->getPhysicalCoreId(2, 1) == 9));
    }
})

URM_TEST(ExtFeaturesParsingTests, {
    {
        ErrCode parsingStatus = RC_SUCCESS;
        RestuneParser configProcessor;

        parsingStatus = configProcessor.parseExtFeaturesConfigs("/etc/urm/tests/configs/ExtFeaturesConfig.yaml");
        E_ASSERT((ExtFeaturesRegistry::getInstance() != nullptr));
        E_ASSERT((parsingStatus == RC_SUCCESS));
    }

    {
        ExtFeatureInfo* feature =
            ExtFeaturesRegistry::getInstance()->getExtFeatureConfigById(0x00000001);

        E_ASSERT((feature != nullptr));
        E_ASSERT((feature->mFeatureId == 0x00000001));
        E_ASSERT((feature->mFeatureName == "FEAT-1"));
        E_ASSERT((feature->mFeatureLib == "/usr/lib/libtesttuner.so"));

        E_ASSERT((feature->mSignalsSubscribedTo != nullptr));
        E_ASSERT((feature->mSignalsSubscribedTo->size() == 2));
        E_ASSERT(((*feature->mSignalsSubscribedTo)[0] == 0x000dbbca));
        E_ASSERT(((*feature->mSignalsSubscribedTo)[1] == 0x000a00ff));
    }

    {
        ExtFeatureInfo* feature =
            ExtFeaturesRegistry::getInstance()->getExtFeatureConfigById(0x00000002);

        E_ASSERT((feature != nullptr));
        E_ASSERT((feature->mFeatureId == 0x00000002));
        E_ASSERT((feature->mFeatureName == "FEAT-2"));
        E_ASSERT((feature->mFeatureLib == "/usr/lib/libpropagate.so"));

        E_ASSERT((feature->mSignalsSubscribedTo != nullptr));
        E_ASSERT((feature->mSignalsSubscribedTo->size() == 2));
        E_ASSERT(((*feature->mSignalsSubscribedTo)[0] == 0x00a105ea));
        E_ASSERT(((*feature->mSignalsSubscribedTo)[1] == 0x000ccca5));
    }
})

URM_TEST(ResourceParsingTestsAddOn, {
    {
        ErrCode parsingStatus = RC_SUCCESS;
        std::string additionalResources = "/etc/urm/tests/configs/ResourcesConfigAddOn.yaml";

        RestuneParser configProcessor;
        parsingStatus = configProcessor.parseResourceConfigs(additionalResources);

        E_ASSERT((ResourceRegistry::getInstance() != nullptr));
        E_ASSERT((parsingStatus == RC_SUCCESS));
    }

    {
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x00ff000b);

        E_ASSERT((resourceConfigInfo != nullptr));
        E_ASSERT((resourceConfigInfo->mResourceResType == 0xff));
        E_ASSERT((resourceConfigInfo->mResourceResID == 0x000b));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourceName.data(), "OVERRIDE_RESOURCE_1") == 0));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/etc/resouce-tuner/tests/Configs/pathB/overwrite") == 0));
        E_ASSERT((resourceConfigInfo->mHighThreshold == 220));
        E_ASSERT((resourceConfigInfo->mLowThreshold == 150));
        E_ASSERT((resourceConfigInfo->mPolicy == LOWER_BETTER));
        E_ASSERT((resourceConfigInfo->mPermissions == PERMISSION_SYSTEM));
        E_ASSERT((resourceConfigInfo->mModes == (MODE_RESUME | MODE_DOZE)));
        E_ASSERT((resourceConfigInfo->mApplyType == ResourceApplyType::APPLY_CORE));
    }

    {
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x00ff1000);

        E_ASSERT((resourceConfigInfo != nullptr));
        E_ASSERT((resourceConfigInfo->mResourceResType == 0xff));
        E_ASSERT((resourceConfigInfo->mResourceResID == 0x1000));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourceName.data(), "CUSTOM_SCALING_FREQ") == 0));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/usr/local/customfreq/node") == 0));
        E_ASSERT((resourceConfigInfo->mHighThreshold == 90));
        E_ASSERT((resourceConfigInfo->mLowThreshold == 80));
        E_ASSERT((resourceConfigInfo->mPolicy == LAZY_APPLY));
        E_ASSERT((resourceConfigInfo->mPermissions == PERMISSION_THIRD_PARTY));
        E_ASSERT((resourceConfigInfo->mModes == MODE_DOZE));
        E_ASSERT((resourceConfigInfo->mApplyType == ResourceApplyType::APPLY_CORE));
    }

    {
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x00ff1001);

        E_ASSERT((resourceConfigInfo != nullptr));
        E_ASSERT((resourceConfigInfo->mResourceResType == 0xff));
        E_ASSERT((resourceConfigInfo->mResourceResID == 0x1001));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourceName.data(), "CUSTOM_RESOURCE_ADDED_BY_BU") == 0));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/some/bu/specific/node/path/customized_to_usecase") == 0));
        E_ASSERT((resourceConfigInfo->mHighThreshold == 512));
        E_ASSERT((resourceConfigInfo->mLowThreshold == 128));
        E_ASSERT((resourceConfigInfo->mPolicy == LOWER_BETTER));
        E_ASSERT((resourceConfigInfo->mPermissions == PERMISSION_SYSTEM));
        E_ASSERT((resourceConfigInfo->mModes == MODE_RESUME));
        E_ASSERT((resourceConfigInfo->mApplyType == ResourceApplyType::APPLY_GLOBAL));
    }

    {
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x00ff000c);

        E_ASSERT((resourceConfigInfo != nullptr));
        E_ASSERT((resourceConfigInfo->mResourceResType == 0xff));
        E_ASSERT((resourceConfigInfo->mResourceResID == 0x000c));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourceName.data(), "OVERRIDE_RESOURCE_2") == 0));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "/proc/kernel/tid/kernel/uclamp.tid.sched/rt") == 0));
        E_ASSERT((resourceConfigInfo->mHighThreshold == 100022));
        E_ASSERT((resourceConfigInfo->mLowThreshold == 87755));
        E_ASSERT((resourceConfigInfo->mPolicy == INSTANT_APPLY));
        E_ASSERT((resourceConfigInfo->mPermissions == PERMISSION_THIRD_PARTY));
        E_ASSERT((resourceConfigInfo->mModes == (MODE_RESUME | MODE_DOZE)));
        E_ASSERT((resourceConfigInfo->mApplyType == ResourceApplyType::APPLY_GLOBAL));
    }

    {
        ResConfInfo* resourceConfigInfo = ResourceRegistry::getInstance()->getResConf(0x00ff0009);

        E_ASSERT((resourceConfigInfo != nullptr));
        E_ASSERT((resourceConfigInfo->mResourceResType == 0xff));
        E_ASSERT((resourceConfigInfo->mResourceResID == 0x0009));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourceName.data(), "DEFAULT_VALUES_TEST") == 0));
        E_ASSERT((strcmp((const char*)resourceConfigInfo->mResourcePath.data(), "") == 0));
        E_ASSERT((resourceConfigInfo->mHighThreshold == -1));
        E_ASSERT((resourceConfigInfo->mLowThreshold == -1));
        E_ASSERT((resourceConfigInfo->mPolicy == LAZY_APPLY));
        E_ASSERT((resourceConfigInfo->mPermissions == PERMISSION_THIRD_PARTY));
        E_ASSERT((resourceConfigInfo->mModes == 0));
        E_ASSERT((resourceConfigInfo->mApplyType == ResourceApplyType::APPLY_GLOBAL));
    }
})

URM_TEST(SignalParsingTestsAddOn, {
    {
        ErrCode parsingStatus = RC_SUCCESS;
        RestuneParser configProcessor;

        std::string signalsClassA = "/etc/urm/tests/configs/SignalsConfig.yaml";
        std::string signalsClassB = "/etc/urm/tests/configs/SignalsConfigAddOn.yaml";

        parsingStatus = configProcessor.parseSignalConfigs(signalsClassA);
        parsingStatus = configProcessor.parseSignalConfigs(signalsClassB);

        E_ASSERT((SignalRegistry::getInstance() != nullptr));
        E_ASSERT((parsingStatus == RC_SUCCESS));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0xde, 0xaadd), 0
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0xaadd));
        E_ASSERT((signalInfo->mSignalCategory == 0xde));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "OVERRIDE_SIGNAL_1") == 0));
        E_ASSERT((signalInfo->mTimeout == 14500));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mDerivatives != nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 1));
        E_ASSERT((signalInfo->mDerivatives->size() == 1));
        E_ASSERT((signalInfo->mSignalResources->size() == 1));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_SYSTEM));

        E_ASSERT((strcmp((const char*)signalInfo->mDerivatives->at(0).data(), "test-derivative") == 0));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 0x00dbaaa0));
        E_ASSERT((resource1->getValuesCount() == 1));
        E_ASSERT((resource1->getValueAt(0) == 887));
        E_ASSERT((resource1->getResInfo() == 0x000776aa));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x0007), 0
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0x0007));
        E_ASSERT((signalInfo->mSignalCategory == 0x0d));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_8") == 0));
        E_ASSERT((signalInfo->mTimeout == 5500));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mDerivatives == nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 1));
        E_ASSERT((signalInfo->mSignalResources->size() == 2));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 0x000900aa));
        E_ASSERT((resource1->getValuesCount() == 3));
        E_ASSERT((resource1->getValueAt(0) == NSIG_PLACEHOLDER));
        E_ASSERT((resource1->getValueAt(1) == NSIG_PLACEHOLDER));
        E_ASSERT((resource1->getValueAt(2) == 68));
        E_ASSERT((resource1->getResInfo() == 0));

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        E_ASSERT((resource2->getResCode() == 0x000900dc));
        E_ASSERT((resource2->getValuesCount() == 4));
        E_ASSERT((resource2->getValueAt(0) == NSIG_PLACEHOLDER));
        E_ASSERT((resource2->getValueAt(1) == NSIG_PLACEHOLDER));
        E_ASSERT((resource2->getValueAt(2) == 50));
        E_ASSERT((resource2->getValueAt(3) == 512));
        E_ASSERT((resource2->getResInfo() == 0));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x1e, 0x00ab), 0
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0x00ab));
        E_ASSERT((signalInfo->mSignalCategory == 0x1e));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "CUSTOM_SIGNAL_1") == 0));
        E_ASSERT((signalInfo->mTimeout == 6700));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mDerivatives != nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 1));
        E_ASSERT((signalInfo->mDerivatives->size() == 1));
        E_ASSERT((signalInfo->mSignalResources->size() == 2));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));

        E_ASSERT((strcmp((const char*)signalInfo->mDerivatives->at(0).data(), "derivative-device1") == 0));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 0x00f10000));
        E_ASSERT((resource1->getValuesCount() == 1));
        E_ASSERT((resource1->getValueAt(0) == 665));
        E_ASSERT((resource1->getResInfo() == 0x0a00f000));

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        E_ASSERT((resource2->getResCode() == 0x000100d0));
        E_ASSERT((resource2->getValuesCount() == 2));
        E_ASSERT((resource2->getValueAt(0) == 679));
        E_ASSERT((resource2->getValueAt(1) == 812));
        E_ASSERT((resource2->getResInfo() == 0x00100112));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x08, 0x0000), 0
        );
        E_ASSERT((signalInfo == nullptr));
    }

    {
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0xce, 0xffcf), 0
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0xffcf));
        E_ASSERT((signalInfo->mSignalCategory == 0xce));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "CAMERA_OPEN_CUSTOM") == 0));
        E_ASSERT((signalInfo->mTimeout == 1));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mDerivatives == nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 1));
        E_ASSERT((signalInfo->mSignalResources->size() == 2));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_SYSTEM));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 0x00d9aa00));
        E_ASSERT((resource1->getValuesCount() == 2));
        E_ASSERT((resource1->getValueAt(0) == 1));
        E_ASSERT((resource1->getValueAt(1) == 556));
        E_ASSERT((resource1->getResInfo() == 0));

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        E_ASSERT((resource2->getResCode() == 0x00c6500f));
        E_ASSERT((resource2->getValuesCount() == 3));
        E_ASSERT((resource2->getValueAt(0) == 1));
        E_ASSERT((resource2->getValueAt(1)  == 900));
        E_ASSERT((resource2->getValueAt(2)  == 965));
        E_ASSERT((resource2->getResInfo() == 0));
    }

    {
        // Verify that a signal with extra features can be overridden
        uint32_t* extraAttrs = new uint32_t[SIGNAL_EXTRA_ATTRS_COUNT];
        extraAttrs[0] = 55;
        extraAttrs[1] = 6003;
        extraAttrs[2] = 7122;
        SignalInfo* signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
            CONSTRUCT_SIG_CODE(0x0d, 0x000f), 0, SIGNAL_EXTRA_ATTRS_COUNT, extraAttrs
        );

        E_ASSERT((signalInfo != nullptr));
        E_ASSERT((signalInfo->mSignalID == 0x000f));
        E_ASSERT((signalInfo->mSignalCategory == 0x0d));
        E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_13_OVERRIDE") == 0));
        E_ASSERT((signalInfo->mTimeout == 12000));

        E_ASSERT((signalInfo->mPermissions != nullptr));
        E_ASSERT((signalInfo->mSignalResources != nullptr));

        E_ASSERT((signalInfo->mPermissions->size() == 2));
        E_ASSERT((signalInfo->mSignalResources->size() == 2));

        E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
        E_ASSERT((signalInfo->mPermissions->at(1) == PERMISSION_SYSTEM));

        Resource* resource1 = signalInfo->mSignalResources->at(0);
        E_ASSERT((resource1->getResCode() == 0x00ff000a));
        E_ASSERT((resource1->getValuesCount() == 1));
        E_ASSERT((resource1->getValueAt(0) == 654));
        E_ASSERT((resource1->getResInfo() == 0x00000102));

        Resource* resource2 = signalInfo->mSignalResources->at(1);
        E_ASSERT((resource2->getResCode() == 0x00ff000b));
        E_ASSERT((resource2->getValuesCount() == 1));
        E_ASSERT((resource2->getValueAt(0) == 120));
        E_ASSERT((resource2->getResInfo() == 0));

        E_ASSERT((signalInfo->mHasExtraAttrs == true));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_FPS] == 55));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_HEIGHT] == 6003));
        E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_WIDTH] == 7122));
    }
})

URM_TEST(AppConfigParserTests, {
    ErrCode parsingStatus = RC_SUCCESS;
    RestuneParser configProcessor;

    std::string perAppConfPath = "/etc/urm/tests/configs/PerApp.yaml";
    parsingStatus = configProcessor.parsePerAppConfigs(perAppConfPath);

    E_ASSERT((AppConfigs::getInstance() != nullptr));
    E_ASSERT((parsingStatus == RC_SUCCESS));

    {
        AppConfig* appConfigInfo = AppConfigs::getInstance()->getAppConfig("chrome");
        E_ASSERT((appConfigInfo != nullptr));

        E_ASSERT((appConfigInfo->mAppName == "chrome"));
        E_ASSERT((appConfigInfo->mNumThreads == 2));

        E_ASSERT((appConfigInfo->mThreadNameList != nullptr));
        E_ASSERT((appConfigInfo->mCGroupIds != nullptr));

        E_ASSERT((appConfigInfo->mNumSignals == 1));
        E_ASSERT((appConfigInfo->mSignalCodes != nullptr));

        E_ASSERT((appConfigInfo->mSignalCodes[0] == 0x80034aab));
    }

    {
        AppConfig* appConfigInfo = AppConfigs::getInstance()->getAppConfig("vlc");
        E_ASSERT((appConfigInfo != nullptr));

        E_ASSERT((appConfigInfo->mAppName == "vlc"));
        E_ASSERT((appConfigInfo->mNumThreads == 0));

        E_ASSERT((appConfigInfo->mThreadNameList == nullptr));
        E_ASSERT((appConfigInfo->mCGroupIds == nullptr));

        E_ASSERT((appConfigInfo->mNumSignals == 1));
        E_ASSERT((appConfigInfo->mSignalCodes != nullptr));

        E_ASSERT((appConfigInfo->mSignalCodes[0] == 0x000d000c));
    }
})
