# Install URM Client Lib
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/APIs/*.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Socket/*.cpp")
add_library(UrmClient ${SOURCES})
set_target_properties(UrmClient PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_link_libraries(UrmClient PRIVATE UrmAuxUtils)
target_include_directories(UrmClient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APIs/Include)
target_include_directories(UrmClient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Comm/Socket/Include)
install(TARGETS UrmClient LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# URM CLI Client
add_executable(
    urmCli
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientCLI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/APIs/UrmAPIs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Comm/Socket/SocketClient.cpp
)
target_link_libraries(urmCli PRIVATE UrmAuxUtils)
target_include_directories(urmCli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APIs/Include)
target_include_directories(urmCli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Comm/Socket/Include)
install(TARGETS urmCli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/APIs/Include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Urm)
