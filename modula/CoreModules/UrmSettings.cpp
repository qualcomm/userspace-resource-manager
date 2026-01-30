// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "UrmSettings.h"

int32_t UrmSettings::serverOnlineStatus = false;
MetaConfigs UrmSettings::metaConfigs{};
TargetConfigs UrmSettings::targetConfigs{};

const std::string UrmSettings::mCommonResourceFilePath =
                                    "/etc/urm/common/ResourcesConfig.yaml";
const std::string UrmSettings::mCustomResourceFilePath =
                                    "/etc/urm/custom/ResourcesConfig.yaml";

const std::string UrmSettings::mCommonSignalFilePath =
                                    "/etc/urm/common/SignalsConfig.yaml";
const std::string UrmSettings::mCustomSignalFilePath =
                                    "/etc/urm/custom/SignalsConfig.yaml";

const std::string UrmSettings::mCommonInitConfigFilePath =
                                    "/etc/urm/common/InitConfig.yaml";
const std::string UrmSettings::mCustomInitConfigFilePath =
                                    "/etc/urm/custom/InitConfig.yaml";

const std::string UrmSettings::mCommonPropertiesFilePath =
                                    "/etc/urm/common/PropertiesConfig.yaml";
const std::string UrmSettings::mCustomPropertiesFilePath =
                                    "/etc/urm/custom/PropertiesConfig.yaml";

const std::string UrmSettings::mCustomTargetFilePath =
                                    "/etc/urm/custom/TargetConfig.yaml";

const std::string UrmSettings::mCustomExtFeaturesFilePath =
                                    "/etc/urm/custom/ExtFeaturesConfig.yaml";

const std::string UrmSettings::mCustomAppConfigFilePath =
                                    "/etc/urm/custom/PerApp.yaml";

const std::string UrmSettings::mExtensionPluginsLibPath =
                                    "/etc/urm/extensions";

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
