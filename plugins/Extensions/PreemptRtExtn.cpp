// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <strings.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "Helpers.h"

#define POLICY_DIR_PATH "/sys/devices/system/cpu/cpufreq/"
#define IRQ_DIR_PATH    "/proc/irq/"
#define WQ_DIR_PATH     "/sys/devices/virtual/workqueue/"

// ---------------------------
// Conditional logging (URM_EXT__RT)
// ---------------------------
static bool gLogInit = false;
static bool gLogEnabled = false;
static constexpr const char* kLogTag = "urm-ext-rt";

static uint64_t getValidMask() {
    uint64_t coreCount = GET_TARGET_INFO(GET_CORE_COUNT, 0, nullptr);
    if (coreCount == 0 || coreCount > 64) coreCount = 64;
    return (1ULL << coreCount) - 1;
}

static bool parseBoolEnv(const char* v) {
    if (!v) return false;
    return (!strcasecmp(v, "1") || !strcasecmp(v, "true") ||
            !strcasecmp(v, "on") || !strcasecmp(v, "yes") ||
            !strcasecmp(v, "y"));
}

static inline bool isLogEnabled() {
    if (!gLogInit) {
        gLogEnabled = parseBoolEnv(std::getenv("URM_EXT_RT"));
        gLogInit = true;
    }
    return gLogEnabled;
}

static void logLine(const std::string& msg) {
    if (!isLogEnabled()) return;
    LOGI(kLogTag, msg.c_str());
}


static inline void logWriteFailure(const std::string& path, int rc) {
    if (!isLogEnabled()) return;
    const int e = (rc < 0) ? -rc : rc;
    const char* err = strerror(e);
    std::string msg = "write failed for " + path +
                      " rc=" + std::to_string(rc) +
                      " err='" + std::string(err ? err : "unknown") + "'";
    logLine(msg);
}

// ---------------------------
// PREEMPT_RT detection for cyclictest
// ---------------------------
static bool isPreemptRtActive() {
    std::string rt;
    if (readLineFromFile("/sys/kernel/realtime", rt)) {
        rt = trim(rt);
        if (isLogEnabled()) logLine(std::string("/sys/kernel/realtime = '") + rt + "'");
        if (rt == "1") return true;
        if (rt == "0") return false;
    }
    struct utsname u{};
    if (uname(&u) == 0) {
        std::string ver(u.version);
        toLower(ver);
        if (isLogEnabled()) logLine(std::string("uname -v: ") + u.version);
        if (ver.find("preempt rt") != std::string::npos || ver.find("preempt_rt") != std::string::npos) {
            return true;
        }
    }
    return false;
}

// ---------------------------
// cpufreq: apply/tear
// ---------------------------
static bool gCpufreqApplied = false;
static std::vector<std::pair<std::string, std::string>> gCpufreqGovBackup;

static void cpufreqGovApplierCallback(void* /*context*/) {
    logLine("enter cpufreqGovApplierCallback");

    if (gCpufreqApplied) return;

    gCpufreqGovBackup.clear();

    DIR* dir = opendir(POLICY_DIR_PATH);
    if (!dir) {
        TYPELOGV(ERRNO_LOG, strerror(errno));
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "policy", 6) != 0) continue;

        std::string base = std::string(POLICY_DIR_PATH) + entry->d_name;
        std::string govFile = base + "/scaling_governor";
        if (!isWritable(govFile)) continue;

        std::string oldVal;
        if (readLineFromFile(govFile, oldVal)) {
            gCpufreqGovBackup.emplace_back(govFile, oldVal);
            logLine("[" + std::string(entry->d_name) + "] old governor: " + oldVal);
            int rc = writeLineToFile(govFile, "performance");
            if (rc != 0) {
               logWriteFailure(govFile, rc);
            }
            
            if (rc == 0) {
                std::string now;
                if (readLineFromFile(govFile, now)) {
                    logLine("verify " + govFile + " -> " + now);
                }
            }
        }
    }
    closedir(dir);
    gCpufreqApplied = !gCpufreqGovBackup.empty();
}

static void cpufreqGovTearCallback(void* /*context*/) {
    if (!gCpufreqApplied) return;
    logLine("enter cpufreqTearCallback");

    for (const auto& kv : gCpufreqGovBackup) {
        const std::string& path = kv.first;
        const std::string& oldVal = kv.second;
        AuxRoutines::writeToFile(path, oldVal);
    }
    gCpufreqGovBackup.clear();
    gCpufreqApplied = false;
}

// ---------------------------
// IRQ affinity: apply/tear
// ---------------------------
static bool gIrqApplied = false;
static std::vector<std::pair<std::string, std::string>> gIrqAffBackup;

static void irqAffinityApplierCallback(void* /*context*/) {
    logLine("enter irqAffinityApplierCallback");

    if (gIrqApplied) return;

    gIrqAffBackup.clear();

    int32_t args[2] = {GET_MAX_CLUSTER, -1};
    uint64_t hexMask = GET_TARGET_INFO(GET_MASK, 2, args);
    uint64_t validMask = getValidMask();
    std::string maskStr = cpuMaskToHex((~(hexMask) & validMask));

    DIR* dir = opendir(IRQ_DIR_PATH);
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // numeric directories only
        bool numeric = true;
        for (const char* p = entry->d_name; *p; ++p) {
            if (!std::isdigit(static_cast<unsigned char>(*p))) { numeric = false; break; }
        }
        if (!numeric) continue;

        std::string smpFile = std::string(IRQ_DIR_PATH) + entry->d_name + "/smp_affinity";
        if (!isWritable(smpFile)) continue;

        std::string oldVal;

        if (readLineFromFile(smpFile, oldVal)) {
            gIrqAffBackup.emplace_back(smpFile, oldVal);

            int rc = writeLineToFile(smpFile, maskStr);
            if (rc != 0) {
                logWriteFailure(smpFile, rc);
            }
            if (rc == 0) {
                std::string now;
                if (readLineFromFile(smpFile, now)) {
                    logLine("verify " + smpFile + " -> " + now);
                }
            }
        }
    }
    closedir(dir);
    gIrqApplied = !gIrqAffBackup.empty();
}

static void irqAffinityTearCallback(void* /*context*/) {
    if (!gIrqApplied) return;
    logLine("enter irqAffinityTearCallback");

    for (const auto& kv : gIrqAffBackup) {
        const std::string& path = kv.first;
        const std::string& oldVal = kv.second;
        AuxRoutines::writeToFile(path, oldVal);
    }
    gIrqAffBackup.clear();
    gIrqApplied = false;
}

// ---------------------------
// Workqueue cpumask: apply/tear
// ---------------------------
static bool gWqApplied = false;
static std::vector<std::pair<std::string, std::string>> gWqMaskBackup;

static void workqueueApplierCallback(void* /*context*/) {
    logLine("enter workqueueApplierCallback");
    if (gWqApplied) return;

    gWqMaskBackup.clear();

    int32_t args[2] = {GET_MAX_CLUSTER, -1};
    uint64_t hexMask = GET_TARGET_INFO(GET_MASK, 2, args);
    uint64_t validMask = getValidMask();
    std::string maskStr = cpuMaskToHex((~(hexMask) & validMask));

    DIR* dir = opendir(WQ_DIR_PATH);
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue; // skip . and ..
        std::string cpumaskFile = std::string(WQ_DIR_PATH) + entry->d_name + "/cpumask";
        if (!isWritable(cpumaskFile)) continue;

        std::string oldVal;
        if (readLineFromFile(cpumaskFile, oldVal)) {
            gWqMaskBackup.emplace_back(cpumaskFile, oldVal);

            int rc = writeLineToFile(cpumaskFile, maskStr);
            if (rc != 0) {
                logWriteFailure(cpumaskFile, rc);
            }
            if (rc == 0) {
                std::string now;
                if (readLineFromFile(cpumaskFile, now)) {
                    logLine("verify " + cpumaskFile + " -> " + now);
                }
            }
        }
    }
    closedir(dir);
    gWqApplied = !gWqMaskBackup.empty();
}

static void workqueueTearCallback(void* /*context*/) {
    if (!gWqApplied) return;
    logLine("enter workqueueTearCallback");

    for (const auto& kv : gWqMaskBackup) {
        const std::string& path = kv.first;
        const std::string& oldVal = kv.second;

        AuxRoutines::writeToFile(path, oldVal);
    }
    gWqMaskBackup.clear();
    gWqApplied = false;
}


// ---------------------------
// Tracing: apply/tear
// ---------------------------
#define TRACING_ON_PATH "/sys/kernel/tracing/tracing_on"

static bool gTracingApplied = false;
static std::string gTracingBackup;

static void tracingApplierCallback(void* /*context*/) {
    logLine("enter tracingApplierCallback");

    if (gTracingApplied) return;

    gTracingBackup.clear();

    if (!isWritable(TRACING_ON_PATH)) {
        logLine("tracing_on not writable, skipping");
        return;
    }

    if (readLineFromFile(TRACING_ON_PATH, gTracingBackup)) {
        logLine("tracing_on old value: " + gTracingBackup);
        int rc = writeLineToFile(TRACING_ON_PATH, "0");
        if (rc != 0) {
            logWriteFailure(TRACING_ON_PATH, rc);
            gTracingBackup.clear();
            return;
        }
        std::string now;
        if (readLineFromFile(TRACING_ON_PATH, now)) {
            logLine("verify " TRACING_ON_PATH " -> " + now);
        }
        gTracingApplied = true;
    }
}

static void tracingTearCallback(void* /*context*/) {
    if (!gTracingApplied) return;
    logLine("enter tracingTearCallback");

    int rc = writeLineToFile(TRACING_ON_PATH, gTracingBackup);
    if (rc != 0) {
        logWriteFailure(TRACING_ON_PATH, rc);
    } else {
        logLine("restored " TRACING_ON_PATH " -> " + gTracingBackup);
    }
    gTracingBackup.clear();
    gTracingApplied = false;
}

// ---------------------------
// URM registrations
// ---------------------------

// IDs:
//   0x00800001 -> cpufreq
//   0x00800002 -> irqaffinity
//   0x00800003 -> workqueue
//   0x00800004 -> tracing

URM_REGISTER_RES_APPLIER_CB(0x00800001, cpufreqGovApplierCallback)
URM_REGISTER_RES_TEAR_CB   (0x00800001, cpufreqGovTearCallback)

URM_REGISTER_RES_APPLIER_CB(0x00800002, irqAffinityApplierCallback)
URM_REGISTER_RES_TEAR_CB   (0x00800002, irqAffinityTearCallback)

URM_REGISTER_RES_APPLIER_CB(0x00800003, workqueueApplierCallback)
URM_REGISTER_RES_TEAR_CB   (0x00800003, workqueueTearCallback)

URM_REGISTER_RES_APPLIER_CB(0x00800004, tracingApplierCallback)
URM_REGISTER_RES_TEAR_CB   (0x00800004, tracingTearCallback)
