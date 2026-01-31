// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


// ExtensionIntfTests.cpp
#include "TestUtils.h"
#include "ResourceRegistry.h"
#include "RestuneParser.h"
#include "Extensions.h"
#include "TestAggregator.h"

#include <string>
#include <cstdint>

#define MTEST_NO_MAIN
#include "../framework/mini.h"

using namespace mtest;

// Keep your RESTUNE registrations (paths as provided)
URM_REGISTER_CONFIG(RESOURCE_CONFIG,   "/etc/urm/tests/configs/ResourcesConfig.yaml")
URM_REGISTER_CONFIG(PROPERTIES_CONFIG, "/etc/urm/tests/configs/PropertiesConfig.yaml")
URM_REGISTER_CONFIG(SIGNALS_CONFIG,    "/etc/urm/tests/configs/SignalsConfig.yaml")
URM_REGISTER_CONFIG(TARGET_CONFIG,     "/etc/urm/tests/configs/TargetConfig.yaml")
URM_REGISTER_CONFIG(INIT_CONFIG,       "/etc/urm/tests/configs/InitConfig.yaml")

// Global flags/counters (same semantics as original)
static int8_t  funcCalled    = false;
static int32_t invokeCounter = 0;

// Callbacks
static void customApplier1(void* /*context*/) { funcCalled = true; }
static void customApplier2(void* /*context*/) { invokeCounter++; }
static void customTear1   (void* /*context*/) { funcCalled = true; }

// Register callbacks (same IDs)
URM_REGISTER_RES_APPLIER_CB(0x80ff0000, customApplier1)
URM_REGISTER_RES_TEAR_CB   (0x80ff0001, customTear1)
URM_REGISTER_RES_APPLIER_CB(0x80ff0002, customApplier2)

// --- Component-friendly initialization helpers ---

static bool file_exists(const std::string& path) {
    if (path.empty()) return false;
    if (auto* f = std::fopen(path.c_str(), "r")) { std::fclose(f); return true; }
    return false;
}

// Try to parse available configs to populate the registry; return true on success.
static bool TryParseAllConfigs() {
    RestuneParser configProcessor;

    const auto resPath  = Extensions::getResourceConfigFilePath();
    const auto propPath = Extensions::getPropertiesConfigFilePath();
    const auto sigPath  = Extensions::getSignalsConfigFilePath();
    const auto tgtPath  = Extensions::getTargetConfigFilePath();
    const auto initPath = Extensions::getInitConfigFilePath();

    // Require at least the resources file.
    if (!file_exists(resPath)) return false;

    try {
        // Correct single-argument signatures:
        configProcessor.parseResourceConfigs(resPath);
        if (file_exists(propPath)) configProcessor.parsePropertiesConfigs(propPath);
        if (file_exists(sigPath))  configProcessor.parseSignalConfigs(sigPath); // singular: Signal
        if (file_exists(tgtPath))  configProcessor.parseTargetConfigs(tgtPath);
        if (file_exists(initPath)) configProcessor.parseInitConfigs(initPath);

        // Apply plugin modifications after parsing.
        auto rr = ResourceRegistry::getInstance(); // shared_ptr<ResourceRegistry>
        rr->pluginModifications();
        return true;
    } catch (...) {
        // If parse throws, treat as unavailable.
        return false;
    }
}

// One-time initialization via fixture
struct ExtensionFixture : mtest::Fixture {
    static bool initialized;
    static bool parsed_ok;

    void setup(mtest::TestContext&) override {
        if (!initialized) {
            parsed_ok = TryParseAllConfigs();
            initialized = true;
        }
    }
    void teardown(mtest::TestContext&) override {}
};
bool ExtensionFixture::initialized = false;
bool ExtensionFixture::parsed_ok   = false;

// ---------------------------
// Suite: ExtensionIntfTests (tagged as "component-serial")
// ---------------------------

MT_TEST_F(Component, ModifiedResourceConfigPath, "extensionintf", ExtensionFixture) {
    MT_REQUIRE_EQ(ctx, Extensions::getResourceConfigFilePath(),
                  std::string("/etc/urm/tests/configs/ResourcesConfig.yaml"));
}
MT_TEST_F_END

MT_TEST_F(Component, ModifiedPropertiesConfigPath, "extensionintf", ExtensionFixture) {
    MT_REQUIRE_EQ(ctx, Extensions::getPropertiesConfigFilePath(),
                  std::string("/etc/urm/tests/configs/PropertiesConfig.yaml"));
}
MT_TEST_F_END

MT_TEST_F(Component, ModifiedSignalConfigPath, "extensionintf", ExtensionFixture) {
    MT_REQUIRE_EQ(ctx, Extensions::getSignalsConfigFilePath(),
                  std::string("/etc/urm/tests/configs/SignalsConfig.yaml"));
}
MT_TEST_F_END

MT_TEST_F(Component, ModifiedTargetConfigPath, "extensionintf", ExtensionFixture) {
    MT_REQUIRE_EQ(ctx, Extensions::getTargetConfigFilePath(),
                  std::string("/etc/urm/tests/configs/TargetConfig.yaml"));
}
MT_TEST_F_END

MT_TEST_F(Component, ModifiedInitConfigPath, "extensionintf", ExtensionFixture) {
    MT_REQUIRE_EQ(ctx, Extensions::getInitConfigFilePath(),
                  std::string("/etc/urm/tests/configs/InitConfig.yaml"));
}
MT_TEST_F_END

MT_TEST_F(Component, CustomResourceApplier1, "extensionintf", ExtensionFixture) {
    auto rr    = ResourceRegistry::getInstance();     // shared_ptr<ResourceRegistry>
    auto* info = rr->getResConf(0x80ff0000);
    if (!info) {
        MT_FAIL(ctx, "Resource 0x80ff0000 not found — ensure configs are available and parsed.");
    }

    funcCalled = false;
    MT_REQUIRE(ctx, info->mResourceApplierCallback != nullptr);
    info->mResourceApplierCallback(nullptr);
    MT_REQUIRE_EQ(ctx, funcCalled, true);
}
MT_TEST_F_END

MT_TEST_F(Component, CustomResourceApplier2, "extensionintf", ExtensionFixture) {
    auto rr    = ResourceRegistry::getInstance();
    auto* info = rr->getResConf(0x80ff0002);
    if (!info) {
        MT_FAIL(ctx, "Resource 0x80ff0002 not found — ensure configs are available and parsed.");
    }

    invokeCounter = 0;
    MT_REQUIRE(ctx, info->mResourceApplierCallback != nullptr);
    info->mResourceApplierCallback(nullptr);
    MT_REQUIRE_EQ(ctx, invokeCounter, 1);
}
MT_TEST_F_END

MT_TEST_F(Component, CustomResourceTear, "extensionintf", ExtensionFixture) {
    auto rr    = ResourceRegistry::getInstance();
    auto* info = rr->getResConf(0x80ff0001);
    if (!info) {
        MT_FAIL(ctx, "Resource 0x80ff0001 not found — ensure configs are available and parsed.");
    }

    funcCalled = false;
    MT_REQUIRE(ctx, info->mResourceTearCallback != nullptr);
    info->mResourceTearCallback(nullptr);
    MT_REQUIRE_EQ(ctx, funcCalled, true);
}
MT_TEST_F_END

// Note: No RunTests() / REGISTER_TEST() needed — mini.hpp auto-registers tests
// and provides main() unless compiled with -DMTEST_NO_MAIN.
