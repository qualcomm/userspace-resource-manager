configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/CoreModules/Include/Config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/CoreModules/Include/Config.h
@ONLY)

file(GLOB SOURCES "Components/*.cpp"
                  "Common/*.cpp"
                  "CoreModules/*.cpp")

add_library(UrmAuxUtils ${SOURCES})
set_target_properties(UrmAuxUtils PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_link_libraries(UrmAuxUtils PRIVATE ${LIBYAML_LIBRARIES})
target_include_directories(UrmAuxUtils PRIVATE ${LIBYAML_INCLUDE_DIRS})
target_include_directories(UrmAuxUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Components/Include)
target_include_directories(UrmAuxUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Common/Include)
target_include_directories(UrmAuxUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/CoreModules/Include)
target_include_directories(UrmAuxUtils PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/CoreModules/Include)
install(TARGETS UrmAuxUtils LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/Common/Include/UrmPlatformAL.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Urm
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/Components/Include/Logger.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Urm
)
