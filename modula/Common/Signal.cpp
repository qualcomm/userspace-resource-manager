// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Signal.h"

Signal::Signal() {}

uint32_t Signal::getSignalCode() {
    return this->mSignalCode;
}

uint32_t Signal::getSignalType() {
    return this->mSignalType;
}

int32_t Signal::getNumArgs() {
    return this->mNumArgs;
}

const std::string Signal::getAppName() {
    return this->mAppName;
}

const std::string Signal::getScenario() {
    return this->mScenario;
}

std::vector<uint32_t>* Signal::getListArgs() {
    return this->mListArgs;
}

uint32_t Signal::getListArgAt(int32_t index) {
    return (*this->mListArgs)[index];
}

void Signal::setSignalCode(uint32_t signalCode) {
    this->mSignalCode = signalCode;
}

void Signal::setSignalType(uint32_t signalType) {
    this->mSignalType = signalType;
}

void Signal::setAppName(const std::string& appName) {
    this->mAppName = appName;
}

void Signal::setScenario(const std::string& scenario) {
    this->mScenario = scenario;
}

void Signal::setNumArgs(int32_t numArgs) {
    this->mNumArgs = numArgs;
}

void Signal::setList(std::vector<uint32_t>* listArgs) {
    this->mListArgs = listArgs;
}

ErrCode Signal::serialize(char* buf) {
    try {
        int8_t* ptr8 = (int8_t*)buf;
        ASSIGN_AND_INCR(ptr8, this->getRequestType());

        int32_t* ptr = (int32_t*)ptr8;
        ASSIGN_AND_INCR(ptr, this->getSignalCode());

        int64_t* ptr64 = (int64_t*)ptr;
        ASSIGN_AND_INCR(ptr64, this->getHandle());
        ASSIGN_AND_INCR(ptr64, this->getDuration());

        char* charPointer = (char*) ptr64;

        for(char ch: this->getAppName()) {
            ASSIGN_AND_INCR(charPointer, ch);
        }

        ASSIGN_AND_INCR(charPointer, '\0');

        for(char ch: this->getScenario()) {
            ASSIGN_AND_INCR(charPointer, ch);
        }

        ASSIGN_AND_INCR(charPointer, '\0');

        ptr = (int32_t*)charPointer;
        ASSIGN_AND_INCR(ptr, this->getNumArgs());
        ASSIGN_AND_INCR(ptr, this->getProperties());
        ASSIGN_AND_INCR(ptr, this->getClientPID());
        ASSIGN_AND_INCR(ptr, this->getClientTID());

        for(int32_t i = 0; i < this->getNumArgs(); i++) {
            uint32_t arg = this->getListArgAt(i);
            ASSIGN_AND_INCR(ptr, arg)
        }
    } catch(const std::bad_alloc& e) {
        return RC_REQUEST_PARSING_FAILED;
    } catch(const std::exception& e) {
        return RC_INVALID_VALUE;
    }

    return RC_SUCCESS;
}

ErrCode Signal::deserialize(char* buf) {
    try {
        int8_t* ptr8 = (int8_t*)buf;
        DEREF_AND_INCR(ptr8, int8_t);
        this->mReqType = DEREF_AND_INCR(ptr8, int8_t);

        int32_t* ptr = (int32_t*)ptr8;
        this->mSignalCode = DEREF_AND_INCR(ptr, int32_t);
        this->mSignalType = DEREF_AND_INCR(ptr, int32_t);

        int64_t* ptr64 = (int64_t*)ptr;
        this->mHandle = DEREF_AND_INCR(ptr64, int64_t);
        this->mDuration = DEREF_AND_INCR(ptr64, int64_t);

        char* charIterator = (char*)ptr64;
        this->mAppName = charIterator;

        while(*charIterator != '\0') {
            charIterator++;
        }
        charIterator++;

        this->mScenario = charIterator;

        while(*charIterator != '\0') {
            charIterator++;
        }
        charIterator++;

        ptr = (int32_t*)charIterator;
        this->mNumArgs = DEREF_AND_INCR(ptr, int32_t);
        this->mProperties = DEREF_AND_INCR(ptr, int32_t);
        this->mClientPID = DEREF_AND_INCR(ptr, int32_t);
        this->mClientTID = DEREF_AND_INCR(ptr, int32_t);

        this->mListArgs = MPLACED(std::vector<uint32_t>);
        this->mListArgs->resize(this->mNumArgs);

        for(int32_t i = 0; i < this->mNumArgs; i++) {
            (*this->mListArgs)[i] = DEREF_AND_INCR(ptr, uint32_t);
        }

    } catch(const std::invalid_argument& e) {
        TYPELOGV(REQUEST_PARSING_FAILURE, e.what());
        return RC_REQUEST_PARSING_FAILED;

    } catch(const std::bad_alloc& e) {
        TYPELOGV(REQUEST_MEMORY_ALLOCATION_FAILURE, e.what());
        return RC_MEMORY_POOL_BLOCK_RETRIEVAL_FAILURE;

    } catch(const std::exception& e) {
        LOGE("RESTUNE_SERVER",
             "Request Deserialization Failed with error: " + std::string(e.what()));
        return RC_REQUEST_DESERIALIZATION_FAILURE;
    }

    return RC_SUCCESS;
}

Signal::~Signal() {}

// Signal Utils
void Signal::cleanUpSignal(Signal* signal) {
    if(signal == nullptr) return;

    if(signal->mListArgs != nullptr) {
        FreeBlock<std::vector<uint32_t>>
                (static_cast<void*>(signal->mListArgs));
        signal->mListArgs = nullptr;
    }

    FreeBlock<Signal>(static_cast<void*>(signal));
}
