// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "TestUtils.h"
#include "CocoTable.h"
#include "URMTests.h"

#define TEST_CLASS "COMPONENT"

URM_TEST(TestCocoTableInsertRequest1, {
    E_ASSERT((CocoTable::getInstance()->insertRequest(nullptr) == false));
})

URM_TEST(TestCocoTableInsertRequest2, {
    Request* request = new Request;
    E_ASSERT((CocoTable::getInstance()->insertRequest(request) == false));
    delete request;
})

URM_TEST(TestCocoTableInsertRequest3, {
    Request* request = new Request;
    E_ASSERT((CocoTable::getInstance()->insertRequest(request) == false));
    delete request;
})
