// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Message.h"

int8_t Message::getRequestType() const {
    return this->mReqType;
}

int64_t Message::getHandle() const {
    return this->mHandle;
}

int32_t Message::getClientPID() const {
    return this->mClientPID;
}

int32_t Message::getClientTID() const {
    return this->mClientTID;
}

int8_t Message::getPriority() const {
    return (int8_t)(this->mProperties & ((((int32_t) 1 << 8)) - 1));
}

int64_t Message::getDuration() const {
    return this->mDuration;
}

int32_t Message::getProperties() const {
    return this->mProperties;
}

int8_t Message::getProcessingModes() const {
    return (int8_t) ((this->mProperties >> 8) & ((((int32_t) 1 << 8)) - 1));
}

void Message::setRequestType(int8_t reqType) {
    this->mReqType = reqType;
}

void Message::setHandle(int64_t handle) {
    this->mHandle = handle;
}

void Message::setDuration(int64_t duration) {
    this->mDuration = duration;
}

void Message::setClientPID(int32_t clientPid) {
    this->mClientPID = clientPid;
}

void Message::setClientTID(int32_t clientTid) {
    this->mClientTID = clientTid;
}

void Message::setProperties(int32_t properties) {
    this->mProperties = properties;
}

void Message::setPriority(int8_t priority) {
    this->mProperties |= (int8_t) priority;
}

void Message::addProcessingMode(int8_t processingMode) {
    this->mProperties |= (((this->mProperties >> 8) | processingMode) << 8);
}

void Message::setBackgroundProcessing(int8_t backgroundProcessing) {
    this->mProperties |= ((int32_t) backgroundProcessing << 8);
}

void Message::setUntuneProcessingOrder(int8_t untuneProcessingOrder) {
    this->mProperties |= ((int32_t) untuneProcessingOrder << 16);
}
