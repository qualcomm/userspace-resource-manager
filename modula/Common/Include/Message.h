// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef RESOURCE_TUNER_MESSAGE_H
#define RESOURCE_TUNER_MESSAGE_H

#include <cstdint>

/**
* @brief Base-Type for Request and Signal classes.
*/
class Message {
protected:
    int64_t mHandle; //!< The unique generated handle for the request.
    int64_t mDuration; //!< Duration. -1 means infinite duration.
    int32_t mProperties; //!< Request Properties, includes Priority and Background Processing Status.
    int32_t mClientPID; //!< Process ID of the client making the request.
    int32_t mClientTID; //!< Thread ID of the client making the request.
    int8_t mReqType; //!< Type of the request. Possible values: TUNE, UNTUNE, RETUNE, TUNESIGNAL, FREESIGNAL.

public:
    Message() : mProperties(0) {}

    int8_t getRequestType() const;
    int64_t getDuration() const;
    int32_t getClientPID() const;
    int32_t getClientTID() const;
    int64_t getHandle() const;
    int8_t getPriority() const;
    int8_t getProcessingModes() const;
    int32_t getProperties() const;

    void setRequestType(int8_t reqType);
    void setDuration(int64_t duration);
    void setClientPID(int32_t clientPID);
    void setClientTID(int32_t clientTID);
    void setProperties(int32_t properties);
    void setPriority(int8_t priority);
    void addProcessingMode(int8_t processingMode);
    void setUntuneProcessingOrder(int8_t untuneProcessingOrder);
    void setHandle(int64_t handle);
    void setBackgroundProcessing(int8_t backgroundProcessing);

    virtual ~Message() {}
};

#endif
