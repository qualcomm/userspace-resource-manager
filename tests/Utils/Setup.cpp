// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Extensions.h"

__attribute__((constructor))
void registerWithResourceTuner() {
    RESTUNE_REGISTER_CONFIG(RESOURCE_CONFIG, "/etc/urm/tests/configs/ResourcesConfig.yaml")
    RESTUNE_REGISTER_CONFIG(PROPERTIES_CONFIG, "/etc/urm/tests/configs/PropertiesConfig.yaml")
    RESTUNE_REGISTER_CONFIG(SIGNALS_CONFIG, "/etc/urm/tests/configs/SignalsConfig.yaml")
    RESTUNE_REGISTER_CONFIG(TARGET_CONFIG, "/etc/urm/tests/configs/TargetConfig.yaml")
    RESTUNE_REGISTER_CONFIG(INIT_CONFIG, "/etc/urm/tests/configs/InitConfig.yaml")
}
