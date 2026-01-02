// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#ifdef __cplusplus
#include <cstdint>
#include <cstring>
#else
#include <stdint.h>
#include <string.h>
#endif

// Use these options to configure a well defined value
#define OPT_WITHMAX 0x00000001
#define OPT_WITHMIN 0x00000002

// Used to define the new config value, relative to the already configured value
#define OPT_WITHREL 0X00000004

/**
 * @struct SysResource
 * @brief Used to store information regarding Resources / Tunables which need to be
 *        Provisioned as part of the tuneResources API.
 */
typedef struct {
    /**
     * @brief A uniqued 32-bit (unsigned) identifier for the Resource.
     *        - The last 16 bits (17-32) are used to specify the ResId
     *        - The next 8 bits (9-16) are used to specify the ResType (type of the Resource)
     *        - In addition for Custom Resources, the MSB must be set to 1 as well
     */
    uint32_t mResCode;
    /**
     * @brief Holds Logical Core and Cluster Information:
     *        - The last 8 bits (25-32) hold the Logical Core Value
     *        - The next 8 bits (17-24) hold the Logical Cluster Value
     */
    int32_t mResInfo;
    int32_t mOptionalInfo; //!< Field to hold optional information for Request Processing
    /**
     * @brief Number of values to be configured for the Resource,
     *        both single-valued and multi-valued Resources are supported.
     */
    int32_t mNumValues;

    union {
        int32_t value; //!< Use this field for single Valued Resources
        int32_t* values; //!< Use this field for Multi Valued Resources
    } mResValue; //!< The value to be Configured for this Resource Node.
} SysResource;

/**
 * @enum RequestPriority
 * @brief Requests can have 2 levels of Priorities, HIGH or LOW.
 */
enum RequestPriority {
    REQ_PRIORITY_HIGH = 0,
    REQ_PRIORITY_LOW,
    NUMBER_OF_RQUEST_PRIORITIES
};

/**
 * @enum Modes
 * @brief Represents the operational modes based on the device's display state.
 * @details Certain system resources are optimized only when the device display is active,
 *          primarily to conserve power. However, for critical components, tuning may be
 *          performed regardless of the display state, including during doze mode.
 */
enum Modes {
    MODE_RESUME = 0x01, //!< Tuning allowed when the display is on.
    MODE_SUSPEND = 0x02, //!< Tuning allowed when the display is off.
    MODE_DOZE = 0x04 //!< Tuning allowed during doze (low-power idle) mode.
};

// Helper Macros to set Request and SysResource attributes.
#define SET_REQUEST_PRIORITY(properties, priority)({                                          \
    int32_t retVal;                                                                           \
    if(properties < 0 || priority < 0 || priority >= NUMBER_OF_RQUEST_PRIORITIES) {           \
        retVal = -1;                                                                          \
    } else {                                                                                  \
        retVal = (int32_t) ((properties | priority));                                         \
    }                                                                                         \
    retVal;                                                                                   \
})                                                                                            \

#define ADD_ALLOWED_MODE(properties, mode)({                                                  \
    int32_t retVal;                                                                           \
    if(properties < 0 || mode < MODE_RESUME || mode > MODE_DOZE) {                            \
        retVal = -1;                                                                          \
    } else {                                                                                  \
        retVal = (int32_t) (properties | (((properties >> 8) | mode) << 8));                  \
    }                                                                                         \
    retVal;                                                                                   \
})                                                                                            \

#define EXTRACT_REQUEST_PRIORITY(properties)({                                                \
    (int8_t) ((properties) & ((1 << 8) - 1));                                                 \
})

#define EXTRACT_ALLOWED_MODES(properties)({                                                   \
    (int8_t) ((properties >> 8) & ((1 << 8) - 1));                                            \
})                                                                                            \

// Define Utilities to parse and set the mResInfo field in Resource struct.
#define EXTRACT_RESOURCE_CORE_VALUE(resInfo)({                                                \
    (int8_t) ((resInfo) & ((1 << 8) - 1));                                                    \
})                                                                                            \

#define EXTRACT_RESOURCE_CLUSTER_VALUE(resInfo)({                                             \
    (int8_t) ((resInfo >> 8) & ((1 << 8) - 1));                                               \
})                                                                                            \

#define EXTRACT_RESOURCE_MPAM_VALUE(resInfo)({                                                \
    (int8_t) ((resInfo >> 16) & ((1 << 8) - 1));                                              \
})                                                                                            \

#define SET_RESOURCE_CORE_VALUE(resInfo, newValue)({                                          \
    (int32_t) ((resInfo ^ EXTRACT_RESOURCE_CORE_VALUE(resInfo)) | newValue);                  \
})                                                                                            \

#define SET_RESOURCE_CLUSTER_VALUE(resInfo, newValue)({                                       \
    (int32_t) ((resInfo ^ (EXTRACT_RESOURCE_CLUSTER_VALUE(resInfo) << 8)) | (newValue << 8)); \
})                                                                                            \

#define SET_RESOURCE_MPAM_VALUE(resInfo, newValue)({                                          \
    (int32_t) ((resInfo ^ (EXTRACT_RESOURCE_MPAM_VALUE(resInfo) << 16)) | (newValue << 16));  \
})                                                                                            \

#define CONSTRUCT_RES_CODE(resType, resCode) ({                                              \
    uint32_t resourceBitmap = 0;                                                             \
    resourceBitmap |= ((uint32_t)resCode);                                                   \
    resourceBitmap |= ((uint32_t)resType << 16);                                             \
    resourceBitmap;                                                                          \
})                                                                                           \

#define CONSTRUCT_SIG_CODE(sigID, category) ({                                              \
    uint32_t signalBitmap = 0;                                                              \
    signalBitmap |= ((uint32_t)category);                                                   \
    signalBitmap |= ((uint32_t)sigID << 8);                                                 \
    signalBitmap;                                                                           \
})                                                                                          \

#define CUSTOM(opCode) ({                                                                   \
    uint32_t opBitmap = opCode;                                                             \
    opBitmap |= (1 << 31);                                                                  \
    opBitmap;                                                                               \
})                                                                                          \

// Common ResCode and ResInfo codes
// These enums can be used directly as part of tuneResources API, to uniquely
// specify the resource to be tuned.
// For target-specific resources use the custom ResourcesConfig.yaml file for
// declaration. These values are only applicable for upstream resources.
#define RES_CODE_LIST                     \
/* Resources */                           \
X(RES_SCALE_MIN_FREQ,         0x00040000) \
X(RES_SCALE_MAX_FREQ,         0x00040001) \
X(RES_RATE_LIMIT_US,          0x00040002) \
X(RES_SCHED_UTIL_CLAMP_MIN,   0x00030000) \
X(RES_SCHED_UTIL_CLAMP_MAX,   0x00030001) \
X(RES_SCHED_ENERGY_AWARE,     0x00030002) \
X(RES_CPU_DMA_LATENCY,        0x00010000) \
X(RES_PM_QOS_LATENCY,         0x00010001) \
/* cgroup resources */                    \
X(RES_CGRP_MOVE_PID,          0x00090000) \
X(RES_CGRP_MOVE_TID,          0x00090001) \
X(RES_CGRP_RUN_CORES,         0x00090002) \
X(RES_CGRP_RUN_CORES_EXCL,    0x00090003) \
X(RES_CGRP_FREEZE,            0x00090004) \
X(RES_CGRP_LIMIT_CPU_TIME,    0x00090005) \
X(RES_CGRP_RUN_WHEN_CPU_IDLE, 0x00090006) \
X(RES_CGRP_UCLAMP_MIN,        0x00090007) \
X(RES_CGRP_UCLAMP_MAX,        0x00090008) \
X(RES_CGRP_REL_CPU_WEIGHT,    0x00090009) \
X(RES_CGRP_HIGH_MEM,          0x0009000a) \
X(RES_CGRP_MAX_MEM,           0x0009000b) \
X(RES_CGRP_LOW_MEM,           0x0009000c) \
X(RES_CGRP_MIN_MEM,           0x0009000d) \
X(RES_CGRP_SWAP_MAX_MEMORY,   0x0009000e) \
X(RES_CGRP_IO_WEIGHT,         0x0009000f) \
X(RES_CGRP_BFQ_IO_WEIGHT,     0x00090010) \
X(RES_CGRP_CPU_LATENCY,       0x00090011) \
X(FOCUSED_CGROUP_IDENTIFIER,  0x00000004) \
/* Cluster and Core Configurations */     \
X(CLUSTER_LITTLE_ALL_CORES,   0x00000000) \
X(CLUSTER_LITTLE_CORE_0,      0x00000001) \
X(CLUSTER_LITTLE_CORE_1,      0x00000002) \
X(CLUSTER_LITTLE_CORE_2,      0x00000003) \
X(CLUSTER_LITTLE_CORE_3,      0x00000004) \
X(CLUSTER_BIG_ALL_CORES,      0x00000100) \
X(CLUSTER_BIG_CORE_0,         0x00000101) \
X(CLUSTER_BIG_CORE_1,         0x00000102) \
X(CLUSTER_BIG_CORE_2,         0x00000103) \
X(CLUSTER_BIG_CORE_3,         0x00000104) \
X(CLUSTER_PLUS_ALL_CORES,     0x00000200) \
X(CLUSTER_PLUS_CORE_0,        0x00000201) \
X(CLUSTER_PLUS_CORE_1,        0x00000202) \
X(CLUSTER_PLUS_CORE_2,        0x00000203) \
X(CLUSTER_PLUS_CORE_3,        0x00000204) \
/* misc */                                \
X(DEFAULT_SIGNAL_TYPE,        0x00000000) \

enum ResCodesDef {
#define X(name, value) name = value,
    RES_CODE_LIST
#undef X
};

typedef struct {
    const char* name;
    uint32_t code;
} ResPair;

static ResPair resCodeMapping[] = {
#define X(name, value) {#name, (uint32_t)value,},
    RES_CODE_LIST
#undef X
};

static uint32_t getResCodeFromString(const char* strCode, int8_t* found) {
    int32_t size = sizeof(resCodeMapping) / sizeof(resCodeMapping[0]);
    for(int32_t i = 0; i < size; i++) {
        if(strcmp(resCodeMapping[i].name, strCode) == 0) {
            *found = true;
            return resCodeMapping[i].code;
        }
    }

    return 0;
}

#endif
