// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

/*!
 * \file  ErrCodes.h
 */

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include <cstdint>

/**
 * @enum ErrCode
 * @brief Custom Error Codes used by Resource Tuner APIs and Internal Functions.
 * @details These Error Codes should be used in conjunction with the Macros
 *          RC_IS_OK and RC_IS_NOTOK to check for Success or Failure.
 */
enum ErrCode {
    RC_REQ_SUBMISSION_FAILURE = -1,
    RC_SUCCESS,
    RC_MODULE_INIT_FAILURE,
    RC_FILE_NOT_FOUND,
    RC_YAML_PARSING_ERROR,
    RC_YAML_INVALID_SYNTAX,
    RC_REQUEST_SANITY_FAILURE,
    RC_REQUEST_VERIFICATION_FAILURE,
    RC_REQUEST_NOT_FOUND,
    RC_DUPLICATE_REQUEST,
    RC_MEMORY_ALLOCATION_FAILURE,
    RC_MEMORY_POOL_ALLOCATION_FAILURE,
    RC_MEMORY_POOL_BLOCK_RETRIEVAL_FAILURE,
    RC_PROP_PARSING_ERROR,
    RC_PROP_NOT_FOUND,
    RC_BAD_ARG,
    RC_INVALID_VALUE,
    RC_SOCKET_CONN_NOT_INITIALIZED,
    RC_SOCKET_OP_FAILURE,
    RC_SOCKET_FD_READ_FAILURE,
    RC_SOCKET_FD_WRITE_FAILURE,
    RC_SOCKET_FD_CLOSE_FAILURE,
    RC_REQUEST_DESERIALIZATION_FAILURE,
    RC_REQUEST_PARSING_FAILED,
    RC_RESOURCE_NOT_SUPPORTED,
    RC_WORKER_THREAD_ASSIGNMENT_FAILURE,
    RC_LOGICAL_TO_PHYSICAL_GEN_FAILED,
    RC_CGROUP_CREATION_FAILURE,
    RC_DBUS_COMM_FAIL,
};

#define RC_IS_OK(rc) ({          \
    int8_t retval;               \
    retval = (rc == RC_SUCCESS); \
    retval;                      \
})

#define RC_IS_NOTOK(rc) ({       \
    int8_t retval;               \
    retval = !RC_IS_OK(rc);      \
    retval;                      \
})

#endif
