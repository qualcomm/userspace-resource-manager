// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Logger.h"

int32_t Logger::mLowestLogLevel = LOG_DEBUG;
int8_t Logger::mLevelSpecificLogging = false;
RedirectOptions Logger::mRedirectOutputTo = RedirectOptions::LOG_TOSYSLOG;

void Logger::configure(int32_t level, int8_t levelSpecificLogging, RedirectOptions redirectOutputTo) {
    mLowestLogLevel = level;
    mLevelSpecificLogging = levelSpecificLogging;
    mRedirectOutputTo = redirectOutputTo;
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    time_t now_c = std::chrono::system_clock::to_time_t(now);
    tm local_tm = *localtime(&now_c);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_tm);
    return std::string(buffer);
}

const char* Logger::levelToString(int32_t level) {
    switch(level) {
        case LOG_DEBUG:
            return "DEBUG";
        case LOG_INFO:
            return "INFO";
        case LOG_ERR:
            return "ERROR";
        case LOG_WARNING:
            return "WARN";
        default:
            break;
    }
    return "";
}

void Logger::log(int32_t level,
                 const std::string& tag,
                 const std::string& funcName,
                 const char* message) {
    if(mLevelSpecificLogging) {
        if(level != mLowestLogLevel) return;
    } else {
        if(level > mLowestLogLevel) return;
    }

    std::string timestamp = getTimestamp();
    const char* levelStr = levelToString(level);

    switch(mRedirectOutputTo) {
        case RedirectOptions::LOG_TOSYSLOG: {
            syslog(level, "[%s] [%s] %s: %s", tag.c_str(), levelStr, funcName.c_str(), message);
            break;
        }

        case RedirectOptions::LOG_TOFTRACE: {
            std::ofstream logFile("/sys/kernel/debug/tracing/trace_marker", std::ios::app);
            if(logFile.is_open()) {
                logFile<<"["<<timestamp<<"] ["<<tag<<"] ["<<levelStr<<"] "<<funcName<<": "<<message<<std::endl;
                logFile.close();
            }
            break;
        }

        case RedirectOptions::LOG_TOFILE: {
            std::ofstream logFile("log.txt", std::ios::app);
            if(logFile.is_open()) {
                logFile<<"["<<timestamp<<"] ["<<tag<<"] ["<<levelStr<<"] "<<funcName<<": "<<message<<std::endl;
                logFile.close();
            }
            break;
        }

        default:
            break;
    }
}

void Logger::log(int32_t level,
                 const std::string& tag,
                 const std::string& funcName,
                 const std::string& message) {

    if(mLevelSpecificLogging) {
        if(level != mLowestLogLevel) return;
    } else {
        if(level > mLowestLogLevel) return;
    }

    std::string timestamp = getTimestamp();
    const char* levelStr = levelToString(level);

    switch(mRedirectOutputTo) {
        case RedirectOptions::LOG_TOSYSLOG: {
            syslog(level, "[%s] [%s] %s: %s", tag.c_str(), levelStr, funcName.c_str(), message.c_str());
            break;
        }

        case RedirectOptions::LOG_TOFTRACE: {
            std::ofstream logFile("/sys/kernel/debug/tracing/trace_marker", std::ios::app);
            if(logFile.is_open()) {
                logFile<<"["<<timestamp<<"] ["<<tag<<"] ["<<levelStr<<"] "<<funcName<<": "<<message<<std::endl;
                logFile.close();
            }
            break;
        }

        case RedirectOptions::LOG_TOFILE: {
            std::ofstream logFile("log.txt", std::ios::app);
            if(logFile.is_open()) {
                logFile<<"["<<timestamp<<"] ["<<tag<<"] ["<<levelStr<<"] "<<funcName<<": "<<message<<std::endl;
                logFile.close();
            }
            break;
        }

        default:
            break;
    }
}

void Logger::typeLog(CommonMessageTypes type, const std::string& funcName, ...) {
    char buffer[256];
    va_list args;
    va_start(args, funcName);

    switch(type) {
        case CommonMessageTypes::NOTIFY_MODULE_ENABLED:
            vsnprintf(buffer, sizeof(buffer),
                     "Module: [%s] is Enabled, Proceeding with Initialization", args);

            Logger::log(LOG_INFO, "RESTUNE_COMPONENT_REGISTRY",
                        funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_MODULE_NOT_ENABLED:
            vsnprintf(buffer, sizeof(buffer),
                      "Module: [%s] is Not Enabled, Dropping Request", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_DECODE",
                        funcName, buffer);
            break;

        case CommonMessageTypes::SYSTEM_THREAD_CREATION_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "[%s] thread could not be created, Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_SYSTEM_ERROR",
                        funcName, buffer);
            break;

        case CommonMessageTypes::LISTENER_THREAD_CREATION_SUCCESS:
            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName,
                        "Listener Thread Successfully Created, Server ready for Requests");
            break;

        case CommonMessageTypes::LISTENER_THREAD_CREATION_FAILURE:
            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName,
                        "Listener Thread Creation Failed, Aborting Initialization");
            break;

        case CommonMessageTypes::CLIENT_ALLOCATION_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                     "Memory allocation for Client: " \
                     "[PID: %d, TID: %d]. Failed with Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_CLIENT_DATA_MANAGER",
                        funcName, buffer);
            break;

        case CommonMessageTypes::PROPERTY_RETRIEVAL_FAILED:
            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName,
                        "Failed to Fetch Properties, " \
                        "Boot Configs. Resource Tuner Server Initialization Failed.");
            break;

        case CommonMessageTypes::META_CONFIG_PARSE_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                     "Fetch Meta Configs failed, with Error %s", args);

            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::THREAD_POOL_CREATION_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Failed to create Thread Pool. Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::THREAD_POOL_INIT_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Could Not Initialize Thread Pool. Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_THREAD_POOL", funcName, buffer);
            break;

        case CommonMessageTypes::THREAD_POOL_THREAD_ALLOCATION_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Failed to allocate Memory for ThreadNode. " \
                      "Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_THREAD_POOL", funcName, buffer);
            break;

        case CommonMessageTypes::THREAD_POOL_THREAD_CREATION_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Failed to create New thread, call to std::thread Failed. " \
                      "Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_THREAD_POOL", funcName, buffer);
            break;

        case CommonMessageTypes::THREAD_POOL_THREAD_TERMINATED:
            vsnprintf(buffer, sizeof(buffer),
                      "Thread Terminated with Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_THREAD_POOL", funcName, buffer);
            break;

        case CommonMessageTypes::THREAD_POOL_ENQUEUE_TASK_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Task Submisission failed with Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_THREAD_POOL", funcName, buffer);
            break;

        case CommonMessageTypes::THREAD_POOL_FULL_ALERT:
            Logger::log(LOG_ERR, "RESTUNE_THREAD_POOL", funcName,
                        "ThreadPool is full, Task Submission Failed");

            break;

        case CommonMessageTypes::TIMER_START_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Failed to start Timer for Request [%ld], Dropping Request.", args);

            Logger::log(LOG_ERR, "RESTUNE_COCO_TABLE", funcName, buffer);

            break;

        case CommonMessageTypes::NOTIFY_RESOURCE_TUNER_INIT_START:
            vsnprintf(buffer, sizeof(buffer),
                      "Starting Resource Tuner Server, PID = [%d]", args);

            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_CURRENT_TARGET_NAME:
            vsnprintf(buffer, sizeof(buffer),
                      "Detected Target: [%s]", args);

            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName, buffer);

            break;

        case CommonMessageTypes::PULSE_MONITOR_INIT_FAILED:
            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName,
                        "Pulse Monitor Could not be started, " \
                        "Aborting Initialization.");
            break;

        case CommonMessageTypes::GARBAGE_COLLECTOR_INIT_FAILED:
            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName,
                        "Client Garbage Collector Could not be started, " \
                        "Aborting Initialization.");
            break;

        case CommonMessageTypes::MEMORY_POOL_INVALID_BLOCK_SIZE:
            vsnprintf(buffer, sizeof(buffer),
                     "Invalid block Size Provided [size = %d bytes.]", args);

            Logger::log(LOG_ERR, "RESTUNE_MEMORY_POOL", funcName, buffer);
            break;

        case CommonMessageTypes::MEMORY_POOL_BLOCK_RETRIEVAL_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "No Free Blocks of Requested Size Available " \
                      "[size = %d bytes.]", args);

            Logger::log(LOG_ERR, "RESTUNE_MEMORY_POOL", funcName, buffer);
            break;

        case CommonMessageTypes::MEMORY_POOL_ALLOCATION_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Insufficient Memory to Allocate Desired Number of Blocks" \
                      "[Requested Size = %d bytes, "                             \
                      "Requested Count = %d, "                                   \
                      "Allocated Count = %d.]", args);

            Logger::log(LOG_ERR, "RESTUNE_MEMORY_POOL", funcName, buffer);
            break;

        case CommonMessageTypes::MODULE_INIT_FAILED:
            vsnprintf(buffer, sizeof(buffer),
                      "Failed to Initialize Module: %s, " \
                      "Aborting Server Startup.", args);

            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_EXTENSIONS_LIB_NOT_PRESENT:
            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName,
                        "Extension library not present at expected path — ignoring");
            break;

        case CommonMessageTypes::NOTIFY_EXTENSIONS_LOAD_FAILED:
            vsnprintf(buffer, sizeof(buffer),
                      "Error loading extension lib: [%s], Error: [%s]", args);

            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_EXTENSIONS_LIB_LOADED_SUCCESS:
            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName,
                        "Successfully Loaded: [%d] extension plugins");
            break;

        case CommonMessageTypes::NOTIFY_COCO_TABLE_INSERT_START:
            vsnprintf(buffer, sizeof(buffer),
                      "Inserting in CocoTable: Request Handle: [%ld]", args);

            Logger::log(LOG_DEBUG, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_COCO_TABLE_INSERT_SUCCESS:
            vsnprintf(buffer, sizeof(buffer),
                      "Request Handle: [%ld] Successfully inserted in CocoTable", args);

            Logger::log(LOG_DEBUG, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_COCO_TABLE_UPDATE_START:
            vsnprintf(buffer, sizeof(buffer),
                      "Updating Request Handle: [%ld], to New Duration: [%ld]", args);

            Logger::log(LOG_DEBUG, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_COCO_TABLE_REMOVAL_START:
            vsnprintf(buffer, sizeof(buffer),
                      "Request cleanup for Request Handle: [%ld] initiated", args);

            Logger::log(LOG_DEBUG, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_COCO_TABLE_REQUEST_EXPIRY:
            vsnprintf(buffer, sizeof(buffer),
                      "Timer over for Request Handle: [%ld]", args);

            Logger::log(LOG_DEBUG, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_COCO_TABLE_WRITE:
            vsnprintf(buffer, sizeof(buffer),
                      "Writing value: [%d], to node [%s]", args);

            Logger::log(LOG_DEBUG, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::REQUEST_MEMORY_ALLOCATION_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Memory allocation for Incoming Request. " \
                      "Failed with Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_ALLOCATION_FAILURE",
                        funcName, buffer);
            break;

        case CommonMessageTypes::REQUEST_MEMORY_ALLOCATION_FAILURE_HANDLE:
            vsnprintf(buffer, sizeof(buffer),
                      "Memory allocation for Request: [%ld]. " \
                      "Failed with Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_SERVER",
                        funcName, buffer);
            break;

        case CommonMessageTypes::REQUEST_PARSING_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Request Parsing Failed, Request is Malformed. " \
                      "Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_PARSING_FAILURE",
                        funcName, buffer);
            break;

        case CommonMessageTypes::ERRNO_LOG:
            vsnprintf(buffer, sizeof(buffer),
                      "Call to %s, Failed with Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_SYSCALL_FAILURE", funcName, buffer);
            break;

        case CommonMessageTypes::CORE_COUNT_EXTRACTION_FAILED:
            vsnprintf(buffer, sizeof(buffer),
                      "Failed to get Online Core Count, Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_TARGET_REGISTRY", funcName, buffer);
            break;

        case CommonMessageTypes::CLUSTER_CPU_LIST_EXTRACTION_FAILED:
            vsnprintf(buffer, sizeof(buffer),
                      "Failed to get the list of CPUs for cluster [%s], Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_TARGET_REGISTRY", funcName, buffer);
            break;

        case CommonMessageTypes::CLUSTER_CPU_CAPACITY_EXTRACTION_FAILED:
            vsnprintf(buffer, sizeof(buffer),
                      "Failed to get the capacity for CPU [%d], Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_TARGET_REGISTRY", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_INVALID_OPCODE:
            vsnprintf(buffer, sizeof(buffer),
                      "Invalid Opcode [%u], Dropping Request.", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_INVALID_DEVICE_MODE:
            vsnprintf(buffer, sizeof(buffer),
                      "Request [%ld], cannot be processed in current Device Mode. " \
                      "Dropping Request.", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_INVALID_PERMISSION:
            vsnprintf(buffer, sizeof(buffer),
                      "Permissions for Client [PID: %d, TID: %d] Could not be Fetched, " \
                      "Dropping Request.", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_INVALID_PRIORITY:
            vsnprintf(buffer, sizeof(buffer),
                      "Invalid Priority [%d], Dropping Request.", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_UNSUPPORTED_SIGNAL_TUNING:
            vsnprintf(buffer, sizeof(buffer),
                      "Specified Signal [0x%08x] is not enabled for provisioning", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_VALUE_OUT_OF_BOUNDS:
            vsnprintf(buffer, sizeof(buffer),
                      "Config Value [%d] does not fall in the Allowed Range " \
                      "for the Resource [0x%08x], Dropping Request.", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_NOT_SUFFICIENT_PERMISSION:
            vsnprintf(buffer, sizeof(buffer),
                      "Permission Check Failed for Resource [0x%08x], "  \
                      "Dropping Request", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_NOT_SUFFICIENT_SIGNAL_ACQ_PERMISSION:
            vsnprintf(buffer, sizeof(buffer),
                      "Permission Check Failed for Signal [0x%08x], "  \
                      "Client does not have sufficient Permissions to provision Signal", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_TARGET_CHECK_FAILED:
             vsnprintf(buffer, sizeof(buffer),
                       "Specified Signal [0x%08x] is not enabled for provisioning on this Target", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_CGROUP_NOT_FOUND:
             vsnprintf(buffer, sizeof(buffer),
                       "CGroup with Identifier [%d] does not exist", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_LOGICAL_TO_PHYSICAL_MAPPING_FAILED:
            vsnprintf(buffer, sizeof(buffer),
                      "Logical to Physical Core / Cluster Mapping for the "  \
                      "Resource [0x%08x], Failed. Dropping Request", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_INVALID_LOGICAL_CORE:
             vsnprintf(buffer, sizeof(buffer),
                       "Invalid Logical Core value: [%d], dropping Request", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_INVALID_LOGICAL_CLUSTER:
             vsnprintf(buffer, sizeof(buffer),
                       "Invalid Logical Cluster value: [%d], dropping Request", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_REQUEST_VALIDATED:
            vsnprintf(buffer, sizeof(buffer),
                      "Request with handle: %lld, Successfully Validated.", args);

            Logger::log(LOG_INFO, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::VERIFIER_STATUS_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Verification Failed for Request [%lld], Dropping Request.", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_VERIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_NODE_WRITE:
            vsnprintf(buffer, sizeof(buffer),
                      "Writing to Node: [%s], Value: [%d]", args);

            Logger::log(LOG_INFO, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_NODE_WRITE_S:
            vsnprintf(buffer, sizeof(buffer),
                      "Writing to Node: [%s], Value: [%s]", args);

            Logger::log(LOG_INFO, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_NODE_RESET:
            vsnprintf(buffer, sizeof(buffer),
                      "Resetting Node: [%s], to Value: [%s]", args);

            Logger::log(LOG_INFO, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::RATE_LIMITER_RATE_LIMITED:
            vsnprintf(buffer, sizeof(buffer),
                      "Client TID: [%d] Rate Limited, Dropping Request [%lld].", args);

            Logger::log(LOG_INFO, "RESTUNE_RATE_LIMITER", funcName, buffer);
            break;

        case CommonMessageTypes::YAML_PARSE_ERROR:
            vsnprintf(buffer, sizeof(buffer),
                      "Failed to parse file: %s, Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_YAML_PARSER", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_PARSING_START:
            vsnprintf(buffer, sizeof(buffer),
                      "Proceeding with [%s] Config Parsing", args);

            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_PARSING_SUCCESS:
            vsnprintf(buffer, sizeof(buffer),
                      "[%s] Configs successfully parsed", args);

            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_PARSING_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "[%s] Configs Could not be parsed", args);

            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_CUSTOM_CONFIG_FILE:
            vsnprintf(buffer, sizeof(buffer),
                      "Custom [%s] Config file provided [path: %s]", args);

            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_PARSER_FILE_NOT_FOUND:
            vsnprintf(buffer, sizeof(buffer),
                      "[%s] Config Parsing Failed, Expected File: [%s] not found.", args);

            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName, buffer);
            break;

        case CommonMessageTypes::LOGICAL_TO_PHYSICAL_MAPPING_GEN_FAILURE:
            Logger::log(LOG_ERR, "RESTUNE_SERVER_INIT", funcName,
                        "Reading Physical Core, Cluster LOG_INFO Failed, Server Init Failed");
            break;

        case CommonMessageTypes::LOGICAL_TO_PHYSICAL_MAPPING_GEN_SUCCESS:
            Logger::log(LOG_INFO, "RESTUNE_SERVER_INIT", funcName,
                        "Logical to Physical Core / Cluster mapping successfully created");
            break;

        case CommonMessageTypes::SYSTEM_THREAD_NOT_JOINABLE:
            vsnprintf(buffer, sizeof(buffer),
                      "[%s] Thread is not joinable", args);

            Logger::log(LOG_ERR, "RESTUNE_SERVER_TERMINATION", funcName, buffer);
            break;

        case CommonMessageTypes::RATE_LIMITER_GLOBAL_RATE_LIMIT_HIT:
            vsnprintf(buffer, sizeof(buffer),
                      "Max Concurrent Requests Count hit, Dropping Request [%ld]", args);

            Logger::log(LOG_ERR, "RESTUNE_RATE_LIMITER", funcName, buffer);
            break;

        case CommonMessageTypes::SIGNAL_REGISTRY_SIGNAL_NOT_FOUND:
            vsnprintf(buffer, sizeof(buffer),
                      "Signal ID [0x%08x] not found in the registry", args);

            Logger::log(LOG_ERR, "RESTUNE_SIGNAL_REGISTRY", funcName, buffer);
            break;

        case CommonMessageTypes::SIGNAL_REGISTRY_PARSING_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Signal Parsing Failed with Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_SIGNAL_REGISTRY", funcName, buffer);
            break;

        case CommonMessageTypes::RESOURCE_REGISTRY_RESOURCE_NOT_FOUND:
            vsnprintf(buffer, sizeof(buffer),
                      "Resource ID [0x%08x] not found in the registry", args);

            Logger::log(LOG_ERR, "RESTUNE_RESOURCE_REGISTRY", funcName, buffer);
            break;

        case CommonMessageTypes::RESOURCE_REGISTRY_PARSING_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Resource Parsing Failed with Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_RESOURCE_REGISTRY", funcName, buffer);
            break;

        case CommonMessageTypes::CLIENT_ENTRY_CREATION_FAILURE:
            vsnprintf(buffer, sizeof(buffer),
                      "Client Tracking Entry could not be created for handle [%ld] \
                      Dropping Request", args);

            Logger::log(LOG_ERR, "RESTUNE_SERVER", funcName, buffer);
            break;

        case CommonMessageTypes::REQUEST_MANAGER_DUPLICATE_FOUND:
            vsnprintf(buffer, sizeof(buffer),
                      "Duplicate Request found for handle [%ld]. Dropping Request", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_MANAGER", funcName, buffer);
            break;

        case CommonMessageTypes::REQUEST_MANAGER_REQUEST_NOT_ACTIVE:
            vsnprintf(buffer, sizeof(buffer),
                      "No existing request with handle [%ld] found, Dropping this Request", args);

            Logger::log(LOG_ERR, "RESTUNE_REQUEST_MANAGER", funcName, buffer);
            break;

        case CommonMessageTypes::EXT_FEATURE_CONFIGS_ELEM_LIB_OPEN_FAILED:
            vsnprintf(buffer, sizeof(buffer),
                      "Lib with the Path: [%s] could not be opened", args);

            Logger::log(LOG_ERR, "RESTUNE_EXT_FEATURES", funcName, buffer);
            break;

        case CommonMessageTypes::EXT_FEATURE_ROUTINE_NOT_DEFINED:
            vsnprintf(buffer, sizeof(buffer),
                      "No Routine with the Name: [%s] defined by the lib: [%s]", args);

            Logger::log(LOG_ERR, "RESTUNE_EXT_FEATURES", funcName, buffer);
            break;

        case CommonMessageTypes::GENERIC_CALL_FAILURE_LOG:
            vsnprintf(buffer, sizeof(buffer),
                      "Call Failed, Error: %s", args);

            Logger::log(LOG_ERR, "RESTUNE_ORDERED_QUEUE", funcName, buffer);
            break;

        case CommonMessageTypes::INV_ATTR_VAL:
            vsnprintf(buffer, sizeof(buffer),
                      "Encountered Attribute with Invalid Value: Attr [%s], Value: [%s]", args);

            Logger::log(LOG_ERR, "RESTUNE_CONFIG_PROCESSOR", funcName, buffer);
            break;

        case CommonMessageTypes::INV_COCO_TBL_INDEX:
            vsnprintf(buffer, sizeof(buffer),
                      "Encountered Invalid Coco Indexes for Resource: [0x%08x], "\
                      "Primary Index: [%d], Secondary Index: [%d]", args);

            Logger::log(LOG_ERR, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_RESMODE_REJECT:
            vsnprintf(buffer, sizeof(buffer),
                      "Configured Value for Resource: [0x%08x] cannot be applied in "\
                      "current Display Mode: [%d], Skipping.", args);

            Logger::log(LOG_ERR, "RESTUNE_COCO_TABLE", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_CLASSIFIER_PROC_EVENT:
            vsnprintf(buffer, sizeof(buffer),
                      "Received event: [%s] for pid: [%d]", args);

            Logger::log(LOG_DEBUG, "CONTEXTUAL_CLASSIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_CLASSIFIER_PROC_IGNORE:
            vsnprintf(buffer, sizeof(buffer),
                      "Ignoring black listed pid: [%d]", args);

            Logger::log(LOG_DEBUG, "CONTEXTUAL_CLASSIFIER", funcName, buffer);
            break;

        case CommonMessageTypes::NOTIFY_MODEL_PREDICTION:
            vsnprintf(buffer, sizeof(buffer),
                      "Prediction complete. PID: %d, Comm: %s, Class: %s, "\
                      "Probability: %.4f", args);

            Logger::log(LOG_INFO, "CONTEXTUAL_CLASSIFIER", funcName, buffer);
            break;

        default:
            break;
    }

    va_end(args);
}
