// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef URM_TEST_AGGREGATOR_H
#define URM_TEST_AGGREGATOR_H

#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

#define BROKEN 901

enum TestStatus {
    NOT_RUN,
    PASSED,
    FAILED,
    SKIPPED,
};

typedef void (*URMTestCase)(void);
typedef struct {
    URMTestCase testCallback;
    std::string classLabel;
    TestStatus status;
    std::string comment;
    int32_t flags;
} URMTest;

class TestAggregator {
private:
    static uint32_t mTestsCount;
    static std::map<std::string, URMTest> mTests;

    static int32_t mPassCount;
    static int32_t mFailCount;
    static int32_t mSkipCount;

public:
    TestAggregator(URMTestCase testCb,
                   const std::string& name,
                   const std::string& testCat,
                   const std::string& tag);

    static int32_t runAll(const std::string& className);
    static void addPass(const std::string& name, const std::string& testCat);
    static void addFail(const std::string& name, const std::string& testCat);
    static void addSkip(const std::string& name, const std::string& testCat);
};

#define URM_TEST(testCallback, ...)                                               \
    static void testCallback(void) {                                              \
        try {                                                                     \
            LOG_START                                                             \
            __VA_ARGS__                                                           \
            LOG_END                                                               \
            TestAggregator::addPass(__func__, TEST_SUBCAT);                       \
        } catch(const std::exception& e) {                                        \
            TestAggregator::addFail(__func__, TEST_SUBCAT);                       \
        }                                                                         \
    }                                                                             \
    static TestAggregator CONCAT(aggregate, __LINE__) (testCallback,              \
                                                       #testCallback,             \
                                                       TEST_SUBCAT,               \
                                                       TEST_CLASS);               \

#define URM_TEST_P(testCallback, param, ...)                                      \
    static void testCallback(void) {                                              \
        try {                                                                     \
            if(param != BROKEN) {                                                 \
                LOG_START                                                         \
                __VA_ARGS__                                                       \
                LOG_END                                                           \
                TestAggregator::addPass(__func__, TEST_SUBCAT)                    \
            }                                                                     \
        } catch(const std::exception& e) {                                        \
            TestAggregator::addFail(__func__, TEST_SUBCAT);                       \
        }                                                                         \
    }                                                                             \
    static TestAggregator CONCAT(aggregate, __LINE__) (testCallback, TEST_CLASS); \

#define SKIP                                                                      \
    TestAggregator::addSkip(__func__, TEST_SUBCAT);                               \
    return;                                                                       \

#define REGISTER_AND_TRIGGER_SUITE(name)                                          \
    int32_t main() {                                                              \
        return TestAggregator::runAll(name);                                      \
    }                                                                             \

#endif
