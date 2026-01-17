// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef AUX_ROUTINES_H
#define AUX_ROUTINES_H

#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <getopt.h>
#include <mutex>
#include <dirent.h>

#include "Logger.h"
#include "Request.h"
#include "Signal.h"
#include "UrmSettings.h"
#include "ClientEndpoint.h"

class AuxRoutines {
private:
    static std::mutex handleGenLock;

public:
    static std::string readFromFile(const std::string& fileName);
    static void writeToFile(const std::string& fileName, const std::string& value);
    static void deleteFile(const std::string& fileName);
    static void writeSysFsDefaults();
    static int8_t fileExists(const std::string& filePath);
    static int32_t createProcess();
    static std::string getMachineName();

    static int8_t isNumericString(const std::string& str);
	static pid_t fetchPid(const std::string& processName);
    static int32_t fetchComm(pid_t pid, std::string &comm);

    static int64_t generateUniqueHandle();
    static int64_t getCurrentTimeInMilliseconds();
};


// Following are some client-lib centric utilities
class FlatBuffEncoder {
private:
    char* mBuffer;
    void* mCurPtr;
    int32_t mRunningIndex;

public:
    FlatBuffEncoder();

    template <typename T>
    FlatBuffEncoder& append(T val) {
        if(this->mRunningIndex == -1 || this->mBuffer == nullptr) {
            return *this;
        }

        if(this->mRunningIndex + sizeof(T) < REQ_BUFFER_SIZE) {
            T* tPtr = (T*)(this->mCurPtr);
            try {
                ASSIGN_AND_INCR(tPtr, val);
                this->mRunningIndex += sizeof(T);
                this->mCurPtr = reinterpret_cast<char*>(tPtr);

            } catch(const std::exception& e) {
                this->mRunningIndex = -1;
            }
        } else {
            // Prevent further updates on the current buffer
            this->mRunningIndex = REQ_BUFFER_SIZE;
        }

        return *this;
    }

    FlatBuffEncoder appendString(const char* valStr);

    void setBuf(char* buffer);
    int8_t isBufSane();
};

class ConnectionManager {
private:
    std::shared_ptr<ClientEndpoint> connection;

public:
    ConnectionManager(std::shared_ptr<ClientEndpoint> connection) {
        this->connection = connection;
    }

    ~ConnectionManager() {
        if(this->connection != nullptr) {
            this->connection->closeConnection();
        }
    }
};

#endif
