# Include directories for the entire project
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include)
include_directories(${CMAKE_SOURCE_DIR}/modula/Components/Include)
include_directories(${CMAKE_SOURCE_DIR}/modula/Common/Include)
include_directories(${CMAKE_SOURCE_DIR}/modula/CoreModules/Include)
include_directories(${CMAKE_SOURCE_DIR}/extensions/Include)
include_directories(${CMAKE_SOURCE_DIR}/resource-tuner/core/Include)
include_directories(${CMAKE_SOURCE_DIR}/resource-tuner/signals/Include/)

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
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

# fastText detection and USE_FASTTEXT macro
option(ENABLE_FASTTEXT "Build ML inference with fastText support" ON)

set(FASTTEXT_FOUND FALSE)
set(FASTTEXT_TARGET "")

if(ENABLE_FASTTEXT)
    # First try a CMake package
    find_package(fasttext QUIET)

    if(TARGET fasttext::fasttext)
        set(FASTTEXT_FOUND TRUE)
        set(FASTTEXT_TARGET fasttext::fasttext)
    elseif(TARGET fasttext)
        set(FASTTEXT_FOUND TRUE)
        set(FASTTEXT_TARGET fasttext)
    else()
        # Fallback: probe the linker directly for -lfasttext
        include(CheckLibraryExists)
        check_library_exists(fasttext fasttext_version "" FASTTEXT_LINKABLE)
        if(FASTTEXT_LINKABLE)
            set(FASTTEXT_FOUND TRUE)
            set(FASTTEXT_TARGET fasttext)
        endif()
    endif()
endif()

if(FASTTEXT_FOUND)
    message(STATUS "fastText found, building MLInference with USE_FASTTEXT=1")
    add_definitions(-DUSE_FASTTEXT=1)

    # Define the ML inference library that actually uses fastText
    add_library(ml_inference_lib STATIC
        FeatureExtractor.cpp
        FeaturePruner.cpp
        MLInference.cpp
    )

    target_link_libraries(ml_inference_lib
        PRIVATE
            ${FASTTEXT_TARGET}
    )

    target_include_directories(ml_inference_lib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/Include
    )
else()
    message(STATUS "fastText not found or ENABLE_FASTTEXT=OFF — falling back to base Inference only")
endif()

target_link_libraries(ContextualClassifier PUBLIC UrmClient)

# Installation rules
install(TARGETS ContextualClassifier DESTINATION lib)
if(FASTTEXT_FOUND)
    install(TARGETS ml_inference_lib DESTINATION lib)
    install(FILES ${CMAKE_SOURCE_DIR}/contextual-classifier/Artifacts/fasttext_model_supervised.bin DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/classifier)
endif()
install(FILES Configs/ignore-tokens.txt DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/classifier)
install(FILES Configs/classifier-blocklist.txt DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/classifier)
