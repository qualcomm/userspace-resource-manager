// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ErrCodes.h"
#include "UrmPlatformAL.h"
#include "Utils.h"
#include "TestUtils.h"
#include "URMTests.h"

// Test configuration and paths
#define TEST_CLASS "INTEGRATION"
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
