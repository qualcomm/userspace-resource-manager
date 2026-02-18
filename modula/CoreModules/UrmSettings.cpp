// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "UrmSettings.h"

int32_t UrmSettings::serverOnlineStatus = false;
MetaConfigs UrmSettings::metaConfigs{};
TargetConfigs UrmSettings::targetConfigs{};

const std::string UrmSettings::mTargetConfDir = "/etc/urm/target/";

const std::string UrmSettings::mCommonResourcesPath =
                                    "/etc/urm/common/ResourcesConfig.yaml";
const std::string UrmSettings::mCustomResourcesPath =
                                    "/etc/urm/custom/ResourcesConfig.yaml";
const std::string UrmSettings::mDevIndexedResourcesPath =
                                    "/etc/urm/target/ResourcesConfig.yaml";

const std::string UrmSettings::mCommonSignalsPath =
                                    "/etc/urm/common/SignalsConfig.yaml";
const std::string UrmSettings::mCustomSignalsPath =
                                    "/etc/urm/custom/SignalsConfig.yaml";
const std::string UrmSettings::mDevIndexedSignalsPath =
                                    "/etc/urm/target/SignalsConfig.yaml";

const std::string UrmSettings::mCommonInitPath =
                                    "/etc/urm/common/InitConfig.yaml";
const std::string UrmSettings::mCustomInitPath =
                                    "/etc/urm/custom/InitConfig.yaml";
const std::string UrmSettings::mDevIndexedInitPath =
                                    "/etc/urm/target/InitConfig.yaml";

const std::string UrmSettings::mCommonPropertiesPath =
                                    "/etc/urm/common/PropertiesConfig.yaml";
const std::string UrmSettings::mCustomPropertiesPath =
                                    "/etc/urm/custom/PropertiesConfig.yaml";
const std::string UrmSettings::mDevIndexedPropertiesPath =
                                    "/etc/urm/target/PropertiesConfig.yaml";

const std::string UrmSettings::mCustomTargetPath =
                                    "/etc/urm/custom/TargetConfig.yaml";
const std::string UrmSettings::mDevIndexedTargetPath =
                                    "/etc/urm/target/TargetConfig.yaml";

const std::string UrmSettings::mCustomExtFeaturesPath =
                                    "/etc/urm/custom/ExtFeaturesConfig.yaml";
const std::string UrmSettings::mDevIndexedExtFeatPath =
                                    "/etc/urm/target/ExtFeaturesConfig.yaml";

const std::string UrmSettings::mCustomAppConfigPath =
                                    "/etc/urm/custom/PerApp.yaml";
const std::string UrmSettings::mDevIndexedAppPath =
                                    "/etc/urm/target/PerApp.yaml";

const std::string UrmSettings::mExtensionPluginsLibPath =
                                    "/usr/lib/urm/";

const std::string UrmSettings::mDeviceNamePath =
                                    "/sys/devices/soc0/machine";
const std::string UrmSettings::mBaseCGroupPath =
                                    "/sys/fs/cgroup/";
const std::string UrmSettings::focusedCgroup =
                                    "focused.slice";

const std::string UrmSettings::mPersistenceFile =
                                    "/etc/urm/data/resource_original_values.txt";

int32_t UrmSettings::isServerOnline() {
    return serverOnlineStatus;
}

void UrmSettings::setServerOnlineStatus(int32_t isOnline) {
    serverOnlineStatus = isOnline;
}
