// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <thread>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "Logger.h"
#include "ComponentRegistry.h"
#include "UrmSettings.h"
#include "RestuneInternal.h"

#define DBUS_SIGNAL_SENDER "org.freedesktop.login1"
#define DBUS_SIGNAL_SENDER_PATH "/org/freedesktop/login1"
#define DBUS_SIGNAL_INTERFACE "org.freedesktop.login1.Manager"
#define DBUS_SIGNAL_NAME "PrepareForSleep"

static sd_bus* bus = nullptr;
static sd_bus_slot* slot = nullptr;
static sd_event* event = nullptr;

static std::thread eventTrackerThread;

static void cleanup() {
    // Cleanup
    if(event != nullptr) {
        sd_event_unref(event);
    }

    if(slot != nullptr) {
        sd_bus_slot_unref(slot);
    }

    if(bus != nullptr) {
        sd_bus_unref(bus);
    }

    event = nullptr;
    slot = nullptr;
    bus = nullptr;
}

static int32_t onSdBusMessageReceived(sd_bus_message* message,
                                      void *userdata,
                                      sd_bus_error* retError) {
    (void)userdata;
    (void)retError;
    int8_t sleepStatus;
    LOGI("RESTUNE_MODE_DETECTION", "sd-bus signal received");

    if(sd_bus_message_read(message, "b", &sleepStatus) < 0) {
        LOGE("RESTUNE_MODE_DETECTION", "Failed to parse Signal Parameter");
        return 0;
    }

    std::shared_ptr<RequestManager> requestManager = RequestManager::getInstance();
    if(sleepStatus) {
        // System is suspending
        LOGI("RESTUNE_MODE_DETECTION", "System is suspending");
        if(UrmSettings::targetConfigs.currMode & MODE_RESUME) {
            // Toggle to Display Off
            UrmSettings::targetConfigs.currMode &= ~MODE_RESUME;
            UrmSettings::targetConfigs.currMode |= MODE_SUSPEND;

            // First drain out the CocoTable, and move Requests to the Pending List
            // (the ones which cannot be processed in Background)
            requestManager->moveToPendingList();
        }
    } else {
        // System has resumed
        LOGI("RESTUNE_MODE_DETECTION", "System is resuming");
        if(UrmSettings::targetConfigs.currMode & MODE_SUSPEND) {
            // Toggle to Display On
            UrmSettings::targetConfigs.currMode &= ~MODE_SUSPEND;
            UrmSettings::targetConfigs.currMode |= MODE_RESUME;

            // Add all the Requests from the Pending List into the Active List
            std::vector<Request*> pendingRequests = requestManager->getPendingList();
            for(Request* request: pendingRequests) {
                submitResProvisionRequest(request, false);
            }
            requestManager->clearPending();
        }
    }

    return 0;
}

static int32_t eventLoopTerminator(sd_event_source *s,
                                   const struct signalfd_siginfo *si,
                                   void *userdata) {
    (void)s;
    (void)si;
    (void)userdata;
    sd_event_exit(event, 0);
    return 0;
}

static void initHelper() {
    // Connect to the system bus
    if(sd_bus_default_system(&bus) < 0) {
        LOGE("RESTUNE_DISPLAY_AWARE_OPS", "Failed to establish connection with system bus");
        cleanup();
        return;
    }

    // Create the Event Loop object
    if(sd_event_default(&event) < 0) {
        LOGE("RESTUNE_DISPLAY_AWARE_OPS", "Failed to create event-loop");
        cleanup();
        return;
    }

    // Subscribe to D-Bus signal (PrepareForSleep)
    if(sd_bus_match_signal(bus,
                           &slot,
                           nullptr,
                           DBUS_SIGNAL_SENDER_PATH,
                           DBUS_SIGNAL_INTERFACE,
                           DBUS_SIGNAL_NAME,
                           onSdBusMessageReceived,
                           nullptr) < 0) {
        LOGE("RESTUNE_DISPLAY_AWARE_OPS", "Failed to subscribe to PrepareForSleep (D-Bus) signal");
        cleanup();
        return;
    }

    // Listen for D-Bus events
    if(sd_bus_attach_event(bus, event, 0) < 0) {
        LOGE("RESTUNE_DISPLAY_AWARE_OPS", "Failed to start event-loop");
        cleanup();
        return;
    }

    // Start the Event Loop
    if(sd_event_loop(event) < 0) {
        LOGE("RESTUNE_DISPLAY_AWARE_OPS", "Failed to start event-loop");
        cleanup();
        return;
    }

    cleanup();
}

// Register Module's Callback functions
static ErrCode init(void* arg) {
    (void)arg;
    try {
        eventTrackerThread = std::thread(initHelper);
    } catch(const std::system_error& e) {
        TYPELOGV(SYSTEM_THREAD_CREATION_FAILURE, "State_Optimizer", e.what());
        return RC_MODULE_INIT_FAILURE;
    }

    return RC_SUCCESS;
}

static ErrCode tear(void* arg) {
    (void)arg;
    if(eventTrackerThread.joinable()) {
        eventTrackerThread.join();
    }

    return RC_SUCCESS;
}
