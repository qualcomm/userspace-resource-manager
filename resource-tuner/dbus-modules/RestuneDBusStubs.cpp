// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "RestuneDBus.h"

std::shared_ptr<RestuneSDBus> RestuneSDBus::restuneSDBusInstance = nullptr;

RestuneSDBus::RestuneSDBus() {}

ErrCode RestuneSDBus::startService(const std::string& unitName) {
    (void)unitName;
    LOGE("URM_DBUS_DEP_MODULES", "dBus support not found, skipping.");
    return RC_SUCCESS;
}

ErrCode RestuneSDBus::stopService(const std::string& unitName) {
    (void)unitName;
    LOGE("URM_DBUS_DEP_MODULES", "dBus support not found, skipping.");
    return RC_SUCCESS;
}

ErrCode RestuneSDBus::restartService(const std::string& unitName) {
    (void)unitName;
    LOGE("URM_DBUS_DEP_MODULES", "dBus support not found, skipping.");
    return RC_SUCCESS;
}

void* RestuneSDBus::getBus() {
    return nullptr;
}

RestuneSDBus::~RestuneSDBus() {}
