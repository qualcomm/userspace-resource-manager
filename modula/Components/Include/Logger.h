// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <ctime>
#include <chrono>
#include <fstream>
#include <cstdarg>
#include <sstream>
#include <syslog.h>

#define LOGD(tag, message) Logger::log(LOG_DEBUG, tag, __func__, message)
#define LOGI(tag, message) Logger::log(LOG_INFO, tag, __func__, message)
#define LOGE(tag, message) Logger::log(LOG_ERR, tag, __func__, message)
#define LOGW(tag, message) Logger::log(LOG_WARNING, tag, __func__, message)
#define TYPELOGV(type, args...) Logger::typeLog(type, __func__, args)
#define TYPELOGD(type) Logger::typeLog(type, __func__)

enum RedirectOptions {
    LOG_TOFILE,
    LOG_TOSYSLOG,
    LOG_TOFTRACE,
    LOG_TOLOGCAT,
};

enum CommonMessageTypes {
    ERRNO_LOG,
    CLIENT_ALLOCATION_FAILURE,
    PROPERTY_RETRIEVAL_FAILED,
    META_CONF_FETCH_FAILED,
    PULSE_MONITOR_INIT_FAILED,
    GARBAGE_COLLECTOR_INIT_FAILED,
    MEMORY_POOL_INVALID_BLOCK_SIZE,
    MEMORY_POOL_BLOCK_RETRIEVAL_FAILURE,
    MEMORY_POOL_ALLOCATION_FAILURE,
    NOTIFY_MODULE_ENABLED,
    NOTIFY_MODULE_NOT_ENABLED,
    SYSTEM_THREAD_CREATION_FAILURE,
    SYSTEM_THREAD_NOT_JOINABLE,
    MODULE_INIT_FAILED,
    REQUEST_MEMORY_ALLOCATION_FAILURE,
    REQUEST_MEMORY_ALLOCATION_FAILURE_HANDLE,
    LISTENER_THREAD_CREATION_SUCCESS,
    LISTENER_THREAD_CREATION_FAILURE,
    NOTIFY_EXTENSIONS_LIB_NOT_PRESENT,
    NOTIFY_EXTENSIONS_LOAD_FAILED,
    NOTIFY_EXTENSIONS_LIB_LOADED_SUCCESS,
    REQUEST_PARSING_FAILURE,
    META_CONFIG_PARSE_FAILURE,
    THREAD_POOL_CREATION_FAILURE,
    THREAD_POOL_INIT_FAILURE,
    THREAD_POOL_THREAD_ALLOCATION_FAILURE,
    THREAD_POOL_THREAD_CREATION_FAILURE,
    THREAD_POOL_THREAD_TERMINATED,
    THREAD_POOL_ENQUEUE_TASK_FAILURE,
    THREAD_POOL_FULL_ALERT,
    TIMER_START_FAILURE,
    VERIFIER_INVALID_OPCODE,
    VERIFIER_INVALID_DEVICE_MODE,
    VERIFIER_INVALID_PRIORITY,
    VERIFIER_VALUE_OUT_OF_BOUNDS,
    VERIFIER_INVALID_PERMISSION,
    VERIFIER_NOT_SUFFICIENT_PERMISSION,
    VERIFIER_LOGICAL_TO_PHYSICAL_MAPPING_FAILED,
    VERIFIER_REQUEST_VALIDATED,
    VERIFIER_STATUS_FAILURE,
    VERIFIER_UNSUPPORTED_SIGNAL_TUNING,
    VERIFIER_NOT_SUFFICIENT_SIGNAL_ACQ_PERMISSION,
    VERIFIER_TARGET_CHECK_FAILED,
    VERIFIER_CGROUP_NOT_FOUND,
    VERIFIER_INVALID_LOGICAL_CORE,
    VERIFIER_INVALID_LOGICAL_CLUSTER,
    NOTIFY_COCO_TABLE_INSERT_START,
    NOTIFY_COCO_TABLE_INSERT_SUCCESS,
    NOTIFY_COCO_TABLE_UPDATE_START,
    NOTIFY_COCO_TABLE_REMOVAL_START,
    NOTIFY_COCO_TABLE_REQUEST_EXPIRY,
    NOTIFY_COCO_TABLE_WRITE,
    RATE_LIMITER_RATE_LIMITED,
    RATE_LIMITER_GLOBAL_RATE_LIMIT_HIT,
    YAML_PARSE_ERROR,
    NOTIFY_RESOURCE_TUNER_INIT_START,
    NOTIFY_CURRENT_TARGET_NAME,
    NOTIFY_PARSING_START,
    NOTIFY_PARSING_SUCCESS,
    NOTIFY_PARSING_FAILURE,
    NOTIFY_PARSER_FILE_NOT_FOUND,
    LOGICAL_TO_PHYSICAL_MAPPING_GEN_FAILURE,
    LOGICAL_TO_PHYSICAL_MAPPING_GEN_SUCCESS,
    SIGNAL_REGISTRY_SIGNAL_NOT_FOUND,
    SIGNAL_REGISTRY_PARSING_FAILURE,
    RESOURCE_REGISTRY_RESOURCE_NOT_FOUND,
    RESOURCE_REGISTRY_PARSING_FAILURE,
    CLIENT_ENTRY_CREATION_FAILURE,
    REQUEST_MANAGER_DUPLICATE_FOUND,
    REQUEST_MANAGER_REQUEST_NOT_ACTIVE,
    EXT_FEATURE_CONFIGS_ELEM_LIB_OPEN_FAILED,
    EXT_FEATURE_ROUTINE_NOT_DEFINED,
    CORE_COUNT_EXTRACTION_FAILED,
    CLUSTER_CPU_LIST_EXTRACTION_FAILED,
    CLUSTER_CPU_CAPACITY_EXTRACTION_FAILED,
    NOTIFY_NODE_WRITE,
    NOTIFY_NODE_WRITE_S,
    NOTIFY_NODE_RESET,
    GENERIC_CALL_FAILURE_LOG,
    INV_ATTR_VAL,
    INV_COCO_TBL_INDEX,
    NOTIFY_RESMODE_REJECT,
    NOTIFY_CLASSIFIER_PROC_EVENT,
    NOTIFY_CLASSIFIER_PROC_IGNORE,
    NOTIFY_MODEL_PREDICTION,
    SYSTEM_BUS_CONN_FAILED,
};

/**
 * @brief Logger.
 * @details Provides a Simplified and Consistent interface for Logging across different Targets
 *          Logging levels supported:\n
 *          1. Debug - For almost all non-essential debug statements.
 *          2. Info - For essential statements.
 *          3. Error - Statements if printed, shows errors.
 */
class Logger {
private:
    static int32_t mLowestLogLevel;
    static int8_t mLevelSpecificLogging;
    static RedirectOptions mRedirectOutputTo;

    static std::string getTimestamp();
    static const char* levelToString(int32_t level);

public:
    /**
     * @brief Configure the Logger
     * @details The logger is designed to be Customizable. Among the knobs which can be configured are:
     *          1) What levels to Log, (User can specify an exact value say ERROR, or a Lower Bound say INFO).
     *             In case of Lower Bound Config for Logging, any Message with Log Level higher than the
     *             specified Level will be logged.
     *          2) Output Redirection: Output can be captured in a File or directly in syslog
     * @param level The exact level to Log or the Lower Bound Logging level.
     * @param levelSpecificLogging Indicates whether Exact Level Logging is needed, default behaviour is
     *                             Lower Bound Logging.
     * @param redirectOutputTo Indicates whether a new file needs to be created to capture the Logging,
     *                         Or syslog should be used.
     */
    static void configure(int32_t level, int8_t levelSpecificLogging, RedirectOptions redirectOutputTo);

    /**
     * @brief Responsible for actually Logging a Message to the desired Medium (file or syslog)
     * @details Note, this Routine should not be called directly, instead the Macros
     *          LOGD, LOGE, LOGI, TYPELOGD and TYPELOGV should be used.
     */
    static void log(int32_t level, const std::string& tag, const std::string& funcName, const char* message);
    static void log(int32_t level, const std::string& tag, const std::string& funcName, const std::string& message);
    static void typeLog(CommonMessageTypes type, const std::string& funcName, ...);
};

#endif
