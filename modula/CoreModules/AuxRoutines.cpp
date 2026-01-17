// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "AuxRoutines.h"

std::mutex AuxRoutines::handleGenLock {};

std::string AuxRoutines::readFromFile(const std::string& fileName) {
    if(fileName.length() == 0) return "";

    std::ifstream fileStream(fileName, std::ios::in);
    std::string value = "";

    if(!fileStream.is_open()) {
        LOGW("URM_AUX_ROUTINE", "Failed to open file: " + fileName + " Error: " + strerror(errno));
        return "";
    }

    if(!getline(fileStream, value)) {
        LOGW("URM_AUX_ROUTINE", "Failed to read from file: " + fileName + " Error: " + strerror(errno));
        return "";
    }

    fileStream.close();
    return value;
}

void AuxRoutines::writeToFile(const std::string& fileName, const std::string& value) {
    if(fileName.length() == 0) return;

    std::ofstream fileStream(fileName, std::ios::out | std::ios::trunc);
    if(!fileStream.is_open()) {
        LOGE("URM_AUX_ROUTINE", "Failed to open file: " + fileName + " Error: " + strerror(errno));
        return;
    }

    fileStream<<value;

    if(fileStream.fail()) {
        LOGE("URM_AUX_ROUTINE", "Failed to write to file: "+ fileName + " Error: " + strerror(errno));
    }

    fileStream.flush();
    fileStream.close();
}

void AuxRoutines::writeSysFsDefaults() {
    // Write Defaults
    std::ifstream file;

    file.open(UrmSettings::mPersistenceFile);
    if(!file.is_open()) {
        LOGE("RESTUNE_SERVER_INIT",
             "Failed to open sysfs original values file: " + UrmSettings::mPersistenceFile);
        return;
    }

    std::string line;
    while(std::getline(file, line)) {
        std::stringstream lineStream(line);
        std::string token;

        int8_t index = 0;
        std::string sysfsNodePath = "";
        int32_t sysfsNodeDefaultValue = -1;

        while(std::getline(lineStream, token, ',')) {
            if(index == 0) {
                sysfsNodePath = token;
            } else if(index == 1) {
                try {
                    sysfsNodeDefaultValue = std::stoi(token);
                } catch(const std::exception& e) {}
            }
            index++;
        }

        if(sysfsNodePath.length() > 0 && sysfsNodeDefaultValue != -1) {
            AuxRoutines::writeToFile(sysfsNodePath, std::to_string(sysfsNodeDefaultValue));
        }
    }
}

void AuxRoutines::deleteFile(const std::string& fileName) {
    remove(fileName.c_str());
}

int8_t AuxRoutines::fileExists(const std::string& filePath) {
    return access(filePath.c_str(), F_OK) == 0;
}

int32_t AuxRoutines::createProcess() {
    return fork();
}

std::string AuxRoutines::getMachineName() {
    return AuxRoutines::readFromFile(UrmSettings::mDeviceNamePath);
}

// Helper to check if a string contains only digits
int8_t AuxRoutines::isNumericString(const std::string& str) {
    return std::all_of(str.begin(), str.end(), ::isdigit);
}

// Function to get the first matching PID for a given process name
pid_t AuxRoutines::fetchPid(const std::string& process_name) {
    DIR* proc_dir = opendir("/proc");
    if(proc_dir == nullptr) {
        TYPELOGV(ERRNO_LOG, "opendir", strerror(errno));
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type == DT_DIR && isNumericString(entry->d_name)) {
            std::string pid_str = entry->d_name;
            std::string comm_path = COMM_S(pid_str);
            std::ifstream comm_file(comm_path);
            std::string comm;
            if (comm_file) {
                std::getline(comm_file, comm);
                if (comm.find(process_name) != std::string::npos) {
                    closedir(proc_dir);
                    return static_cast<pid_t>(std::stoi(pid_str));
                }
            }
        }
    }

    closedir(proc_dir);
    return -1; // Not found
}

int32_t AuxRoutines::fetchComm(pid_t pid, std::string &comm) {
    std::string proc_path = "/proc/" + std::to_string(pid);
    if(!AuxRoutines::fileExists(proc_path)) {
        LOGD("URM_AUX_ROUTINE", "Process %d has exited." + std::to_string(pid));
        return -1;
    }

    std::string comm_path = COMM(pid);
    std::ifstream comm_file(comm_path);
    if (comm_file.is_open()) {
        std::getline(comm_file, comm);
        // Trim
        size_t first = comm.find_first_not_of(" \t\n\r");
        if (first != std::string::npos) {
            size_t last = comm.find_last_not_of(" \t\n\r");
            comm = comm.substr(first, (last - first + 1));
        }
    }
    return 0;
}

FlatBuffEncoder::FlatBuffEncoder() {
    this->mBuffer = nullptr;
    this->mCurPtr = nullptr;
    this->mRunningIndex = 0;
}

void FlatBuffEncoder::setBuf(char* buffer) {
    this->mBuffer = buffer;
    this->mCurPtr = buffer;
    this->mRunningIndex = 0;
}

FlatBuffEncoder FlatBuffEncoder::appendString(const char* valStr) {
    if(this->mRunningIndex == -1 || this->mBuffer == nullptr) {
        return *this;
    }

    const char* charIterator = valStr;
    char* charPointer = reinterpret_cast<char*>(this->mCurPtr);

    while(*charIterator != '\0') {
        if(this->mRunningIndex != -1 && this->mRunningIndex + 1 < REQ_BUFFER_SIZE) {
            try {
                ASSIGN_AND_INCR(charPointer, *charIterator);
                this->mRunningIndex++;
                this->mCurPtr = reinterpret_cast<char*>(charPointer);

            } catch(const std::exception& e) {
                this->mRunningIndex = -1;
                break;
            }
        } else {
            // Prevent further updates on the current buffer
            this->mRunningIndex = REQ_BUFFER_SIZE;
            break;
        }

        charIterator++;
    }

    if(this->mRunningIndex >= 0 && this->mRunningIndex < REQ_BUFFER_SIZE) {
        return this->append<char>('\0');
    }

    return *this;
}

int8_t FlatBuffEncoder::isBufSane() {
    if(this->mRunningIndex >= REQ_BUFFER_SIZE || this->mBuffer == nullptr) {
        return false;
    }
    return true;
}

int64_t AuxRoutines::generateUniqueHandle() {
    const std::lock_guard<std::mutex> lock(handleGenLock);

    static int64_t handleGenerator = 0;
    OperationStatus opStatus;
    int64_t nextHandle = Add(handleGenerator, (int64_t)1, opStatus);
    if(opStatus == SUCCESS) {
        handleGenerator = nextHandle;
        return nextHandle;
    }

    return -1;
}

int64_t AuxRoutines::getCurrentTimeInMilliseconds() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}
