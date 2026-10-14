cmake_minimum_required(VERSION 3.16)

project(ce_scriptc VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Angelscript CONFIG REQUIRED)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT DEFINED ANGELSCRIPT_LINK_TARGET)
    set(ANGELSCRIPT_LINK_TARGET Angelscript::angelscript)
endif()

if(NOT DEFINED ANGELSCRIPT_ADDON_INCLUDE_DIR)
    find_path(ANGELSCRIPT_ADDON_INCLUDE_DIR
        NAMES scriptstdstring/scriptstdstring.h
        PATH_SUFFIXES include/angelscript
        REQUIRED
    )
endif()

if(NOT DEFINED ANGELSCRIPT_ADDON_SOURCE_DIR)
    set(ANGELSCRIPT_ADDON_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../../vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include/angelscript"
    )
endif()

set(ANGELSCRIPT_ADDON_SOURCES
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/datetime/datetime.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptmath/scriptmath.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scripthandle/scripthandle.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptstdstring/scriptstdstring.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptstdstring/scriptstdstring_utils.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptbuilder/scriptbuilder.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptarray/scriptarray.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptdictionary/scriptdictionary.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptany/scriptany.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptgrid/scriptgrid.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/debugger/debugger.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scripthelper/scripthelper.cpp"
)

foreach(source IN LISTS ANGELSCRIPT_ADDON_SOURCES)
    if(NOT EXISTS "${source}")
        message(FATAL_ERROR
            "AngelScript add-on source not found: ${source}\n"
            "Set ANGELSCRIPT_ADDON_SOURCE_DIR to the directory containing the add-on .cpp files."
        )
    endif()
endforeach()

set(CE_DUMMY_GENERATOR
    "${CMAKE_CURRENT_SOURCE_DIR}/../build/dummy_bindings_generator.py")
set(CE_IDL_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/source/engine/scripting/bindings")
set(CE_DUMMY_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(CE_DUMMY_HEADER  "${CE_DUMMY_GEN_DIR}/DummyBindings.h")
set(CE_DUMMY_SOURCE  "${CE_DUMMY_GEN_DIR}/DummyBindings.cpp")

file(GLOB_RECURSE CE_IDL_FILES CONFIGURE_DEPENDS "${CE_IDL_DIR}/*.idl.yaml")

add_custom_command(
    OUTPUT  "${CE_DUMMY_HEADER}" "${CE_DUMMY_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CE_DUMMY_GENERATOR}"
            --idl_dir "${CE_IDL_DIR}"
            --output_header "${CE_DUMMY_HEADER}"
            --output_source "${CE_DUMMY_SOURCE}"
            --function_name RegisterDummyBindings
    DEPENDS "${CE_DUMMY_GENERATOR}" ${CE_IDL_FILES}
    COMMENT "Generating dummy AngelScript bindings from *.idl.yaml"
    VERBATIM
)

add_custom_target(ce_scriptc_dummy_bindings
    DEPENDS "${CE_DUMMY_HEADER}" "${CE_DUMMY_SOURCE}")

file(GLOB_RECURSE CE_SCRIPTC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)

add_executable(ce_scriptc ${CE_SCRIPTC_SOURCES})

target_sources(ce_scriptc PRIVATE
    ${ANGELSCRIPT_ADDON_SOURCES}
    "${CE_DUMMY_SOURCE}"       
)

add_dependencies(ce_scriptc ce_scriptc_dummy_bindings)

target_include_directories(ce_scriptc PRIVATE
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}"
    "${CE_DUMMY_GEN_DIR}"    
    "${CMAKE_CURRENT_SOURCE_DIR}/source"     
)

target_link_libraries(ce_scriptc PRIVATE
    ce_common
    ce_runtime_shared
    ${ANGELSCRIPT_LINK_TARGET}
)

set_target_properties(ce_scriptc PROPERTIES
    OUTPUT_NAME ce_scriptc
)