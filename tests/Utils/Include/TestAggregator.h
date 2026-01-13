// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef RESTUNE_TEST_AGGREGATOR_H
#define RESTUNE_TEST_AGGREGATOR_H

#include <vector>

typedef void (*ComponentTest)(void);

class TestAggregator {
private:
    static std::vector<ComponentTest> mTests;

public:
    TestAggregator(ComponentTest test);

    static std::vector<ComponentTest> getAllTests();
};

#define REGISTER_TEST(testCallback) \
    static TestAggregator aggregate(testCallback);

#endif
