if(BUILD_TESTS)
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
                ${CMAKE_CURRENT_SOURCE_DIR}/Utils/URMTests.cpp)
    set_target_properties(RestuneTestUtils PROPERTIES VERSION 1.0.0 SOVERSION 1)
    target_link_libraries(RestuneTestUtils UrmAuxUtils)
    target_include_directories(RestuneTestUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Include)
    install(TARGETS RestuneTestUtils DESTINATION ${CMAKE_INSTALL_LIBDIR})

    add_library(UrmTestPlugin ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Setup.cpp)
    set_target_properties(UrmTestPlugin PROPERTIES VERSION 1.0.0 SOVERSION 1)
    target_link_libraries(UrmTestPlugin UrmExtAPIs)
    install(TARGETS UrmTestPlugin DESTINATION ${CMAKE_INSTALL_LIBDIR}/urm)

    add_executable(
        RestuneComponentTests
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/MemoryPoolTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/ClientDataManagerTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/ParserTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/ExtensionIntfTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/MiscTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/RequestQueueTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/ThreadPoolTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/SafeOpsTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/RateLimiterTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/TimerTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/DeviceInfoTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/CocoTableTests.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/Component/RequestMapTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Component/Trigger.cpp)

    target_link_libraries(RestuneComponentTests PUBLIC UrmAuxUtils
                                                       UrmExtAPIs
                                                       RestuneCore
                                                       RestuneTestUtils)

    install(TARGETS RestuneComponentTests DESTINATION ${CMAKE_INSTALL_BINDIR})

    add_executable(RestuneIntegrationTests Integration/IntegrationTests.cpp)
    target_link_libraries(RestuneIntegrationTests PUBLIC UrmAuxUtils
                                                         UrmClient
                                                         RestuneTestUtils
                                                         ${LIBYAML_LIBRARIES})
    target_include_directories(RestuneIntegrationTests PRIVATE ${LIBYAML_INCLUDE_DIRS})
    install(TARGETS RestuneIntegrationTests RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

endif()
