// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

/*!
 * \file RestuneInternal.h
 */

#ifndef FRAMEWORK_INTERNAL_H
#define FRAMEWORK_INTERNAL_H

#include "ErrCodes.h"
#include "Request.h"
#include "RequestQueue.h"
#include "ThreadPool.h"
#include "CocoTable.h"
#include "RateLimiter.h"
#include "RequestManager.h"
#include "ResourceRegistry.h"
#include "PropertiesRegistry.h"
#include "RestuneInternal.h"
#include "TargetRegistry.h"

/**
 * @brief Submit a Resource Provisioning Request from a Client for processing.
 * @details Note: This API acts an interface for other Resource Tuner components like Signals
 *          to submit a Resource Provisioning Request to the Resource Tuner Server, and
 *          subsequently provision the desired Resources.
 * @param request A buffer holding the Request.
 */
void submitResProvisionReqMsg(void* request);

void submitResProvisionRequest(Request* request, int8_t isVerified);

/**
 * @brief Gets a property from the Config Store.
 * @details Note: This API is meant to be used internally, i.e. by other Resource Tuner modules like Signals
 *          and not the End-Client Directly. Client Facing APIs are provided in Core/Client/APIs/
 * @param prop Name of the Property to be fetched.
 * @param buffer A buffer to hold the result, i.e. the property value corresponding to the specified name.
 * @param defValue Value to return in case a property with the specified Name is not found in the Config Store
 * @return size_t: Number of bytes written to the result buffer.
 */

size_t submitPropGetRequest(void* request, std::string& result);

__attribute__((visibility("default"))) size_t submitPropGetRequest(const std::string& propName, std::string& result, const std::string& defVal);

ErrCode translateToPhysicalIDs(Resource* resource);

#endif
