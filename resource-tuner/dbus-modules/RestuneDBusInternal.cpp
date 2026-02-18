// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "RestuneDBus.h"

#define SYSTEMD_DBUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_DBUS_PATH "/org/freedesktop/systemd1"
#define SYSTEMD_DBUS_IF "org.freedesktop.systemd1.Manager"

std::shared_ptr<RestuneSDBus> RestuneSDBus::restuneSDBusInstance = nullptr;

RestuneSDBus::RestuneSDBus() {
    this->mBus = nullptr;

    // Connect to the system bus
    int32_t rc;
    if((rc = sd_bus_default_system((sd_bus**)&this->mBus)) < 0) {
        TYPELOGV(SYSTEM_BUS_CONN_FAILED, strerror(-rc));
    }
}

ErrCode RestuneSDBus::startService(const std::string& unitName) {
    if(this->mBus == nullptr) return RC_MODULE_INIT_FAILURE;

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;

    int32_t rc = sd_bus_call_method(
        static_cast<sd_bus*>(this->mBus),
        SYSTEMD_DBUS_NAME,
        SYSTEMD_DBUS_PATH,
        SYSTEMD_DBUS_IF,
        "StartUnit",
        &error,
        &reply,
        "ss",
        unitName.c_str(),
        "replace"
    );

    if(rc < 0) {
        LOGE("RESTUNE_COCO_TABLE", "Failed to start irqbalanced: " + std::string(error.message));
        return RC_DBUS_COMM_FAIL;
    }

    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);

    return RC_SUCCESS;
}

ErrCode RestuneSDBus::stopService(const std::string& unitName) {
    if(this->mBus == nullptr) return RC_DBUS_COMM_FAIL;

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;

    int32_t rc = sd_bus_call_method(
        static_cast<sd_bus*>(this->mBus),
        SYSTEMD_DBUS_NAME,
        SYSTEMD_DBUS_PATH,
        SYSTEMD_DBUS_IF,
        "StopUnit",
        &error,
        &reply,
        "ss",
        unitName.c_str(),
        "replace"
    );

    if(rc < 0) {
        LOGE("RESTUNE_COCO_TABLE", "Failed to stop irqbalanced: " + std::string(error.message));
        return RC_DBUS_COMM_FAIL;
    }

    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);

    return RC_SUCCESS;
}

ErrCode RestuneSDBus::restartService(const std::string& unitName) {
    if(this->mBus == nullptr) return RC_DBUS_COMM_FAIL;

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;

    int32_t rc = sd_bus_call_method(
        static_cast<sd_bus*>(this->mBus),
        SYSTEMD_DBUS_NAME,
        SYSTEMD_DBUS_PATH,
        SYSTEMD_DBUS_IF,
        "RestartUnit",
        &error,
        &reply,
        "ss",
        unitName.c_str(),
        "replace"
    );

    if(rc < 0) {
        LOGE("RESTUNE_COCO_TABLE", "Failed to start irqbalanced: " + std::string(error.message));
    }

    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);

    return RC_SUCCESS;
}

void* RestuneSDBus::getBus() {
    return this->mBus;
}

RestuneSDBus::~RestuneSDBus() {
    if(this->mBus != nullptr) {
        sd_bus_unref((sd_bus*)this->mBus);
        this->mBus = nullptr;
    }
}
