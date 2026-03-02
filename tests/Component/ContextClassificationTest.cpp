// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

/**
 * These tests validate the machine learning-based application classification
 * functionality. The test suite spawns various applications and verifies that
 * the ML model correctly classifies them into predefined categories:
 * - APP (General applications)
 * - BROWSER (Web browsers)
 * - GAME (Gaming applications)
 * - MULTIMEDIA (Media players, editors)
 *
 * Test configuration is loaded from a YAML file containing application metadata
 * including executables, arguments, and expected classification labels.
 *
 * The tests mirror real-world usage where applications are launched and the
 * classifier must identify their category based on runtime characteristics.
 */

#include "ErrCodes.h"
#include "UrmPlatformAL.h"
#include "Utils.h"
#include "TestUtils.h"
#include "URMTests.h"
#include "MLInference.h"
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <vector>

// Test configuration paths and identifiers
#define TEST_CLASS "COMPONENT"
#define CLASSIFIER_CONFIGS_DIR "/etc/urm/classifier/"
#define TEST_CONFIG_PATH "/etc/urm/tests/configs/ClassificationAppPredConfig.yaml"

static const std::string FT_MODEL_PATH = CLASSIFIER_CONFIGS_DIR "floret_model_supervised.bin";

/**
 * Contains all metadata needed to spawn an application and validate
 * its classification against expected results. Each test application
 * is defined with:
 * - Execution parameters (binary path, arguments)
 * - Expected classification results (category name and numeric label)
 * - Test metadata (description, startup delay)
 */
struct TestAppConfig {
    std::string name;              ///< Application name (used in process spawn)
    std::string executable;        ///< Full path to executable binary
    std::vector<std::string> args; ///< Command-line arguments for application launch
    std::string expectedCategory;  ///< Human-readable expected category (e.g., "CC_MULTIMEDIA")
    int32_t expectedLabel;         ///< Numeric label for ML model validation (1=APP, 2=BROWSER, 3=GAME, 4=MULTIMEDIA)
    std::string description;       ///< Test case description for logging
    int startupDelayMs;           ///< Milliseconds to wait after spawn before classification
};

/**
 * Implements a lightweight YAML parser specifically designed for the test
 * configuration format. Extracts application metadata including executables,
 * arguments, and expected classification labels.
 *
 * The parser handles the following YAML structure:
 *
 * test_applications:
 *   - name: "app_name"
 *     executable: "/path/to/binary"
 *     args: ["arg1", "arg2"]
 *     expected_category: "CC_CATEGORY"
 *     expected_label: N
 *     description: "Description"
 *     startup_delay_ms: NNNN
 */
std::vector<TestAppConfig> loadAppConfigs(const std::string& configPath) {
    std::vector<TestAppConfig> apps;
    std::ifstream file(configPath);

    if (!file.is_open()) {
        return apps;
    }

    std::string line;
    TestAppConfig currentApp;
    bool inApp = false;

    // Parse line by line
    while (std::getline(file, line)) {
        // Trim leading whitespace
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        line = line.substr(first);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Detect start of new application entry
        if (line.find("- name:") != std::string::npos) {
            if (inApp) apps.push_back(currentApp);
            currentApp = TestAppConfig();
            size_t pos = line.find('"');
            if (pos != std::string::npos) {
                size_t end = line.find('"', pos + 1);
                currentApp.name = line.substr(pos + 1, end - pos - 1);
            }
            inApp = true;
        } else if (inApp) {
            // Parse application properties
            if (line.find("executable:") != std::string::npos) {
                size_t pos = line.find('"');
                if (pos != std::string::npos) {
                    size_t end = line.find('"', pos + 1);
                    currentApp.executable = line.substr(pos + 1, end - pos - 1);
                }
            } else if (line.find("args:") != std::string::npos) {
                // Parse argument array [arg1, arg2, ...]
                size_t start = line.find('[');
                size_t end = line.find(']');
                if (start != std::string::npos && end != std::string::npos) {
                    std::string argsStr = line.substr(start + 1, end - start - 1);
                    std::stringstream ss(argsStr);
                    std::string arg;
                    while (std::getline(ss, arg, ',')) {
                        size_t pos = arg.find('"');
                        if (pos != std::string::npos) {
                            size_t end = arg.find('"', pos + 1);
                            std::string trimmed = arg.substr(pos + 1, end - pos - 1);
                            currentApp.args.push_back(trimmed);
                        }
                    }
                }
            } else if (line.find("expected_category:") != std::string::npos) {
                size_t pos = line.find('"');
                if (pos != std::string::npos) {
                    size_t end = line.find('"', pos + 1);
                    currentApp.expectedCategory = line.substr(pos + 1, end - pos - 1);
                }
            } else if (line.find("expected_label:") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    currentApp.expectedLabel = std::stoi(line.substr(pos + 1));
                }
            } else if (line.find("description:") != std::string::npos) {
                size_t pos = line.find('"');
                if (pos != std::string::npos) {
                    size_t end = line.find('"', pos + 1);
                    currentApp.description = line.substr(pos + 1, end - pos - 1);
                }
            } else if (line.find("startup_delay_ms:") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    currentApp.startupDelayMs = std::stoi(line.substr(pos + 1));
                }
            }
        }
    }

    // Add final application if present
    if (inApp) apps.push_back(currentApp);
    return apps;
}


/*
 * Description:
 * This test suite validates the ML-based contextual classification system.
 * The classifier uses a Floret-based machine learning model to categorize
 * running applications into predefined classes based on their runtime behavior.
 *
 * Test Coverage:
 * - Model Loading and Initialization
 * - Application Spawning and Process Management
 * - ML Inference Execution
 * - Classification Accuracy Validation
 * - Resource Cleanup and Reset
 *
 * Classification Categories:
 * - CC_APP (1):        General applications (editors, productivity tools)
 * - CC_BROWSER (2):    Web browsers (Chrome, Firefox, etc.)
 * - CC_GAME (3):       Gaming applications
 * - CC_MULTIMEDIA (4): Media players and editors (VLC, MPV, etc.)
 *
 * Test Flow:
 * 1. Verify ML model file exists and is accessible
 * 2. Load test application configurations from YAML
 * 3. Initialize ML inference engine (Floret model)
 * 4. For each configured application:
 *    a. Verify executable exists
 *    b. Spawn process with configured arguments
 *    c. Wait for application startup (configurable delay)
 *    d. Run ML classification on process
 *    e. Compare predicted label with expected label
 *    f. Terminate process and collect results
 * 5. Generate summary report with pass/fail/skip counts
 * 6. List all failed and skipped applications for debugging
 *
 * Notes:
 * - Requires USE_FLORET to be defined at compile time
 * - Applications that fail to spawn are marked as SKIPPED, not FAILED
 * - Test fails if no applications pass or if any classification is incorrect
 * - All spawned processes are properly cleaned up (SIGTERM + waitpid)
 */

/**
 * API under test: MLInference::Classify
 * - Validates that the ML model correctly classifies various applications
 * - Tests multiple application categories (APP, BROWSER, GAME, MULTIMEDIA)
 * - Verifies classification accuracy against expected labels
 * - Handles missing executables gracefully (marked as SKIPPED)
 * - Provides detailed logging for debugging classification failures
 * Cross-Reference: Comprehensive application classification validation
 */
URM_TEST(TestApplicationClassification, {
    const std::string modelPath = FT_MODEL_PATH;

    // Wait for URM service to fully initialize and settle
    std::this_thread::sleep_for(std::chrono::milliseconds(1800));

    // Verify ML model file is present and accessible
    int64_t modelExists = AuxRoutines::fileExists(modelPath);
    if (modelExists <= 0) {
        std::cout << LOG_BASE << "Model not found, skipping test" << std::endl;
        E_ASSERT(false);
        return;
    }

    std::cout << LOG_BASE << "Testing application classification..." << std::endl;

    // Verify Floret support is enabled at compile time
    #ifndef USE_FLORET
        std::cout << LOG_BASE << "ERROR: USE_FLORET not defined!" << std::endl;
        std::cout << LOG_BASE << "Cannot create MLInference object" << std::endl;
        std::cout << LOG_BASE << "Rebuild with: cmake -DUSE_FLORET=ON .." << std::endl;
        E_ASSERT(false);
        return;
    #endif

    // Load test configurations from YAML file
    std::vector<TestAppConfig> apps = loadAppConfigs(TEST_CONFIG_PATH);

    if (apps.empty()) {
        std::cout << LOG_BASE << "ERROR: No applications found in config file" << std::endl;
        std::cout << LOG_BASE << "Config path: " << TEST_CONFIG_PATH << std::endl;
        E_ASSERT(false);
        return;
    }

    std::cout << LOG_BASE << "Found " << apps.size() << " application(s) to test" << std::endl;

    // Initialize ML inference engine (Floret-based model)
    Inference* inference = nullptr;
    #ifdef USE_FLORET
        inference = new MLInference(FT_MODEL_PATH);
    #else
        inference = new Inference(modelPath);
    #endif

    E_ASSERT((inference != nullptr));
    std::cout << LOG_BASE << "Inference created successfully" << std::endl;

    // Test result counters
    int passed = 0;
    int failed = 0;
    int skipped = 0;

    // Track failed and skipped applications for detailed reporting
    std::vector<std::string> failedApps;
    std::vector<std::string> skippedApps;

    // Execute classification test for each configured application
    for (const auto& app : apps) {
        std::cout << LOG_BASE << "\n=== Testing: " << app.name << " ===" << std::endl;
        std::cout << LOG_BASE << "Description: " << app.description << std::endl;
        std::cout << LOG_BASE << "Expected: " << app.expectedCategory 
                  << " (label=" << app.expectedLabel << ")" << std::endl;

        // Verify executable exists before attempting spawn
        if (!AuxRoutines::fileExists(app.executable)) {
            std::cout << LOG_BASE << "Executable not found: " << app.executable << std::endl;
            std::cout << LOG_BASE << "Status: SKIPPED" << std::endl;
            skipped++;
            skippedApps.push_back(app.name);
            continue;
        }

        // Fork and spawn application process
        pid_t pid = fork();
        if (pid == 0) {
            // Child process: redirect output to suppress application noise
            // This prevents terminal clutter from application stderr/stdout
            int devNull = open("/dev/null", O_WRONLY);
            if (devNull != -1) {
                dup2(devNull, STDERR_FILENO);
                dup2(devNull, STDOUT_FILENO);
                close(devNull);
            }

            // Build argument vector for execv
            std::vector<const char*> argv;
            argv.push_back(app.name.c_str());
            for (const auto& arg : app.args) {
                argv.push_back(arg.c_str());
            }
            argv.push_back(nullptr);

            // Execute application - if successful, this replaces the child process
            execv(app.executable.c_str(), const_cast<char* const*>(argv.data()));

            // If execv returns, it failed
            exit(EXIT_FAILURE);

        } else if (pid > 0) {
            // Parent process: wait for application to start and stabilize
            std::this_thread::sleep_for(std::chrono::milliseconds(app.startupDelayMs));

            // Verify process is still alive (didn't crash during startup)
            char commPath[256];
            snprintf(commPath, sizeof(commPath), "/proc/%d/comm", pid);

            if (!AuxRoutines::fileExists(commPath)) {
                std::cout << LOG_BASE << "Process died after spawn" << std::endl;
                std::cout << LOG_BASE << "Status: SKIPPED" << std::endl;
                skipped++;
                skippedApps.push_back(app.name);
                continue;
            }

            // Read process name from /proc for logging
            std::string comm = AuxRoutines::readFromFile(commPath);
            if (!comm.empty() && comm.back() == '\n') {
                comm.pop_back();
            }

            std::cout << LOG_BASE << "Process: " << comm << " (PID: " << pid << ")" << std::endl;

            // Call ML model inference to classify the running process
            std::cout << LOG_BASE << "Running model inference..." << std::endl;
            CC_TYPE predictedLabel = inference->Classify(pid);

            std::cout << LOG_BASE << "Predicted label: " << predictedLabel << std::endl;

            // Decode numeric prediction to human-readable category name
            const char* categoryName = "UNKNOWN";
            switch(predictedLabel) {
                case CC_APP:        categoryName = "APP"; break;
                case CC_BROWSER:    categoryName = "BROWSER"; break;
                case CC_GAME:       categoryName = "GAME"; break;
                case CC_MULTIMEDIA: categoryName = "MULTIMEDIA"; break;
                default:            categoryName = "APP";
            }

            std::cout << LOG_BASE << "Category: " << categoryName << std::endl;

            // Compare predicted label with expected label
            bool correct = (predictedLabel == app.expectedLabel);
            std::cout << LOG_BASE << "Match: " << (correct ? "YES" : "NO") << std::endl;

            // Cleanup: terminate spawned process
            // Use SIGTERM for graceful shutdown, then wait for process to exit
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);

            // Update test statistics
            if (correct) {
                std::cout << LOG_BASE << "Status: ✓ PASSED" << std::endl;
                passed++;
            } else {
                std::cout << LOG_BASE << "Status: ✗ FAILED" << std::endl;
                failed++;
                failedApps.push_back(app.name);
            }
        }
    }

    // Cleanup inference engine
    delete inference;

    // Generate comprehensive test summary
    std::cout << LOG_BASE << "\n\n===== TestApplicationClassification Summary ====" << std::endl;
    std::cout << LOG_BASE << "Total:   " << apps.size() << std::endl;
    std::cout << LOG_BASE << "Passed:  " << passed << std::endl;
    std::cout << LOG_BASE << "Failed:  " << failed << std::endl;
    std::cout << LOG_BASE << "Skipped: " << skipped << std::endl;

    // Display detailed list of failed applications for debugging
    if (!failedApps.empty()) {
        std::cout << LOG_BASE << "\n\n==================== Failed Applications ===" << std::endl;
        for (const auto& appName : failedApps) {
            std::cout << LOG_BASE << "  - " << appName << std::endl;
        }
    }

    // Display detailed list of skipped applications (missing executables or crashed)
    if (!skippedApps.empty()) {
        std::cout << LOG_BASE << "\n\n=================== Skipped Applications ===" << std::endl;
        for (const auto& appName : skippedApps) {
            std::cout << LOG_BASE << "  - " << appName << std::endl;
        }
    }

    // Assert test success criteria:
    // - At least one application must pass (validates model is working)
    // - No applications should fail (all classifications must be correct)
    E_ASSERT((passed > 0));
    E_ASSERT((failed == 0));

    std::cout << LOG_BASE << "Application classification tests PASSED" << std::endl;
})
