cmake_minimum_required(VERSION 3.16)

set(CE_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CE_ROOT_DIR "${CE_RUNTIME_ROOT}")
set(CE_RUNTIME_SOURCE_DIR "${CE_RUNTIME_ROOT}/source")
set(CE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(CE_GENERATED_BINDINGS_DIR "${CE_GENERATED_DIR}/bindings")
set(CE_DATA_FILE_NAME "data.tcf")

include("${CE_RUNTIME_ROOT}/cmake/options.cmake")

project(ce_runtime VERSION 1.0.0 LANGUAGES C CXX)

include("${CE_RUNTIME_ROOT}/cmake/compiler.cmake")
include("${CE_RUNTIME_ROOT}/cmake/dependencies.cmake")
include("${CE_RUNTIME_ROOT}/cmake/generated_files.cmake")
include("${CE_RUNTIME_ROOT}/cmake/sources.cmake")
include("${CE_RUNTIME_ROOT}/cmake/engine.cmake")