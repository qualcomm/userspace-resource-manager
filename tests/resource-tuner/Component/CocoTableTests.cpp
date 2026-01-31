// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// CocoTableTests.cpp
#include "TestUtils.h"
#include "CocoTable.h"
#include "Request.h"        // Added: required for Request*
#include "TestAggregator.h"

#include <iostream>
#include <cstdint>

#define MTEST_NO_MAIN
#include "../framework/mini.h"


using namespace mtest;

// ---------------------------
// Suite: CocoTableTests
// Tag:  component-serial
// ---------------------------

MT_TEST(Component, InsertRequest1, "cocotable") {
    MT_REQUIRE_EQ(ctx, CocoTable::getInstance()->insertRequest(nullptr), false);
}

MT_TEST(Component, InsertNewRequestNoSetup, "cocotable") {
    Request* request = new Request;
    // If CocoTable requires preconditions (e.g., initialized pool, request properties),
    // this test expects insertion to fail without setup.
    MT_REQUIRE_EQ(ctx, CocoTable::getInstance()->insertRequest(request), false);
    delete request;
}

MT_TEST(Component, InsertRequestWithoutCocoNodes, "cocotable") {
    Request* request = new Request;

    // Original code hinted at MakeAlloc<std::vector<CocoNode*>>(1) but commented out.
    // Keeping the same semantics: without pre-allocating coco nodes / required structures,
    // insertion should fail.
    // MakeAlloc<std::vector<CocoNode*>>(1);
    MT_REQUIRE_EQ(ctx, CocoTable::getInstance()->insertRequest(request), false);

    delete request;
}

