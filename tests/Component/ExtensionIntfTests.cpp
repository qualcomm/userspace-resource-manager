// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "TestUtils.h"
#include "ResourceRegistry.h"
#include "RestuneParser.h"
#include "Extensions.h"
#include "URMTests.h"

#define TEST_CLASS "COMPONENT"

URM_REGISTER_CONFIG(RESOURCE_CONFIG, "/etc/urm/tests/configs/ResourcesConfig.yaml")
URM_REGISTER_CONFIG(PROPERTIES_CONFIG, "/etc/urm/tests/configs/PropertiesConfig.yaml")
URM_REGISTER_CONFIG(SIGNALS_CONFIG, "/etc/urm/tests/configs/SignalsConfig.yaml")
URM_REGISTER_CONFIG(TARGET_CONFIG, "/etc/urm/tests/configs/TargetConfig.yaml")
URM_REGISTER_CONFIG(INIT_CONFIG, "/etc/urm/tests/configs/InitConfig.yaml")

static int8_t funcCalled = false;
static int32_t invokeCounter = 0;

static void customApplier1(void* context) {
    (void)context;
    funcCalled = true;
}

static void customApplier2(void* context) {
    (void)context;
    invokeCounter++;
}

static void customTear1(void* context) {
    (void)context;
    funcCalled = true;
}

URM_REGISTER_RES_APPLIER_CB(0x00ff0000, customApplier1)
URM_REGISTER_RES_TEAR_CB(0x00ff0001, customTear1)
URM_REGISTER_RES_APPLIER_CB(0x00ff0002, customApplier2)

static void Init() {
    static int8_t initDone = false;
    if(!initDone) {
        initDone = true;

        RestuneParser configProcessor;
        configProcessor.parseResourceConfigs(Extensions::getResourceConfigFilePath());
        ResourceRegistry::getInstance()->pluginModifications();
    }
}

URM_TEST(TestExtensionIntfModifiedResourceConfigPath, {
    Init();
    E_ASSERT((Extensions::getResourceConfigFilePath() == "/etc/urm/tests/configs/ResourcesConfig.yaml"));
})

URM_TEST(TestExtensionIntfModifiedPropertiesConfigPath, {
    Init();
    E_ASSERT((Extensions::getPropertiesConfigFilePath() == "/etc/urm/tests/configs/PropertiesConfig.yaml"));
})

URM_TEST(TestExtensionIntfModifiedSignalConfigPath, {
    Init();
    E_ASSERT((Extensions::getSignalsConfigFilePath() == "/etc/urm/tests/configs/SignalsConfig.yaml"));
})

URM_TEST(TestExtensionIntfModifiedTargetConfigPath, {
    Init();
    E_ASSERT((Extensions::getTargetConfigFilePath() == "/etc/urm/tests/configs/TargetConfig.yaml"));
})

URM_TEST(TestExtensionIntfModifiedInitConfigPath, {
    Init();
    E_ASSERT((Extensions::getInitConfigFilePath() == "/etc/urm/tests/configs/InitConfig.yaml"));
})

URM_TEST(TestExtensionIntfCustomResourceApplier1, {
    Init();
    ResConfInfo* info = ResourceRegistry::getInstance()->getResConf(0x00ff0000);
    E_ASSERT((info != nullptr));
    funcCalled = false;
    E_ASSERT((info->mResourceApplierCallback != nullptr));
    info->mResourceApplierCallback(nullptr);
    E_ASSERT((funcCalled == true));
})

URM_TEST(TestExtensionIntfCustomResourceApplier2, {
    Init();
    ResConfInfo* info = ResourceRegistry::getInstance()->getResConf(0x00ff0002);
    E_ASSERT((info != nullptr));
    E_ASSERT((info->mResourceApplierCallback != nullptr));
    info->mResourceApplierCallback(nullptr);
    E_ASSERT((invokeCounter == 1));
})

URM_TEST(TestExtensionIntfCustomResourceTear, {
    Init();
    ResConfInfo* info = ResourceRegistry::getInstance()->getResConf(0x00ff0001);
    E_ASSERT((info != nullptr));
    funcCalled = false;
    E_ASSERT((info->mResourceTearCallback != nullptr));
    info->mResourceTearCallback(nullptr);
    E_ASSERT((funcCalled == true));
})
