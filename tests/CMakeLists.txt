if(BUILD_TESTS)
    
    # Where test reports should be written (inside the build tree)
    set(TEST_REPORT_DIR "${CMAKE_BINARY_DIR}/test-reports")

    # Ensure the directory exists at build time (safe even if already there)
    file(MAKE_DIRECTORY "${TEST_REPORT_DIR}")

    # Optional: help the tests find a writable location through an env var
    set(TEST_REPORT_ENV "TEST_REPORT_DIR=${TEST_REPORT_DIR}")

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
# Source list for mini.hpp-based tests (add more .cpps here as you create them)
# IMPORTANT: Do NOT include Integration/IntegrationTests.cpp here; it belongs to the integration runner.
set(MINI_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/tests_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/ThreadPoolTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/TimerTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/SafeOpsTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/RequestQueueTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/RateLimiterTests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/Component/ParserTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/MiscTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/MemoryPoolTests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/Component/ExtensionIntfTests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/Component/DeviceInfoTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Component/RequestMapTests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/Component/CocoTableTests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/Component/ClientDataManagerTests.cpp
    # DO NOT list Integration/IntegrationTests.cpp here
)

# Create a single test runner binary that uses mini.hpp's built-in main()
add_executable(RestuneMiniTests ${MINI_TEST_SOURCES})

# Make the header visible to the test sources
target_include_directories(RestuneMiniTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)

# Link any production libraries your tests depend on (adjust as needed)
target_link_libraries(RestuneMiniTests PUBLIC
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
target_compile_definitions(RestuneMiniTests PRIVATE
    MTEST_DEFAULT_OUTPUT_TAP=0
    # MTEST_ENABLE_EXCEPTIONS=1
)

# Install the test runner like your other test executables
install(TARGETS RestuneMiniTests RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


# All Mini tests – emit reports every run
add_test(NAME RestuneMiniTests_All
         COMMAND RestuneMiniTests
                 --report-json=${TEST_REPORT_DIR}/mini_all.json
                 --report-junit=${TEST_REPORT_DIR}/mini_all.junit.xml
                 --report-md=${TEST_REPORT_DIR}/mini_all.md)


# (Optional) make XPASS fail the test run, useful in CI strict mode
# set_tests_properties(RestuneMiniTests_All PROPERTIES
#     PASS_REGULAR_EXPRESSION "Summary:.*failed=0"
# )
# set_tests_properties(RestuneMiniTests_Unit PROPERTIES
#     PASS_REGULAR_EXPRESSION "Summary:.*failed=0"
# )

set_tests_properties(RestuneMiniTests_All PROPERTIES
    ENVIRONMENT "${TEST_REPORT_ENV};NO_COLOR="
)


# --- RestuneIntegrationTests ---
# Build integration runner that exclusively compiles Integration/IntegrationTests.cpp
add_executable(RestuneIntegrationTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Integration/IntegrationTests.cpp
)

# Include directories (mini.hpp and any headers used)
target_include_directories(RestuneIntegrationTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/framework         # mini.hpp location
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

# DO NOT define MTEST_NO_MAIN here; mini.hpp should provide main()

# Install & register with CTest
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


endif()
