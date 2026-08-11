// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Extensions.h"

__attribute__((constructor))
void registerWithResourceTuner() {
    URM_REGISTER_CONFIG(RESOURCE_CONFIG, "/usr/share/urm/tests/configs/ResourcesConfig.yaml")
    URM_REGISTER_CONFIG(PROPERTIES_CONFIG, "/usr/share/urm/tests/configs/PropertiesConfig.yaml")
    URM_REGISTER_CONFIG(SIGNALS_CONFIG, "/usr/share/urm/tests/configs/SignalsConfig.yaml")
    URM_REGISTER_CONFIG(TARGET_CONFIG, "/usr/share/urm/tests/configs/TargetConfig.yaml")
    URM_REGISTER_CONFIG(INIT_CONFIG, "/usr/share/urm/tests/configs/InitConfig.yaml")
    URM_REGISTER_CONFIG(APP_CONFIG, "/usr/share/urm/tests/configs/PerApp.yaml")
}
