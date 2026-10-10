cmake_minimum_required(VERSION 3.16)

set(CE_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CE_ROOT_DIR "${CE_RUNTIME_ROOT}")
set(CE_RUNTIME_SOURCE_DIR "${CE_RUNTIME_ROOT}/source")
set(CE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(CE_GENERATED_BINDINGS_DIR "${CE_GENERATED_DIR}/bindings")
set(CE_DATA_FILE_NAME "data.tcf")

include("${CE_RUNTIME_ROOT}/cmake/options.cmake")

project(ce_runtime VERSION 1.0.0 LANGUAGES C CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CE_IDL_GENERATOR "${CE_RUNTIME_ROOT}/../tools/build/idl.py")
set(CE_IDL_GENERATOR_SUPPORT_FILES
    "${CE_IDL_GENERATOR}"
    "${CE_RUNTIME_ROOT}/../tools/build/as_binding_gen.py"
    "${CE_RUNTIME_ROOT}/../tools/build/generator.py"
)

file(GLOB_RECURSE CE_IDL_INPUTS CONFIGURE_DEPENDS
    "${CE_RUNTIME_ROOT}/include/*.idl.yaml"
    "${CE_RUNTIME_SOURCE_DIR}/*.idl.yaml"
)

set(CE_GENERATED_BINDING_SOURCES)
set(CE_GENERATED_BINDING_METADATA)
set(CE_GENERATED_BINDING_OUTPUTS)

foreach(CE_IDL_FILE IN LISTS CE_IDL_INPUTS)
    get_filename_component(CE_IDL_NAME "${CE_IDL_FILE}" NAME_WLE)
    string(REGEX REPLACE "\\.idl$" "" CE_IDL_NAME "${CE_IDL_NAME}")

    set(CE_IDL_GENERATED_HEADER
        "${CE_GENERATED_BINDINGS_DIR}/${CE_IDL_NAME}.generated.hpp")
    set(CE_IDL_GENERATED_SOURCE
        "${CE_GENERATED_BINDINGS_DIR}/${CE_IDL_NAME}.generated.cpp")
    set(CE_IDL_GENERATED_METADATA
        "${CE_GENERATED_BINDINGS_DIR}/${CE_IDL_NAME}.generated.json")

    add_custom_command(
        OUTPUT
            "${CE_IDL_GENERATED_HEADER}"
            "${CE_IDL_GENERATED_SOURCE}"
            "${CE_IDL_GENERATED_METADATA}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CE_GENERATED_BINDINGS_DIR}"
        COMMAND "${Python3_EXECUTABLE}" "${CE_IDL_GENERATOR}"
            --yaml_file "${CE_IDL_FILE}"
            --output_header "${CE_IDL_GENERATED_HEADER}"
            --output_source "${CE_IDL_GENERATED_SOURCE}"
            --source_dir "${CE_RUNTIME_SOURCE_DIR}"
            --include_dir "${CE_RUNTIME_ROOT}/include"
        DEPENDS
            "${CE_IDL_FILE}"
            ${CE_IDL_GENERATOR_SUPPORT_FILES}
        WORKING_DIRECTORY "${CE_RUNTIME_ROOT}/../tools/build"
        COMMENT "Generating C++ binding from ${CE_IDL_FILE}"
        VERBATIM
    )

    list(APPEND CE_GENERATED_BINDING_SOURCES "${CE_IDL_GENERATED_SOURCE}")
    list(APPEND CE_GENERATED_BINDING_METADATA "${CE_IDL_GENERATED_METADATA}")
    list(APPEND CE_GENERATED_BINDING_OUTPUTS
        "${CE_IDL_GENERATED_HEADER}"
        "${CE_IDL_GENERATED_SOURCE}"
        "${CE_IDL_GENERATED_METADATA}")
endforeach()

add_custom_target(ce_generated_idl ALL DEPENDS ${CE_GENERATED_BINDING_OUTPUTS})

include("${CE_RUNTIME_ROOT}/cmake/compiler.cmake")
include("${CE_RUNTIME_ROOT}/cmake/dependencies.cmake")
include("${CE_RUNTIME_ROOT}/cmake/generated_files.cmake")
include("${CE_RUNTIME_ROOT}/cmake/sources.cmake")
include("${CE_RUNTIME_ROOT}/cmake/engine.cmake")
