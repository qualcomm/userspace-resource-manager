# tests/contextual-classifier/CMakeLists.txt
if(BUILD_TESTS)

    # ----------------------------------------
    # Report directories & env (same pattern as resource-tuner)
    # ----------------------------------------
    set(TEST_REPORT_DIR "${CMAKE_BINARY_DIR}/test-reports")
    file(MAKE_DIRECTORY "${TEST_REPORT_DIR}")
    set(TEST_REPORT_ENV "TEST_REPORT_DIR=${TEST_REPORT_DIR}")

    # Path to test framework (mini.hpp, testrunner.cpp, etc.)
    set(FRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../framework")

    # ----------------------------------------
    # ContextualClassifierIntegrationTests
    # ----------------------------------------
    add_executable(ContextualClassifierIntegrationTests
        ${CMAKE_CURRENT_SOURCE_DIR}/Integration/IntegrationTests.cpp
        # If you are using a shared runner TU (like testrunner.cpp), uncomment:
        # ${FRAMEWORK_DIR}/testrunner.cpp
    )

    target_include_directories(ContextualClassifierIntegrationTests PRIVATE
        ${FRAMEWORK_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Integration
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(ContextualClassifierIntegrationTests PRIVATE
        # UrmAuxUtils
        # UrmExtAPIs
        # RestuneCore
        # pthread
        # yaml
    )

    # Mini.hpp default knobs (match your style)
    target_compile_definitions(ContextualClassifierIntegrationTests PRIVATE
        MTEST_DEFAULT_OUTPUT_TAP=0
        # MTEST_ENABLE_EXCEPTIONS=1
    )

    # Install the test runner next to other test executables
    install(TARGETS ContextualClassifierIntegrationTests
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # ----------------------------------------
    # CTest registration (auto emit reports)
    # ----------------------------------------
    add_test(NAME ContextualClassifierIntegration_All
             COMMAND ContextualClassifierIntegrationTests
                     --report-json=${TEST_REPORT_DIR}/ctx_integration_all.json
                     --report-junit=${TEST_REPORT_DIR}/ctx_integration_all.junit.xml
                     --report-md=${TEST_REPORT_DIR}/ctx_integration_all.md)

    set_tests_properties(ContextualClassifierIntegration_All PROPERTIES
        ENVIRONMENT "${TEST_REPORT_ENV};NO_COLOR="
    )

endif()
