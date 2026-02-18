// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef URM_TEST_AGGREGATOR_H
#define URM_TEST_AGGREGATOR_H

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

#define BROKEN 901

typedef void (*URMTestCase)(void);
typedef std::pair<URMTestCase, std::string> URMTest;

class TestAggregator {
private:
    static uint32_t mTestsCount;
    static std::vector<URMTest> mTests;
    static std::vector<std::string> mFailed;

public:
    TestAggregator(URMTestCase test, const std::string& tag);

    static std::vector<URMTest> getAllTests();
    static void addFail(const std::string& name);
    static int32_t runAll(const std::string& name);
};

#define URM_TEST(testCallback, ...)                                               \
    static void testCallback(void) {                                              \
        try {                                                                     \
            LOG_START                                                             \
            __VA_ARGS__                                                           \
            LOG_END                                                               \
        } catch(const std::exception& e) {                                        \
            TestAggregator::addFail(__func__);                                    \
        }                                                                         \
    }                                                                             \
    static TestAggregator CONCAT(aggregate, __LINE__) (testCallback, TEST_CLASS); \

#define URM_TEST_P(testCallback, param, ...)                                      \
    static void testCallback(void) {                                              \
        try {                                                                     \
            if(param != BROKEN) {                                                 \
                LOG_START                                                         \
                __VA_ARGS__                                                       \
                LOG_END                                                           \
            }                                                                     \
        } catch(const std::exception& e) {                                        \
            TestAggregator::addFail(__func__);                                    \
        }                                                                         \
    }                                                                             \
    static TestAggregator CONCAT(aggregate, __LINE__) (testCallback, TEST_CLASS); \

#define REGISTER_AND_TRIGGER_SUITE(name)                                          \
    int32_t main() {                                                              \
        return TestAggregator::runAll(name);                                      \
    }                                                                             \

#endif
