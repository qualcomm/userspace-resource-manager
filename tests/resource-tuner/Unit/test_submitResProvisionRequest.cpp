// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// (Optional) If you use a dedicated main TU, add this at top:

#define MTEST_NO_MAIN 1
#include "../framework/mini.h"
#include <cstdint>

struct Request;
// (We will implement submitResProvisionRequest in a shim TU)
void submitResProvisionRequest(Request* request, int8_t isValidated);

// ------ Mock & bookkeeping (GLOBAL SCOPE) ------
struct RecordedCall {
    Request* req{};
    int8_t   flag{};
    int      times{};
};
static RecordedCall g_lastCall;

static void resetRecordedCall() { g_lastCall = RecordedCall{}; }

//  Move this OUT of the anonymous namespace so it has external linkage
void __mock_processIncomingRequest(Request* request, int8_t isValidated) {
    g_lastCall.req  = request;
    g_lastCall.flag = isValidated;
    g_lastCall.times += 1;
}

namespace {
    inline Request* makeNonNullRequestPtr() {
        static int stub;
        return reinterpret_cast<Request*>(&stub);
    }
}

// ---------------------- Tests ----------------------

MT_TEST(unit, Forwards_IsValidated_False, "restune") {
    resetRecordedCall();
    Request* req = makeNonNullRequestPtr();

    submitResProvisionRequest(req, /*isValidated*/ 0);

    MT_REQUIRE_EQ(ctx, g_lastCall.times, 1);
    MT_REQUIRE_EQ(ctx, g_lastCall.req,   req);
    MT_REQUIRE_EQ(ctx, g_lastCall.flag,  0);
}

MT_TEST(unit, Forwards_IsValidated_True, "restune") {
    resetRecordedCall();
    Request* req = makeNonNullRequestPtr();

    submitResProvisionRequest(req, /*isValidated*/ 1);

    MT_REQUIRE_EQ(ctx, g_lastCall.times, 1);
    MT_REQUIRE_EQ(ctx, g_lastCall.req,   req);
    MT_REQUIRE_EQ(ctx, g_lastCall.flag,  1);
}

MT_TEST(unit, Forwards_Null_Request_Pointer, "restune") {
    resetRecordedCall();

    submitResProvisionRequest(nullptr, /*isValidated*/ 1);

    MT_REQUIRE_EQ(ctx, g_lastCall.times, 1);
    MT_REQUIRE_EQ(ctx, g_lastCall.req,   static_cast<Request*>(nullptr));
    MT_REQUIRE_EQ(ctx, g_lastCall.flag,  1);
}

