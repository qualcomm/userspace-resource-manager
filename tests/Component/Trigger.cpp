// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "URMTests.h"
#include "Extensions.h"

#define TEST_CLASS "COMPONENT"

URM_REGISTER_CONFIG(RESOURCE_CONFIG, "/etc/urm/tests/configs/ResourcesConfig.yaml")
URM_REGISTER_CONFIG(PROPERTIES_CONFIG, "/etc/urm/tests/configs/PropertiesConfig.yaml")
URM_REGISTER_CONFIG(SIGNALS_CONFIG, "/etc/urm/tests/configs/SignalsConfig.yaml")
URM_REGISTER_CONFIG(TARGET_CONFIG, "/etc/urm/tests/configs/TargetConfig.yaml")
URM_REGISTER_CONFIG(INIT_CONFIG, "/etc/urm/tests/configs/InitConfig.yaml")

REGISTER_AND_TRIGGER_SUITE(TEST_CLASS)
