// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "AuxRoutines.h"

/*
 * TEST RESOURCES DESCRIPTION:
 * |-------------------------------------|-------------|---------|-----------|-----------|---------|---------------|----------------|---------------|
 * |               Name                  |   ResType   | ResCode | Def Value | ApplyType | Enabled | Permissions   | High Threshold | Low Threshold |
 * |-------------------------------------|-------------|---------|-----------|-----------|---------|---------------|----------------|---------------|
 * |         sched_util_clamp_min        |     ff      |   00    |    300    |   global  |  True   | [third_party] |      1024      |      0        |
 * |         sched_util_clamp_max        |     ff      |   01    |    684    |   global  |  True   | [third_party] |      1024      |      0        |
 * |         scaling_min_freq            |     ff      |   02    |    107    |   global  |  True   | [third_party] |      1024      |      0        |
 * |         scaling_max_freq            |     ff      |   03    |    114    |   global  |  True   | [third_party] |      2048      |      0        |
 * |         target_test_resource1       |     ff      |   04    |    240    |   global  |  True   | [system]      |      400       |      0        |
 * |         target_test_resource2       |     ff      |   05    |    333    |   global  |  True   | [third_party] |      6500      |      50       |
 * |         target_test_resource3       |     ff      |   06    |    4400   |   global  |  True   | [third_party] |      5511      |      4000     |
 * |         target_test_resource4       |     ff      |   07    |    516    |   global  |  False  | [third_party] |      900       |      300      |
 * |         target_test_resource5       |     ff      |   08    |    17     |   global  |  True   | [third_party] |      20        |      0        |
 * | cluster_type_resource_%d_cluster_id |     ff      |   000a  |    180    |   cluster |  True   | [third_party] |      2048      |      0        |
 * |-------------------------------------|-------------|---------|-----------|---------------------|---------------|----------------|---------------|
 */

static void SetUp() {
    // Make sure all the tests have a sane starting point

    AuxRoutines::writeToFile("/etc/urm/tests/nodes/sched_util_clamp_min.txt", "300");
    AuxRoutines::writeToFile("/etc/urm/tests/nodes/sched_util_clamp_max.txt", "684");
    AuxRoutines::writeToFile("/etc/urm/tests/nodes/scaling_min_freq.txt", "107");
    AuxRoutines::writeToFile("/etc/urm/tests/nodes/scaling_max_freq.txt", "114");
    AuxRoutines::writeToFile("/etc/urm/tests/nodes/target_test_resource1.txt", "240");
    AuxRoutines::writeToFile("/etc/urm/tests/nodes/target_test_resource2.txt", "333");
    AuxRoutines::writeToFile("/etc/urm/tests/nodes/target_test_resource3.txt", "4400");
    AuxRoutines::writeToFile("/etc/urm/tests/nodes/target_test_resource4.txt", "516");
    AuxRoutines::writeToFile("/etc/urm/tests/nodes/target_test_resource5.txt", "17");

    std::this_thread::sleep_for(std::chrono::seconds(3));
}

static std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    time_t nowC = std::chrono::system_clock::to_time_t(now);
    tm localTm = *localtime(&nowC);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%b %d %H:%M:%S", &localTm);
    return std::string(buffer);
}

#define LOG_START std::cout<<"\n["<<getTimestamp()<<"] Running Test: "<<__func__<<std::endl;
#define LOG_END std::cout<<"["<<getTimestamp()<<"] "<<__func__<<": Run Successful"<<std::endl;
#define LOG_BASE "["<<getTimestamp()<<"] "<<__func__<<":"<<__LINE__<<") "
#define LOG_SKIP(message) std::cout<<"["<<getTimestamp()<<"] "<<__func__<<": Skipped, Reason: "<<message<<std::endl;

#define RUN_TEST(test)                                                              \
do {                                                                                \
    std::cout<<"["<<getTimestamp()<<"] Running Test: "<<#test<<std::endl;           \
    test();                                                                         \
    std::cout<<"["<<getTimestamp()<<"] "<<#test<<": Run Successful"<<std::endl;     \
    std::cout<<"--------------------------------------------------"<<std::endl;     \
} while(false);                                                                     \

#define RUN_INTEGRATION_TEST(test)                                                  \
do {                                                                                \
    SetUp();                                                                        \
    test();                                                                         \
} while(false);                                                                     \

#define C_STOI(value) ({                                                            \
    int32_t parsedValue = -1;                                                       \
    try {                                                                           \
        parsedValue = (int32_t)std::stoi(value);                                    \
    } catch(const std::invalid_argument& ex) {                                      \
        std::cerr<<"std::invalid_argument::what(): " << ex.what()<<std::endl;       \
    } catch(const std::out_of_range& ex) {                                          \
        std::cerr<<"std::out_of_range::what(): " << ex.what()<<std::endl;           \
    }                                                                               \
    parsedValue;                                                                    \
});

#define GENERATE_RESOURCE_ID(resType, resCode) ({                                   \
    uint32_t resourceBitmap = 0;                                                    \
    resourceBitmap |= (1 << 31);                                                    \
    resourceBitmap |= ((uint32_t)resCode);                                          \
    resourceBitmap |= ((uint32_t)resType << 16);                                    \
    resourceBitmap;                                                                 \
})

#define C_ASSERT(cond)                                                                                                 \
    if(cond == false) {                                                                                                \
        std::cerr<<"["<<getTimestamp()<<"] Assertion failed at line [" << __LINE__ << "]: "<<#cond<<std::endl;         \
        std::cerr<<"["<<getTimestamp()<<"] Test: ["<<__func__<<"] Failed, Terminating Suite\n"<<std::endl;             \
        exit(EXIT_FAILURE);                                                                                            \
    }                                                                                                                  \

#define E_ASSERT(cond)                                                                                                                \
    if(cond == false) {                                                                                                               \
        std::cerr<<"["<<getTimestamp()<<"] Assertion failed at line [" << __FILE__<<":"<<__LINE__ << "]: "<<#cond<<std::endl;         \
        std::cerr<<"["<<getTimestamp()<<"] Test: ["<<__func__<<"] Failed\n"<<std::endl;                                               \
        throw std::runtime_error("Test Failed");                                                                                      \
    }                                                                                                                                 \

#define C_ASSERT_NEAR(val1, val2, tol)                                                                      \
    if(std::fabs((val1) - (val2)) > (tol)) {                                                                \
        std::cerr<<"["<<getTimestamp()<<"] Condition Check on line:["<<__LINE__<<"]  failed"<<std::endl;    \
        std::cerr<<"["<<getTimestamp()<<"] Test: ["<<__func__<<"] Failed, Terminating Suite\n"<<std::endl;  \
        exit(EXIT_FAILURE);                                                                                 \
    }                                                                                                       \

#endif
