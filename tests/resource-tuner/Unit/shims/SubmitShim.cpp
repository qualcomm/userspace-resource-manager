// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


// tests/unit/shims/SubmitShim.cpp
// Provides a test-only implementation of submitResProvisionRequest that forwards
// to our mock (so we never pull in RequestHandler.cpp or the core).

#include "../../../resource-tuner/core/Include/RestuneInternal.h"

// The mock is defined in test_submitResProvisionRequest.cpp
extern void __mock_processIncomingRequest(Request* request, int8_t isValidated);

void submitResProvisionRequest(Request* request, int8_t isValidated) {
    __mock_processIncomingRequest(request, isValidated);
}

