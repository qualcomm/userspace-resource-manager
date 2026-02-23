# Include directories for the entire project
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include)
include_directories(${CMAKE_SOURCE_DIR}/modula/Components/Include)
include_directories(${CMAKE_SOURCE_DIR}/modula/Common/Include)
include_directories(${CMAKE_SOURCE_DIR}/modula/CoreModules/Include)
include_directories(${CMAKE_SOURCE_DIR}/extensions/Include)
include_directories(${CMAKE_SOURCE_DIR}/resource-tuner/core/Include)
include_directories(${CMAKE_SOURCE_DIR}/resource-tuner/signals/Include/)
include_directories(${CMAKE_SOURCE_DIR}/resource-tuner/dbus-modules/Include/)

# Single shared library that contains parser + classifier + netlink glue
add_library(ContextualClassifier SHARED
    ContextualClassifier.cpp
    NetLinkComm.cpp
)

set_target_properties(ContextualClassifier PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

target_include_directories(ContextualClassifier
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

# floret detection and USE_FLORET macro using pkg-config
option(ENABLE_FLORET "Build ML inference with floret support" ON)

set(FLORET_FOUND FALSE)
set(FLORET_LIBRARIES "")
set(FLORET_INCLUDE_DIRS "")

if(ENABLE_FLORET)
    # Use pkg-config to find floret
    pkg_check_modules(PC_FLORET QUIET floret)
    if(PC_FLORET_FOUND)
        message(STATUS "floret not found via pkg-config")
        set(FLORET_FOUND TRUE)
        set(FLORET_LIBRARIES ${PC_FLORET_LIBRARIES})
        set(FLORET_INCLUDE_DIRS ${PC_FLORET_INCLUDE_DIRS})
    endif()
endif()

if(FLORET_FOUND)
    message(STATUS "floret found via pkg-config, building MLInference with USE_FLORET=1")
    add_definitions(-DUSE_FLORET=1)

    # Define the ML inference library that actually uses FLORET
    add_library(ml_inference_lib SHARED
        FeatureExtractor.cpp
        FeaturePruner.cpp
        MLInference.cpp
    )

    set_target_properties(ml_inference_lib PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
    )

    target_link_libraries(ContextualClassifier PRIVATE ml_inference_lib)

    target_link_libraries(ml_inference_lib
        PRIVATE
            ${FLORET_LIBRARIES}
    )

    target_include_directories(ml_inference_lib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/Include
            ${FLORET_INCLUDE_DIRS}
    )
else()
    message(STATUS "floret not found via pkg-config or ENABLE_FLORET=OFF — falling back to base Inference only")
endif()

# Installation rules
install(TARGETS ContextualClassifier DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(FLORET_FOUND)
    install(TARGETS ml_inference_lib DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES ${CMAKE_SOURCE_DIR}/contextual-classifier/Artifacts/floret_model_supervised.bin DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/urm/classifier)
endif()

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Configs/
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/urm/classifier/
  FILES_MATCHING
  PATTERN "*.txt"
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
