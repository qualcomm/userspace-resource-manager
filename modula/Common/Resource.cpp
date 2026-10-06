// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "Resource.h"

Resource::Resource(const Resource& resource) {
    this->mResValue.valueArr = nullptr;

    this->mResCode = resource.getResCode();
    this->mNumValues = resource.getValuesCount();
    if(this->mNumValues > 2) {
        this->mResValue.valueArr = new(std::nothrow) int32_t[this->mNumValues];
    }

    this->mResInfo = resource.getResInfo();
    this->mOptionalInfo = resource.getOptionalInfo();

    for(int32_t i = 0; i < this->mNumValues; i++) {
        if(RC_IS_NOTOK(this->setValueAt(i, resource.getValueAt(i)))) {
            return;
        }
    }
}

int32_t Resource::getCoreValue() const {
    return (int32_t)(this->mResInfo) & ((1 << 8) - 1);
}

int32_t Resource::getClusterValue() const {
    return (int32_t)(this->mResInfo >> 8) & ((1 << 8) - 1);
}

int32_t Resource::getResInfo() const {
    return this->mResInfo;
}

int32_t Resource::getOptionalInfo() const {
    return this->mOptionalInfo;
}

uint32_t Resource::getResCode() const {
    return this->mResCode;
}

int32_t Resource::getValuesCount() const {
    return this->mNumValues;
}

int32_t Resource::getValueAt(int32_t index) const {
    if(index < 0 || index >= this->mNumValues) return -1;
    if(this->mNumValues <= 2) return this->mResValue.values[index];
    return this->mResValue.valueArr[index];
}

uint8_t Resource::getResAction() const {
    return this->mResAction;
}

void Resource::setCoreValue(int32_t core) {
    this->mResInfo = (this->mResInfo ^ this->getCoreValue()) | core;
}

void Resource::setClusterValue(int32_t cluster) {
    this->mResInfo = (this->mResInfo ^ (this->getClusterValue() << 8)) | (cluster << 8);
}

void Resource::setResCode(uint32_t resCode) {
    this->mResCode = resCode;
}

void Resource::setResInfo(int32_t resInfo) {
    this->mResInfo = resInfo;
}

void Resource::setOptionalInfo(int32_t optionalInfo) {
    this->mOptionalInfo = optionalInfo;
}

void Resource::setNumValues(int32_t numValues) {
    this->mNumValues = numValues;
    if(this->mNumValues > 2) {
        this->mResValue.valueArr = new(std::nothrow) int32_t[this->mNumValues];
    }
}

void Resource::setResAction(uint8_t action) {
    this->mResAction = action;
}

ErrCode Resource::setValueAt(int32_t index, int32_t value) {
    if(index < 0 || index >= this->mNumValues) return RC_BAD_ARG;
    if(this->mNumValues > 2) {
        if(this->mResValue.valueArr == nullptr) {
            return RC_MEMORY_ALLOCATION_FAILURE;
        }
        this->mResValue.valueArr[index] = value;
    } else {
        this->mResValue.values[index] = value;
    }
    return RC_SUCCESS;
}

Resource::~Resource() {
    if(this->mNumValues > 2 && this->mResValue.valueArr != nullptr) {
        delete(this->mResValue.valueArr);
        this->mResValue.valueArr = nullptr;
    }
}
