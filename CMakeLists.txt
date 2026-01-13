cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_STANDARD 17 LANGUAGES CXX)
set(CMAKE_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_options(-Wall -Wextra)

project("URM")

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
# Custom Build Options
option(BUILD_CLASSIFIER "Classifier" OFF)
option(BUILD_TESTS "Testing" OFF)


add_subdirectory(${CMAKE_SOURCE_DIR}/configs)
add_subdirectory(${CMAKE_SOURCE_DIR}/modula)
add_subdirectory(${CMAKE_SOURCE_DIR}/client)
add_subdirectory(${CMAKE_SOURCE_DIR}/extensions)

# resource-tuner is a mandatory build
add_subdirectory(${CMAKE_SOURCE_DIR}/resource-tuner)

if(BUILD_TESTS)
include(CTest)       # sets BUILD_TESTING option
enable_testing()     # turns on CTest collection
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
endif()

# Create and install the urm executable
set(SERVER_EXECUTABLE "")
list(APPEND SERVER_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/urm.cpp
                              ${CMAKE_CURRENT_SOURCE_DIR}/resource-tuner/init/RestuneInit.cpp)

# build classifier conditionally
if(BUILD_CLASSIFIER)
  add_subdirectory(${CMAKE_SOURCE_DIR}/contextual-classifier)
  include_directories(${CMAKE_SOURCE_DIR}/modula/Components/Include)
  list(APPEND SERVER_EXECUTABLE
       ${CMAKE_CURRENT_SOURCE_DIR}/contextual-classifier/ContextualClassifierInit.cpp)
endif()

set(SERVER_LIBS "")
list(APPEND SERVER_LIBS
  UrmAuxUtils
  RestuneCore
  UrmExtAPIs
)

add_executable(urm ${SERVER_EXECUTABLE} ${CMAKE_SOURCE_DIR})
target_link_libraries(urm PUBLIC ${SERVER_LIBS})

# Install bin
install(TARGETS urm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/urm.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/urm.service
@ONLY)

# Install service
pkg_check_modules(PC_SYSTEMD QUIET systemd)
if(PC_SYSTEMD_FOUND)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/urm.service DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system/)
endif()
