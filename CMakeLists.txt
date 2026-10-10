cmake_minimum_required(VERSION 3.16)

project(ce_engine VERSION 1.0.0 LANGUAGES C CXX)

if(WIN32)
    set(CE_OS_NAME "windows")
elseif(APPLE)
    set(CE_OS_NAME "macos")
elseif(LINUX)
    set(CE_OS_NAME "linux")
else()
    set(CE_OS_NAME "unknown")
endif()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CE_ARCH)

if(CE_ARCH MATCHES "^(amd64|x86_64|x64)$")
    set(CE_ARCH "x86_64")
elseif(CE_ARCH MATCHES "^(aarch64|arm64)$")
    set(CE_ARCH "arm64")
elseif(CE_ARCH MATCHES "^(i[3-6]86|x86)$")
    set(CE_ARCH "x86")
endif()

set(CE_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/${CE_OS_NAME}/${CE_ARCH}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CE_BIN_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CE_BIN_DIR}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CE_BIN_DIR}/$<CONFIG>")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(runtime)
add_subdirectory(loader)