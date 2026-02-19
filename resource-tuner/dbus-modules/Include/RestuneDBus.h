// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef RESTUNE_DBUS_INTERNAL_H
#define RESTUNE_DBUS_INTERNAL_H

#include <memory>

#include "Logger.h"
#include "ErrCodes.h"

class RestuneSDBus {
private:
    static std::shared_ptr<RestuneSDBus> restuneSDBusInstance;
    void* mBus;

    RestuneSDBus();

public:
    ~RestuneSDBus();

    ErrCode stopService(const std::string& unitName);
    ErrCode startService(const std::string& unitName);
    ErrCode restartService(const std::string& unitName);

    // For custom use-cases, like StateDetector while still keeping
    // a single open connection to system-bus.
    void* getBus();

    static std::shared_ptr<RestuneSDBus> getInstance() {
        if(restuneSDBusInstance == nullptr) {
            restuneSDBusInstance = std::shared_ptr<RestuneSDBus>(new RestuneSDBus());
        }
        return restuneSDBusInstance;
    }
};

#endif
