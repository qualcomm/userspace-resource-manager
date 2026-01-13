# Mandatory Dependencies
pkg_check_modules(LIBYAML REQUIRED yaml-0.1)

# Optional Modules
option(BUILD_STATE_DETECTOR "Display Detector" OFF)

# Restune Core Lib
file(GLOB SOURCES "core/*.cpp"
                  "core/Server/*.cpp"
                  "signals/*.cpp"
                  "init/*.cpp")

set(LINK_LIBS "")
list(APPEND LINK_LIBS
  UrmAuxUtils
  UrmExtAPIs
  ${LIBYAML_LIBRARIES}
)

if(BUILD_STATE_DETECTOR)
  pkg_check_modules(SYSTEMD REQUIRED libsystemd)
  list(APPEND SOURCES extensions/StateDetector.cpp)
  list(APPEND LINK_LIBS ${SYSTEMD_LIBRARIES})
endif()

add_library(RestuneCore ${SOURCES})
set_target_properties(RestuneCore PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_include_directories(RestuneCore PRIVATE ${LIBYAML_INCLUDE_DIRS})
target_link_libraries(RestuneCore PUBLIC ${LINK_LIBS})

target_include_directories(RestuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/Include)
target_include_directories(RestuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/Server/Include)
target_include_directories(RestuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/signals/Include)
target_include_directories(RestuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/init/Include)
install(TARGETS RestuneCore LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

