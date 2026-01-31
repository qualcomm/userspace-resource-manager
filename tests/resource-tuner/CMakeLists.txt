if(BUILD_TESTS)
    
    # Where test reports should be written (inside the build tree)
    set(TEST_REPORT_DIR "${CMAKE_BINARY_DIR}/test-reports")

    # Ensure the directory exists at build time (safe even if already there)
    file(MAKE_DIRECTORY "${TEST_REPORT_DIR}")

    # Optional: help the tests find a writable location through an env var
    set(TEST_REPORT_ENV "TEST_REPORT_DIR=${TEST_REPORT_DIR}")
    
    
    # Put this near the top of the file (inside the if(BUILD_TESTS) block is fine)
    set(FRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../framework")

    # Install Test Configs
    file(GLOB testConfigs "${CMAKE_CURRENT_SOURCE_DIR}/Configs/*.yaml")
    install(
        FILES ${testConfigs}
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/urm/tests/configs/
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )

    # Install Custom Resource Nodes
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Configs/ResourceSysFsNodes/
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/urm/tests/nodes/
        FILES_MATCHING
        PATTERN "*.txt"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )

    add_library(RestuneTestUtils
                ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Baseline.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/Utils/TestAggregator.cpp)
    set_target_properties(RestuneTestUtils PROPERTIES VERSION 1.0.0 SOVERSION 1)
    target_link_libraries(RestuneTestUtils UrmAuxUtils)
    target_include_directories(RestuneTestUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Include)
    install(TARGETS RestuneTestUtils DESTINATION ${CMAKE_INSTALL_LIBDIR})

    add_library(RestunePlugin ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Setup.cpp)
    set_target_properties(RestunePlugin PROPERTIES VERSION 1.0.0 SOVERSION 1)
    target_link_libraries(RestunePlugin UrmExtAPIs)
    install(TARGETS RestunePlugin DESTINATION ${CMAKE_INSTALL_LIBDIR})

   
# --- Mini.hpp header-only unit/module/usecase tests ---
set(COMPONENT_TEST_SOURCES
    ${FRAMEWORK_DIR}/testrunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/ThreadPoolTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/TimerTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/SafeOpsTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/RequestQueueTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/RateLimiterTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/ParserTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/MiscTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/MemoryPoolTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/ExtensionIntfTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/DeviceInfoTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/RequestMapTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/CocoTableTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/ClientDataManagerTests.cpp
)

# Create a single test runner binary that uses mini.hpp's built-in main()
add_executable(RestuneComponentTests ${COMPONENT_TEST_SOURCES})

# Make the header visible to the test sources
target_include_directories(RestuneComponentTests PRIVATE
    ${FRAMEWORK_DIR}
)

# Link any production libraries your tests depend on (adjust as needed)
target_link_libraries(RestuneComponentTests PUBLIC
    UrmAuxUtils
    UrmExtAPIs
    RestuneCore
    RestuneTestUtils
    # If your tests need yaml, etc., add here:
    # ${LIBYAML_LIBRARIES}
    # pthread might be needed depending on your tests:
    # pthread
)

# (Optional) defines
target_compile_definitions(RestuneComponentTests PRIVATE
    MTEST_DEFAULT_OUTPUT_TAP=0
    # MTEST_ENABLE_EXCEPTIONS=1
)

# Install the test runner like your other test executables
install(TARGETS RestuneComponentTests RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


# All Mini tests – emit reports every run
add_test(NAME RestuneComponentTests_All
         COMMAND RestuneComponentTests
                 --report-json=${TEST_REPORT_DIR}/component_all.json
                 --report-junit=${TEST_REPORT_DIR}/component_all.junit.xml
                 --report-md=${TEST_REPORT_DIR}/component_all.md)

set_tests_properties(RestuneComponentTests_All PROPERTIES
    ENVIRONMENT "${TEST_REPORT_ENV};NO_COLOR="
)


# --- RestuneIntegrationTests ---
add_executable(RestuneIntegrationTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Integration/IntegrationTests.cpp
)

target_include_directories(RestuneIntegrationTests PRIVATE
    ${FRAMEWORK_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Integration       # local includes used by Integration tests
    # add other include dirs if your tests need them
)

# Link the test utilities / plugins your integration tests rely on
target_link_libraries(RestuneIntegrationTests PRIVATE
    UrmAuxUtils
    RestuneTestUtils
    UrmClient
    ${LIBYAML_LIBRARIES}
    # pthread if required:
    # pthread
)

install(TARGETS RestuneIntegrationTests RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


# All Integration tests – regenerate reports every run
add_test(NAME RestuneIntegrationTests_All
         COMMAND RestuneIntegrationTests
                 --report-json=${TEST_REPORT_DIR}/integration_all.json
                 --report-junit=${TEST_REPORT_DIR}/integration_all.junit.xml
                 --report-md=${TEST_REPORT_DIR}/integration_all.md)


set_tests_properties(RestuneIntegrationTests_All PROPERTIES
    ENVIRONMENT "${TEST_REPORT_ENV};NO_COLOR="
)

# ----------------------------------------
# all unit tests
# ----------------------------------------

# One main TU that provides main()
set(RESTUNE_UNIT_MAIN
    ${FRAMEWORK_DIR}/testrunner.cpp
)

# Test sources ++ the real file under test for getResConf
set(RESTUNE_UNIT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit/test_submitResProvisionRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit/test_getResConf.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit/shims/SubmitShim.cpp
    ${CMAKE_SOURCE_DIR}/resource-tuner/core/ResourceRegistry.cpp
)


# Create the binary
add_executable(RestuneUnitTests
    ${RESTUNE_UNIT_MAIN}
    ${RESTUNE_UNIT_SOURCES}
)

# Only the main TU should generate main(); the test TUs must not
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit/test_submitResProvisionRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit/test_getResConf.cpp
    PROPERTIES COMPILE_DEFINITIONS "MTEST_NO_MAIN=1"
)

# Include paths for tests and core headers
target_include_directories(RestuneUnitTests PRIVATE
    ${FRAMEWORK_DIR}
    ${CMAKE_SOURCE_DIR}/resource-tuner/core/Include
    ${CMAKE_SOURCE_DIR}/modula/Common/Include
)

# Link minimal libs only (do NOT link RestuneCore; shim+stubs avoid heavy initializers)
target_link_libraries(RestuneUnitTests PRIVATE
    UrmAuxUtils
    UrmExtAPIs
    RestuneTestUtils
    RestuneCore
)

# mini.hpp default behavior
target_compile_definitions(RestuneUnitTests PRIVATE
    MTEST_DEFAULT_OUTPUT_TAP=0
)

# Install & register with CTest
install(TARGETS RestuneUnitTests RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


add_test(NAME RestuneUnitTests_All
         COMMAND RestuneUnitTests
                 --report-json=${TEST_REPORT_DIR}/unit_all.json
                 --report-junit=${TEST_REPORT_DIR}/unit_all.junit.xml
                 --report-md=${TEST_REPORT_DIR}/unit_all.md)

set_tests_properties(RestuneUnitTests_All PROPERTIES
    ENVIRONMENT "${TEST_REPORT_ENV};NO_COLOR="
)

endif()
