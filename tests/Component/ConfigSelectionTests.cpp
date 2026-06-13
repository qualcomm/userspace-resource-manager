// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "URMTests.h"
#include "TestUtils.h"
#include "SignalRegistry.h"
#include "SignalRegistry.h"
#include "RestuneInternal.h"
#include "ResourceRegistry.h"

#define TEST_CLASS "COMPONENT"
#define TEST_SUBCAT "CONFIG_SELECTION"

URM_TEST(TestBestConfigSelection1, {
    std::shared_ptr<SignalRegistry> sigRegistry = SignalRegistry::getInstance();

    SignalInfo* signalInfo = nullptr;
    uint32_t* extraAttrs = new uint32_t[SIGNAL_EXTRA_ATTRS_COUNT];
    extraAttrs[0] = 60;
    extraAttrs[1] = 2160;
    extraAttrs[2] = 3840;

    signalInfo = sigRegistry->getSignalConfigByIdAndType(
        CONSTRUCT_SIG_CODE(0x0d, 0x000e), 0, SIGNAL_EXTRA_ATTRS_COUNT, extraAttrs
    );

    E_ASSERT((signalInfo != nullptr));
    E_ASSERT((signalInfo->mSignalID == 0x000e));
    E_ASSERT((signalInfo->mSignalCategory == 0x0d));
    E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_12_WEF_1") == 0));
    E_ASSERT((signalInfo->mTimeout == 8000));

    E_ASSERT((signalInfo->mHasExtraAttrs == true));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_FPS] == 120));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_HEIGHT] == 2160));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_WIDTH] == 3840));
})

URM_TEST(TestBestConfigSelection2, {
    std::shared_ptr<SignalRegistry> sigRegistry = SignalRegistry::getInstance();

    SignalInfo* signalInfo = nullptr;
    uint32_t* extraAttrs = new uint32_t[SIGNAL_EXTRA_ATTRS_COUNT];
    extraAttrs[0] = 30;
    extraAttrs[1] = 9999;
    extraAttrs[2] = 9999;

    signalInfo = sigRegistry->getSignalConfigByIdAndType(
        CONSTRUCT_SIG_CODE(0x0d, 0x000e), 0, SIGNAL_EXTRA_ATTRS_COUNT, extraAttrs
    );

    E_ASSERT((signalInfo != nullptr));
    E_ASSERT((signalInfo->mSignalID == 0x000e));
    E_ASSERT((signalInfo->mSignalCategory == 0x0d));
    E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_12_WEF_2") == 0));
    E_ASSERT((signalInfo->mTimeout == 8000));

    E_ASSERT((signalInfo->mHasExtraAttrs == true));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_FPS] == 30));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_HEIGHT] == 4320));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_WIDTH] == 7680));
})

URM_TEST(TestBestConfigSelection3, {
    std::shared_ptr<SignalRegistry> sigRegistry = SignalRegistry::getInstance();

    SignalInfo* signalInfo = nullptr;
    signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
        CONSTRUCT_SIG_CODE(0x0d, 0x0010), 0
    );

    E_ASSERT((signalInfo != nullptr));
    E_ASSERT((signalInfo->mSignalID == 0x0010));
    E_ASSERT((signalInfo->mSignalCategory == 0x0d));
    E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_14") == 0));
    E_ASSERT((signalInfo->mTimeout == 6000));

    E_ASSERT((signalInfo->mPermissions != nullptr));
    E_ASSERT((signalInfo->mDerivatives == nullptr));
    E_ASSERT((signalInfo->mSignalResources != nullptr));

    E_ASSERT((signalInfo->mPermissions->size() == 2));
    E_ASSERT((signalInfo->mSignalResources->size() == 1));

    E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
    E_ASSERT((signalInfo->mPermissions->at(1) == PERMISSION_SYSTEM));

    Resource* resource1 = signalInfo->mSignalResources->at(0);
    E_ASSERT((resource1->getResCode() == 0x00ff0009));
    E_ASSERT((resource1->getValuesCount() == 1));
    E_ASSERT((resource1->getValueAt(0) == 445));
    E_ASSERT((resource1->getResInfo() == 0x00000002));

    E_ASSERT((signalInfo->mHasExtraAttrs == true));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_FPS] == 40));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_HEIGHT] == 4200));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_WIDTH] == 5800));
})

URM_TEST(TestBestConfigSelection4, {
    std::shared_ptr<SignalRegistry> sigRegistry = SignalRegistry::getInstance();

    SignalInfo* signalInfo = nullptr;
    uint32_t* extraAttrs = new uint32_t[SIGNAL_EXTRA_ATTRS_COUNT];
    extraAttrs[0] = 120;
    extraAttrs[1] = 2160;
    extraAttrs[2] = 3840;
    signalInfo = SignalRegistry::getInstance()->getSignalConfigByIdAndType(
        CONSTRUCT_SIG_CODE(0x0d, 0x0011), 0, SIGNAL_EXTRA_ATTRS_COUNT, extraAttrs
    );

    E_ASSERT((signalInfo != nullptr));
    E_ASSERT((signalInfo->mSignalID == 0x0011));
    E_ASSERT((signalInfo->mSignalCategory == 0x0d));
    E_ASSERT((strcmp((const char*)signalInfo->mSignalName.data(), "TEST_SIGNAL_15_A") == 0));
    E_ASSERT((signalInfo->mTimeout == 8000));

    E_ASSERT((signalInfo->mPermissions != nullptr));
    E_ASSERT((signalInfo->mDerivatives == nullptr));
    E_ASSERT((signalInfo->mSignalResources != nullptr));

    E_ASSERT((signalInfo->mPermissions->size() == 2));
    E_ASSERT((signalInfo->mSignalResources->size() == 1));

    E_ASSERT((signalInfo->mPermissions->at(0) == PERMISSION_THIRD_PARTY));
    E_ASSERT((signalInfo->mPermissions->at(1) == PERMISSION_SYSTEM));

    Resource* resource1 = signalInfo->mSignalResources->at(0);
    E_ASSERT((resource1->getResCode() == 0x00ff0000));
    E_ASSERT((resource1->getValuesCount() == 1));
    E_ASSERT((resource1->getValueAt(0) == 556));
    E_ASSERT((resource1->getResInfo() == 0x00000000));

    E_ASSERT((signalInfo->mHasExtraAttrs == true));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_FPS] == 120));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_HEIGHT] == 2160));
    E_ASSERT((signalInfo->mExtraAttrs[SIGNAL_EXTRA_ATTR_WIDTH] == 3840));
})
