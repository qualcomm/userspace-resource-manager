# Mandatory Dependencies
pkg_check_modules(LIBYAML REQUIRED yaml-0.1)

# Optional Dependency: libsystemd
set(LIB_SYSTEMD_DEP_FOUND FALSE)
pkg_check_modules(PC_SYSTEMD QUIET systemd)
if(PC_SYSTEMD_FOUND)
  pkg_check_modules(SYSTEMD QUIET libsystemd)
  if(SYSTEMD_FOUND)
    set(LIB_SYSTEMD_DEP_FOUND TRUE)
  endif()
endif()

# Optional Modules
option(BUILD_TESTS "Testing" OFF)
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

if(LIB_SYSTEMD_DEP_FOUND)
  list(APPEND LINK_LIBS ${SYSTEMD_LIBRARIES})
  list(APPEND SOURCES dbus-modules/RestuneDBusInternal.cpp)
  if(BUILD_STATE_DETECTOR)
    list(APPEND SOURCES dbus-modules/StateDetector.cpp)
  endif()
else()
  list(APPEND SOURCES dbus-modules/RestuneDBusStubs.cpp)
endif()

add_library(RestuneCore ${SOURCES})
set_target_properties(RestuneCore PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_link_libraries(RestuneCore PUBLIC ${LINK_LIBS})

target_include_directories(RestuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/Include)
target_include_directories(RestuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/Server/Include)
target_include_directories(RestuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/signals/Include)
target_include_directories(RestuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/init/Include)
target_include_directories(RestuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dbus-modules/Include)

# libyaml headers
target_include_directories(RestuneCore PRIVATE ${LIBYAML_INCLUDE_DIRS})

# libsystemd headers
if(LIB_SYSTEMD_DEP_FOUND)
  target_include_directories(RestuneCore PRIVATE ${SYSTEMD_INCLUDE_DIRS})
endif()

# Install RestuneCore
install(TARGETS RestuneCore LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
