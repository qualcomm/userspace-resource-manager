configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/CoreModules/Include/Config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/CoreModules/Include/Config.h
@ONLY)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Components/*.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/Common/*.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/CoreModules/*.cpp")

add_library(UrmAuxUtils ${SOURCES})
set_target_properties(UrmAuxUtils PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_link_libraries(UrmAuxUtils PRIVATE ${LIBYAML_LIBRARIES})
target_include_directories(UrmAuxUtils PRIVATE ${LIBYAML_INCLUDE_DIRS})
target_include_directories(UrmAuxUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Components/Include)
target_include_directories(UrmAuxUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Common/Include)
target_include_directories(UrmAuxUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/CoreModules/Include)
target_include_directories(UrmAuxUtils PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/CoreModules/Include)
install(TARGETS UrmAuxUtils LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install Headers
set(HEADER_FILES "")
list(APPEND HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Include/UrmPlatformAL.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Include/Resource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Include/ErrCodes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Include/Utils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Components/Include/Logger.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Components/Include/DLManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Components/Include/SafeOps.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Components/Include/ClientEndpoint.h
  ${CMAKE_CURRENT_SOURCE_DIR}/CoreModules/Include/AuxRoutines.h
  ${CMAKE_CURRENT_SOURCE_DIR}/CoreModules/Include/UrmSettings.h
)

install(
  FILES ${HEADER_FILES}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Urm
)
