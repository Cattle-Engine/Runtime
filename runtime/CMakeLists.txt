cmake_minimum_required(VERSION 3.16)

set(EXPORT_COMPILE_COMMANDS ON);
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
    message(FATAL_ERROR "No toolchain file specified! Configure with -DCMAKE_TOOLCHAIN_FILE=<path>")
endif()

set(CE_RUNTIME_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

project(ce_runtime VERSION 1.0.0 LANGUAGES C CXX)
find_package(Git QUIET)

if (GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CE_RUNTIME_SOURCE_DIR}
        OUTPUT_VARIABLE CE_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CE_RUNTIME_SOURCE_DIR}
        OUTPUT_VARIABLE CE_GIT_HASH_FULL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CE_RUNTIME_SOURCE_DIR}
        OUTPUT_VARIABLE CE_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CE_RUNTIME_SOURCE_DIR}
        OUTPUT_VARIABLE CE_GIT_TAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY ${CE_RUNTIME_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_STATUS STREQUAL "")
        set(CE_GIT_ISDIRTY "false")
    else()
        set(CE_GIT_ISDIRTY "true")
    endif()
else()
    set(CE_GIT_HASH "unknown")
    set(CE_GIT_HASH_FULL "unknown")
    set(CE_GIT_BRANCH "unknown")
    set(CE_GIT_TAGS "unknown")
    set(CE_GIT_ISDIRTY "unknown")
endif()

configure_file(
    ${CE_RUNTIME_SOURCE_DIR}/include/git_version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/git_version.hpp
    @ONLY
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)