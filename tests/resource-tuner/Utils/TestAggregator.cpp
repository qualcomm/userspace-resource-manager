// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "TestAggregator.h"

std::vector<ComponentTest> TestAggregator::mTests {};

TestAggregator::TestAggregator(ComponentTest testCallback) {
    mTests.push_back(testCallback);
}

std::vector<ComponentTest> TestAggregator::getAllTests() {
    return mTests;
}
