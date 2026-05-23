// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <thread>

#include "Utils.h"
#include "UrmAPIs.h"
#include "ErrCodes.h"
#include "URMTests.h"
#include "TestUtils.h"
#include "UrmPlatformAL.h"

// Test configuration and paths
#define TEST_CLASS "INTEGRATION"
#define TEST_SUBCAT "CC_INTEGRATION"
#define CLASSIFIER_CONFIGS_DIR "/etc/urm/classifier/"

// Path to the Floret supervised learning model binary
static const std::string FT_MODEL_PATH = CLASSIFIER_CONFIGS_DIR "floret_model_supervised.bin";

/**
 * API under test: Tune / Untune
 * - Spawns GStreamer and verifies that correctly
 * - tunes the resource node when GStreamer is running
 * - Verifies resource value is set when GStreamer is active
 * - Verifies resource value resets once the request expires
 */
URM_TEST(TestGstreamerPerAppConfigValidated, {
    const std::string modelPath = FT_MODEL_PATH;

    // Wait for service to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(1800));

    std::string testResourceName = "/etc/urm/tests/nodes/target_test_resource1.txt";
    int32_t testResourceOriginalValue = 240;

    std::string value;
    int32_t originalValue, newValue;

    // Verify original resource value
    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    E_ASSERT((originalValue == testResourceOriginalValue));

    // Verify executable exists before attempting spawn
    if (!AuxRoutines::fileExists("/usr/bin/gst-launch-1.0")) {
        LOG_SKIP("Executable not found: /usr/bin/gst-launch-1.0");
        SKIP;
        return;
    }

    // Spawn GStreamer
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: redirect output to suppress application noise
        int32_t devNull = open("/dev/null", O_WRONLY);
        if (devNull != -1) {
            dup2(devNull, STDERR_FILENO);
            dup2(devNull, STDOUT_FILENO);
            close(devNull);
        }
        execl("/usr/bin/gst-launch-1.0", "gst-launch-1.0",
              "videotestsrc", "pattern=smpte", "!",
              "videoconvert", "!", "autovideosink", nullptr);
        exit(EXIT_FAILURE);

    } else if (pid > 0) {
        // Wait for GStreamer to start
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        // Verify process is alive
        char commPath[256];
        snprintf(commPath, sizeof(commPath), "/proc/%d/comm", pid);
        E_ASSERT(AuxRoutines::fileExists(commPath));

        // Read and log process name
        std::string comm = AuxRoutines::readFromFile(commPath);
        if (!comm.empty() && comm.back() == '\n') {
            comm.pop_back();
        }
        std::cout << LOG_BASE << "Process: " << comm << " (PID: " << pid << ")" << std::endl;

        // Verify resource value is set correctly
        std::this_thread::sleep_for(std::chrono::seconds(1));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
        E_ASSERT((newValue == 231));

        // Wait for request to expire and verify resource resets
        std::this_thread::sleep_for(std::chrono::seconds(10));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
        E_ASSERT((newValue == originalValue));

        // Cleanup
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
    }
})

/**
 * API under test: Tune / Untune
 * - Spawns vi text editor and verifies that correctly
 * - tunes the resource node when vi is running
 * - Verifies resource value is set when vi is active
 * - Verifies resource value resets once the request expires
 */
URM_TEST(TestViPerAppConfigValidated, {
    const std::string modelPath = FT_MODEL_PATH;

    // Wait for service to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(1800));

    std::string testResourceName = "/etc/urm/tests/nodes/target_test_resource1.txt";
    int32_t testResourceOriginalValue = 240;

    std::string value;
    int32_t originalValue, newValue;

    // Verify original resource value
    value = AuxRoutines::readFromFile(testResourceName);
    originalValue = C_STOI(value);
    std::cout << LOG_BASE << testResourceName << " Original Value: " << originalValue << std::endl;
    E_ASSERT((originalValue == testResourceOriginalValue));

    // Verify executable exists before attempting spawn
    if (!AuxRoutines::fileExists("/usr/bin/vi")) {
        LOG_SKIP("Executable not found: /usr/bin/vi");
        SKIP;
    }

    // Spawn vi
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: redirect output to suppress application noise
        int32_t devNull = open("/dev/null", O_WRONLY);
        if (devNull != -1) {
            dup2(devNull, STDERR_FILENO);
            dup2(devNull, STDOUT_FILENO);
            close(devNull);
        }
        execl("/usr/bin/vi", "vi", "-e", "-s", nullptr);
        exit(EXIT_FAILURE);

    } else if (pid > 0) {
        // Wait for vi to start
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        // Verify process is alive
        char commPath[256];
        snprintf(commPath, sizeof(commPath), "/proc/%d/comm", pid);
        E_ASSERT(AuxRoutines::fileExists(commPath));

        // Read and log process name
        std::string comm = AuxRoutines::readFromFile(commPath);
        if (!comm.empty() && comm.back() == '\n') {
            comm.pop_back();
        }
        std::cout << LOG_BASE << "Process: " << comm << " (PID: " << pid << ")" << std::endl;

        // Verify resource value is set correctly
        std::this_thread::sleep_for(std::chrono::seconds(1));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Configured Value: " << newValue << std::endl;
        E_ASSERT((newValue == 231));

        // Wait for request to expire and verify resource resets
        std::this_thread::sleep_for(std::chrono::seconds(10));
        value = AuxRoutines::readFromFile(testResourceName);
        newValue = C_STOI(value);
        std::cout << LOG_BASE << testResourceName << " Reset Value: " << newValue << std::endl;
        E_ASSERT((newValue == originalValue));

        // Cleanup
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
    }
})
