// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

/*!
 * \file SignalInternal.h
 */

#ifndef SIGNAL_INTERNAL_H
#define SIGNAL_INTERNAL_H

#include "ErrCodes.h"

/**
 * @brief Named indices into SignalInfo::mExtraAttrs.
 */
enum SignalExtraAttrIndex {
    SIGNAL_EXTRA_ATTR_FPS    = 0, //!< Frames per second
    SIGNAL_EXTRA_ATTR_HEIGHT = 1, //!< Frame height in pixels
    SIGNAL_EXTRA_ATTR_WIDTH  = 2, //!< Frame width in pixels
    SIGNAL_EXTRA_ATTRS_COUNT = 3  //!< Total number of extra attributes
};

/**
 * @brief Internal API for submitting Signal Requests for Processing
 * @details URM Modules and Extensions can directly use this API to submit Requests rather than
 *          using the Client Interface.
 */
ErrCode submitSignalRequest(void* clientReq);

/**
 * @brief Internal API for Acquiring a Signal
 * @details URM Modules and Extensions can directly use this API to submit Requests rather than
 *          using the Client Interface.
 */
int64_t acquireSignal(uint32_t sigId,
                      uint32_t sigType,
                      pid_t incomingPID,
                      pid_t incomingTID,
                      int32_t numFilterArgs = 0,
                      uint32_t* filterArgs = nullptr,
                      int32_t numArgs = 0,
                      uint32_t* args = nullptr);
#endif
