// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <string>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <memory>
#include <mutex>

#include "Helpers.h"

class PostProcessingBlock {
private:
    static std::once_flag mInitFlag;
    static std::unique_ptr<PostProcessingBlock> mInstance;

private:
    inline void    SanitizeNulls(char *buf, int32_t len);
    inline int32_t ReadFirstLine(const std::string& filePath, std::string &line);
    inline void    to_lower(std::string &s);
    int32_t        countThreadsWithName(pid_t pid, const std::string& commSub);
    int32_t        fetchUsecaseDetails(int32_t pid, char *buf, uint32_t &sigId, uint32_t &sigType, uint32_t** extraArgs);
    int32_t        countEncoders(const char* buffer, const char* encoderStr);
    std::string    extractSourceName(const char* buffer, const char* namePrefix, const char* defaultName);
    uint32_t       extractFrameRate(const char* buffer, const char* frameRatePrefix);
    uint32_t       extractHeight(const char* buffer);
    uint32_t       extractWidth(const char* buffer);

    uint32_t       calculateEncoderSigType(int32_t count);
    uint32_t       calculateDecoderSigType(int32_t threadCount);

    PostProcessingBlock() = default;
    PostProcessingBlock(const PostProcessingBlock&) = delete;
    PostProcessingBlock& operator=(const PostProcessingBlock&) = delete;

public:
    static PostProcessingBlock& getInstance() {
        std::call_once(mInitFlag, [] {
            mInstance.reset(new PostProcessingBlock());
        });
        return *mInstance;
    }

    ~PostProcessingBlock() = default;
    void PostProcess(pid_t pid, uint32_t &sigId, uint32_t &sigType, uint32_t** extraArgs);
};

inline void PostProcessingBlock::SanitizeNulls(char *buf, int32_t len) {
    /* /proc/<pid>/cmdline contains null charaters instead of spaces
     * sanitize those null characters with spaces such that char*
     * can be treaded till line end.
     */
    for(int32_t i = 0; i < len; i++) {
        if (buf[i] == '\0') {
            buf[i] = ' ';
        }
    }
}

inline int32_t PostProcessingBlock::ReadFirstLine(const std::string& filePath, std::string &line) {
    if(filePath.length() == 0) return 0;

    std::ifstream fileStream(filePath, std::ios::in);

    if(!fileStream.is_open()) {
        return 0;
    }

    if(!getline(fileStream, line)) {
        return 0;
    }

    fileStream.close();
    return line.size();
}

// Lowercase utility
inline void PostProcessingBlock::to_lower(std::string &s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return;
}

// Count threads under /proc/<pid>/task whose names contain `substring`.
int32_t PostProcessingBlock::countThreadsWithName(pid_t pid, const std::string& commSub) {
    std::string commSubStr = std::string(commSub);
    const std::string threadsListPath = "/proc/" + std::to_string(pid) + "/task/";

    DIR* dir = nullptr;
    if((dir = opendir(threadsListPath.c_str())) == nullptr) {
        return 0;
    }

    int32_t count = 0;
    struct dirent* entry;
    while((entry = readdir(dir)) != nullptr) {
        std::string threadNamePath = threadsListPath + std::string(entry->d_name) + "/comm";

        std::ifstream fileStream(threadNamePath, std::ios::in);
        if(!fileStream.is_open()) {
            closedir(dir);
            return 0;
        }

        std::string value = "";
        if(!getline(fileStream, value)) {
            closedir(dir);
            return 0;
        }

        to_lower(value);
        to_lower(commSubStr);
        if(value.find(commSubStr) != std::string::npos) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

int32_t PostProcessingBlock::countEncoders(const char* buffer, const char* encoderStr) {
    if (buffer == nullptr || encoderStr == nullptr) {
        return 0;
    }

    int32_t count = 0;
    const char* current = buffer;
    const size_t encoderStrLen = strlen(encoderStr);

    while ((current = strstr(current, encoderStr)) != nullptr) {
        count++;
        current += encoderStrLen;
    }

    return count;
}

std::string PostProcessingBlock::extractSourceName(const char* buffer,
                                                   const char* namePrefix,
                                                   const char* defaultName) {
    if (buffer == nullptr || namePrefix == nullptr) {
        return std::string(defaultName);
    }

    const char* namePtr = strstr(buffer, namePrefix);
    if (namePtr != nullptr) {
        namePtr += strlen(namePrefix);

        // Find the end of the name
        const char* nameEnd = namePtr;
        while (*nameEnd && *nameEnd != ' ' && *nameEnd != '\t' &&
               *nameEnd != '\n' && *nameEnd != '\r' && *nameEnd != '!') {
            nameEnd++;
        }

        // Return extracted name as string
        if (nameEnd > namePtr) {
            return std::string(namePtr, nameEnd - namePtr);
        }
    }

    return std::string(defaultName);
}

uint32_t PostProcessingBlock::extractHeight(const char* buffer) {
    if(buffer == nullptr) {
        return 0;
    }

    uint32_t maxVal = 0;
    const char* ptr = buffer;
    while((ptr = strstr(ptr, "height=")) != nullptr) {
        ptr += strlen("height=");
        char* endPtr = nullptr;
        int64_t value = strtol(ptr, &endPtr, 10);
        if(endPtr != ptr && value > 0 && static_cast<uint32_t>(value) > maxVal) {
            maxVal = static_cast<uint32_t>(value);
        }
    }

    return maxVal;
}

uint32_t PostProcessingBlock::extractWidth(const char* buffer) {
    if(buffer == nullptr) {
        return 0;
    }

    uint32_t maxVal = 0;
    const char* ptr = buffer;
    while((ptr = strstr(ptr, "width=")) != nullptr) {
        ptr += strlen("width=");
        char* endPtr = nullptr;
        int64_t value = strtol(ptr, &endPtr, 10);
        if(endPtr != ptr && value > 0 && static_cast<uint32_t>(value) > maxVal) {
            maxVal = static_cast<uint32_t>(value);
        }
    }

    return maxVal;
}

uint32_t PostProcessingBlock::extractFrameRate(const char* buffer,
                                               const char* frameRatePrefix) {
    if (buffer == nullptr || frameRatePrefix == nullptr) {
        return 0;
    }

    uint32_t maxVal = 0;
    const char* ptr = buffer;
    while((ptr = strstr(ptr, frameRatePrefix)) != nullptr) {
        ptr += strlen(frameRatePrefix);

        char* endPtr = nullptr;
        int64_t numerator = strtol(ptr, &endPtr, 10);
        if(endPtr == ptr || numerator < 0) {
            continue;
        }

        int64_t denominator = 1;
        if(*endPtr == '/') {
            const char* denomStart = endPtr + 1;
            int64_t parsedDenom = strtol(denomStart, &endPtr, 10);
            if(endPtr != denomStart && parsedDenom > 0) {
                denominator = parsedDenom;
            }
        }

        uint32_t fps = static_cast<uint32_t>(numerator / denominator);
        if(fps > maxVal) {
            maxVal = fps;
        }
    }

    return maxVal;
}

/**
 * @brief Calculate sigType value for encoder based on count thresholds
 *
 * @return sigType value based on encoder load classification:
 *         0 for normal load (≤12 encoders/threads)
 *         13 for high load (>12 encoders/threads)
 */
uint32_t PostProcessingBlock::calculateEncoderSigType(int32_t count) {
    return (count <= 12) ? 0 : 13;
}

/**
 * @brief Calculate sigType value for decoder based on thread count thresholds
 *
 * @return sigType value based on decoder load classification:
 *         0 for low load (0-4 threads)
 *         5 for medium load (5-20 threads)
 *         21 for high load (20+ threads)
 */
uint32_t PostProcessingBlock::calculateDecoderSigType(int32_t threadCount) {
    if (threadCount < 5){
        return 0;
    } else if (threadCount >= 5 && threadCount <= 20) {
        return 5;
    } else {
        return 21;
    }
}

int32_t PostProcessingBlock::fetchUsecaseDetails(int32_t pid,
                                                 char *buf,
                                                 uint32_t& sigId,
                                                 uint32_t& sigType,
                                                 uint32_t** extraArgs) {
    // GStreamer element identifiers for different video operations
    const std::vector<const char*> encoderList = {
        "v4l2h264enc",    // Hardware H.264 encoder element
        "v4l2h265enc",    // Hardware H.265 encoder element
        "qtic2venc",      // Qualcomm C2 encoder element
    };

    const std::vector<const char*> decoderList = {
        "v4l2h264dec",    // Hardware H.264 decoder element
        "v4l2h265dec",    // Hardware H.264 decoder element
        "qtic2vdec",      // Qualcomm C2 decoder element
    };

    const std::vector<const char*> mmSrcList = {
        "qtiqmmfsrc",    // Qualcomm multimedia source element
        "libcamerasrc",  // Upstream multimedia source element
    };

    const char* namePrefix      = "name=";        // GStreamer element name attribute
    const char* defaultName     = "camsrc";       // Default camera source name
    const char* frameRatePrefix = "framerate=";   // GStreamer frame rate attribute

    // Check which source element is in use
    uint32_t srcElement = 0;
    for (size_t i = 0; i < mmSrcList.size(); i++) {
        const char* src = mmSrcList[i];
        if (strstr(buf, src) != nullptr) {
            srcElement = i + 1;
            break;
        }
    }

    // No camera source detected — not a camera/live workload, skip signaling.
    if (srcElement == 0) {
        return -1;
    }

    // Extract frame rate once; used by encoder and preview paths.
    *extraArgs = new uint32_t[SIGNAL_EXTRA_ATTRS_COUNT];
    (*extraArgs)[SIGNAL_EXTRA_ATTR_FPS] = extractFrameRate(buf, frameRatePrefix);
    (*extraArgs)[SIGNAL_EXTRA_ATTR_HEIGHT] = extractHeight(buf);
    (*extraArgs)[SIGNAL_EXTRA_ATTR_WIDTH] = extractWidth(buf);
    (*extraArgs)[SIGNAL_EXTRA_ATTR_SRC_ELEMENT] = srcElement;

    LOGD("CAM_BLOCK", "Printing Query Stats");
    LOGD("CAM_BLOCK", "fps=" + std::to_string((*extraArgs)[SIGNAL_EXTRA_ATTR_FPS]));
    LOGD("CAM_BLOCK", "height=" + std::to_string((*extraArgs)[SIGNAL_EXTRA_ATTR_HEIGHT]));
    LOGD("CAM_BLOCK", "width=" + std::to_string((*extraArgs)[SIGNAL_EXTRA_ATTR_WIDTH]));

    // Check for encoder
    const char* matchedEncoder = nullptr;
    for (const char* enc : encoderList) {
        if (strstr(buf, enc) != nullptr) {
            matchedEncoder = enc;
            break;
        }
    }

    if(matchedEncoder != nullptr) {
        std::string sourceName = extractSourceName(buf, namePrefix, defaultName);
        int32_t encoderCount = countEncoders(buf, matchedEncoder);

        // Encode Multi stream case
        if (encoderCount > 1) {
            sigId = URM_SIG_CAMERA_ENCODE_MULTI_STREAMS;
            sigType = calculateEncoderSigType(encoderCount);
        } else {
            // Encode single stream case
            sigId = URM_SIG_CAMERA_ENCODE;
        }

        return 0;
    }

    // Check for decoder
    const char* matchedDecoder = nullptr;
    for (const char* dec : decoderList) {
        if (strstr(buf, dec) != nullptr) {
            matchedDecoder = dec;
            break;
        }
    }

    if(matchedDecoder != nullptr) {
        int32_t numSources = countThreadsWithName(pid, matchedDecoder);
        sigId = URM_SIG_VIDEO_DECODE;
        sigType = calculateDecoderSigType(numSources);
        return 0;
    }

    // Check for preview
    if(srcElement > 0) {
        sigId = URM_SIG_CAMERA_PREVIEW;
        return 0;
    }

    return -1;
}

void PostProcessingBlock::PostProcess(pid_t pid,
                                      uint32_t &sigId,
                                      uint32_t &sigType,
                                      uint32_t** extraArgs) {
	std::string cmdline;
    std::string cmdLinePath = "/proc/" + std::to_string(pid) + "/cmdline";

    if(ReadFirstLine(cmdLinePath, cmdline) <= 0) {
        return;
    }

    char* buf = (char*)cmdline.data();
    size_t sz = cmdline.size();

    SanitizeNulls(buf, sz);
    fetchUsecaseDetails(pid, buf, sigId, sigType, extraArgs);
}

std::once_flag PostProcessingBlock::mInitFlag;
std::unique_ptr<PostProcessingBlock> PostProcessingBlock::mInstance = nullptr;

static void WorkloadPostprocessCallback(void* context) {
    if(context == nullptr) {
        return;
    }

    PostProcessCBData* cbData = static_cast<PostProcessCBData*>(context);
    if(cbData == nullptr) {
        return;
    }

    pid_t pid = cbData->mPid;
    uint32_t sigId = cbData->mSigId;
    uint32_t sigType = cbData->mSigType;

    uint32_t* extraArgs = nullptr;
    PostProcessingBlock::getInstance().PostProcess(pid, sigId, sigType, &extraArgs);

    int64_t handle =
        acquireSignal(sigId, sigType, pid, pid, SIGNAL_EXTRA_ATTRS_COUNT, extraArgs);
    cbData->mHandleAcq = handle;
}

__attribute__((constructor))
static void registerWithUrm() {
    URM_REGISTER_POST_PROCESS_CB("gst-launch-1.0", WorkloadPostprocessCallback)
    URM_REGISTER_POST_PROCESS_CB("gst-camera-per-port-example", WorkloadPostprocessCallback)
}
