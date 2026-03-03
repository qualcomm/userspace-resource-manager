// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ErrCodes.h"
#include "UrmPlatformAL.h"
#include "Utils.h"
#include "TestUtils.h"
#include "URMTests.h"
#include "UrmAPIs.h"
#include <thread>

// Test configuration and paths
#define TEST_CLASS "INTEGRATION"
#define TEST_SUBCAT "CC_INTEGRATION"
#define CLASSIFIER_CONFIGS_DIR "/etc/urm/classifier/"

// Path to the Floret supervised learning model binary
static const std::string FT_MODEL_PATH = CLASSIFIER_CONFIGS_DIR "floret_model_supervised.bin";


/*
 * Description: Verifies that the URM classifier model file is properly loaded and accessible
 * at system startup.
 */
URM_TEST(TestModelLoads, {
    const std::string modelPath = FT_MODEL_PATH;

    // Wait for URM service to complete initialization and settle
    // This ensures all service components, including the classifier subsystem,
    // are fully initialized before we attempt to access the model file
    std::this_thread::sleep_for(std::chrono::milliseconds(1800));

    // Verify model file exists and is accessible
    // fileExists() returns file size if successful, -1 if file not found or inaccessible
    int64_t modelExists = AuxRoutines::fileExists(modelPath);

    // Log the result for debugging
    if (modelExists > 0) {
        std::cout << LOG_BASE << "Model file found at: " << modelPath << std::endl;
        std::cout << LOG_BASE << "Model file size: " << modelExists << " bytes" << std::endl;
    } else {
        std::cout << LOG_BASE << "ERROR: Model file not found or inaccessible" << std::endl;
        std::cout << LOG_BASE << "Expected path: " << modelPath << std::endl;
        std::cout << LOG_BASE << "Verify model deployment and file permissions" << std::endl;
    }

    // Assert that model file exists and is accessible (size > 0)
    E_ASSERT((modelExists > 0));
})

/**
 * API under test: Tune / Untune
 * - Spawns GStreamer and verifies that the per-app config
 *   correctly tunes the resource node when GStreamer is running
 * - Verifies resource value is set when GStreamer is active
 * - Verifies resource value resets once the request expires
 */
URM_TEST(TestGstreamerPerAppConfigValidated, {
    const std::string modelPath = FT_MODEL_PATH;

    // Wait for service to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(1800));

    // Verify model file exists
    int64_t modelExists = AuxRoutines::fileExists(modelPath);
    if (modelExists <= 0) {
        std::cout << LOG_BASE << "Model not found, skipping test" << std::endl;
        E_ASSERT(false);
        return;
    }

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
        std::cout << LOG_BASE << "Executable not found: /usr/bin/gst-launch-1.0" << std::endl;
        std::cout << LOG_BASE << "Status: SKIPPED" << std::endl;
        return;
    }

    // Spawn GStreamer
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: redirect output to suppress application noise
        int devNull = open("/dev/null", O_WRONLY);
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
 * - Spawns vi text editor and verifies that the per-app config
 *   correctly tunes the resource node when vi is running
 * - Verifies resource value is set when vi is active
 * - Verifies resource value resets once the request expires
 */
URM_TEST(TestViPerAppConfigValidated, {
    const std::string modelPath = FT_MODEL_PATH;

    // Wait for service to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(1800));

    // Verify model file exists
    int64_t modelExists = AuxRoutines::fileExists(modelPath);
    if (modelExists <= 0) {
        std::cout << LOG_BASE << "Model not found, skipping test" << std::endl;
        E_ASSERT(false);
        return;
    }

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
        std::cout << LOG_BASE << "Executable not found: " <<"/usr/bin/vi"<< std::endl;
        std::cout << LOG_BASE << "Status: SKIPPED" << std::endl;

        // return from this Test case if not .exe not exist
        return;
    }

    // Spawn vi
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: redirect output to suppress application noise
        int devNull = open("/dev/null", O_WRONLY);
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
