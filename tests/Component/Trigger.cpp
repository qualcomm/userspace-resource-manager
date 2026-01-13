// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <cstdint>
#include <iostream>

#include "TestAggregator.h"

int32_t main(int32_t argc, const char* argv[]) {
    std::vector<ComponentTest> allTests = TestAggregator::getAllTests();

    for(ComponentTest test: allTests) {
        test();
    }

    return 0;
}
