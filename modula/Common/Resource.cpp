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

ErrCode Resource::truncate(int32_t valueCount) {
    // Cannot truncate to a negative size and cannot extend the list.
    if(valueCount < 0 || valueCount > this->mNumValues) {
        return RC_BAD_ARG;
    }

    if(valueCount == this->mNumValues) {
        return RC_SUCCESS;
    }

    // If mNumValues <= 2, there is no heap allocation. Return Success
    if(this->mNumValues <= 2) {
        this->mNumValues = valueCount;
        return RC_SUCCESS;
    }

    int32_t* oldValues = this->mResValue.valueArr;

    // Case 1:
    // The truncated Resource still needs heap storage.
    if(valueCount > 2) {
        int32_t* newValues = new(std::nothrow) int32_t[valueCount];

        // Allocation may fail because std::nothrow is used.
        // If it fails, do not modify the existing Resource.
        // The caller can reject the request safely.
        if(newValues == nullptr) {
            return RC_MEMORY_ALLOCATION_FAILURE;
        }

        // Copy only the values we want to keep.
        for(int32_t i = 0; i < valueCount; i++) {
            newValues[i] = oldValues[i];
        }

        // oldValues was allocated using new[], so it must be freed using delete[].
        delete[] oldValues;

        // Store the new smaller heap array and update the count.
        this->mResValue.valueArr = newValues;
        this->mNumValues = valueCount;

        return RC_SUCCESS;
    }

    // Case 2:
    // The truncated Resource now fits into inline storage.

    int32_t inlineValues[2] = {0, 0};

    for(int32_t i = 0; i < valueCount; i++) {
        inlineValues[i] = oldValues[i];
    }

    // Free the old heap array before switching to inline storage.
    delete[] oldValues;

    this->mNumValues = valueCount;

    // Copy the preserved values into inline storage.
    for(int32_t i = 0; i < valueCount; i++) {
        this->mResValue.values[i] = inlineValues[i];
    }

    return RC_SUCCESS;
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
        delete[] this->mResValue.valueArr;
        this->mResValue.valueArr = nullptr;
    }
}
