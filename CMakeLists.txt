cmake_minimum_required(VERSION 3.16)

option(USE_CLANG "Use Clang compiler" ON)

if(USE_CLANG)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
endif()

project(ce_engine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(WIN32)
set(HOST_OS "Windows")
set(PLATFORM_FLAGS -DPLATFORM_WINDOWS)

elseif(APPLE)
set(HOST_OS "macOS")
set(PLATFORM_FLAGS -DPLATFORM_MACOS)

elseif(UNIX)
set(HOST_OS "Linux")
set(PLATFORM_FLAGS -DPLATFORM_LINUX)
endif()

set(ASSET_FOLDER assets)
set(SCRIPT_FOLDER scripts)
set(SHADER_DIR shaders)
set(DATA_FILE_NAME data.tcf)

find_package(Python3 COMPONENTS Interpreter REQUIRED)


file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
source/*.cpp
source/*.c
)

if(WIN32)
    list(FILTER SOURCES EXCLUDE REGEX "/platforms/linux/")
elseif(APPLE)
    list(FILTER SOURCES EXCLUDE REGEX "/platforms/windows/")
    list(FILTER SOURCES EXCLUDE REGEX "/platforms/linux/")
elseif(UNIX)
    list(FILTER SOURCES EXCLUDE REGEX "/platforms/windows/")
endif()

set(AS_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(external/angelscript/sdk/angelscript/projects/cmake)

set(ANGELSCRIPT_ADDON_SOURCES
external/angelscript/sdk/add_on/datetime/datetime.cpp
external/angelscript/sdk/add_on/scriptmath/scriptmath.cpp
external/angelscript/sdk/add_on/scripthandle/scripthandle.cpp
external/angelscript/sdk/add_on/scriptstdstring/scriptstdstring.cpp
external/angelscript/sdk/add_on/scriptbuilder/scriptbuilder.cpp
external/angelscript/sdk/add_on/scriptarray/scriptarray.cpp
external/angelscript/sdk/add_on/scriptdictionary/scriptdictionary.cpp
external/angelscript/sdk/add_on/scriptany/scriptany.cpp
external/angelscript/sdk/add_on/scriptgrid/scriptgrid.cpp
)

file(GLOB SHADER_SOURCES
${SHADER_DIR}/*.frag
${SHADER_DIR}/*.vert
)

find_program(GLSLC_EXECUTABLE glslc)
find_program(XXD_EXECUTABLE xxd)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "Required tool 'glslc' not found on PATH (needed for shader compilation).")
endif()
if(NOT XXD_EXECUTABLE)
    message(FATAL_ERROR "Required tool 'xxd' not found on PATH (needed for shader header generation).")
endif()

set(SHADER_HEADERS)

foreach(SHADER ${SHADER_SOURCES})
get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
get_filename_component(SHADER_EXT ${SHADER} EXT)

string(REPLACE "." "" SHADER_EXT_CLEAN ${SHADER_EXT})
set(SYMBOL_NAME ${SHADER_NAME}_${SHADER_EXT_CLEAN}_spv)

set(HEADER ${CMAKE_SOURCE_DIR}/include/engine/renderers/shaders/${SYMBOL_NAME}.h)

list(APPEND SHADER_HEADERS ${HEADER})

add_custom_command(
    OUTPUT ${HEADER}
    DEPENDS ${SHADER}

    COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER}.spv

    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders/Compiled/SPIRV

    COMMAND ${CMAKE_COMMAND} -E copy
        ${SHADER}.spv
        ${CMAKE_SOURCE_DIR}/assets/shaders/Compiled/SPIRV/${SHADER_NAME}.${SHADER_EXT_CLEAN}.spv

    COMMAND ${XXD_EXECUTABLE} -i -n ${SYMBOL_NAME} ${SHADER}.spv ${HEADER}

    COMMAND ${CMAKE_COMMAND} -E remove -f ${SHADER}.spv
)

endforeach()

add_custom_target(shaders DEPENDS ${SHADER_HEADERS})


add_custom_command(
OUTPUT ${CMAKE_BINARY_DIR}/${DATA_FILE_NAME}
COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/${SCRIPT_FOLDER}/tcf.py pack
${CMAKE_SOURCE_DIR}/${ASSET_FOLDER}
${DATA_FILE_NAME}
WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(assets DEPENDS ${CMAKE_BINARY_DIR}/${DATA_FILE_NAME})

add_executable(ce_engine ${SOURCES})
target_sources(ce_engine PRIVATE ${ANGELSCRIPT_ADDON_SOURCES})

add_dependencies(ce_engine shaders assets)

target_include_directories(ce_engine PRIVATE
include
include/third_party
include/third_party/imgui
external/angelscript/sdk/include
external/angelscript/sdk/add_on
)

target_compile_definitions(ce_engine PRIVATE
ENGINE_BUILT_ON_OS="${HOST_OS}"
CE_DATA_FILE_NAME="${DATA_FILE_NAME}"
${PLATFORM_FLAGS}
)


target_compile_options(ce_engine PRIVATE
-Wall
-Wextra
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
target_compile_options(ce_engine PRIVATE -g)
endif()


find_package(SDL3 CONFIG QUIET)

if(NOT SDL3_FOUND)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)

target_include_directories(ce_engine PRIVATE ${SDL3_INCLUDE_DIRS})
target_link_libraries(ce_engine PRIVATE ${SDL3_LIBRARIES})

else()
target_link_libraries(ce_engine PRIVATE SDL3::SDL3)
endif()

find_package(SDL3_image CONFIG QUIET)

if(NOT SDL3_image_FOUND)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3_IMAGE REQUIRED SDL3_image)

target_include_directories(ce_engine PRIVATE ${SDL3_IMAGE_INCLUDE_DIRS})
target_link_libraries(ce_engine PRIVATE ${SDL3_IMAGE_LIBRARIES})


else()
target_link_libraries(ce_engine PRIVATE SDL3_image::SDL3_image)
endif()


find_package(SDL3_ttf CONFIG QUIET)

if(NOT SDL3_ttf_FOUND)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3_TTF REQUIRED SDL3_ttf)

target_include_directories(ce_engine PRIVATE ${SDL3_TTF_INCLUDE_DIRS})
target_link_libraries(ce_engine PRIVATE ${SDL3_TTF_LIBRARIES})


else()
target_link_libraries(ce_engine PRIVATE SDL3_ttf::SDL3_ttf)
endif()


find_package(Vulkan REQUIRED)
target_link_libraries(ce_engine PRIVATE Vulkan::Vulkan)

target_link_libraries(ce_engine PRIVATE
    angelscript
)

if(WIN32)
if(TARGET SDL3::SDL3main)
target_link_libraries(ce_engine PRIVATE SDL3::SDL3main)
endif()

target_link_libraries(ce_engine PRIVATE
    opengl32
    comdlg32
    ole32
    oleaut32
    uuid
    imm32
    user32
    gdi32
    shell32
)

endif()

install(TARGETS ce_engine DESTINATION bin)

add_custom_target(run
COMMAND ce_engine
DEPENDS ce_engine
WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
