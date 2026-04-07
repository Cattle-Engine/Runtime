cmake_minimum_required(VERSION 3.16)

# -------------------------
# Compiler
# -------------------------
option(USE_CLANG "Use Clang compiler" ON)

if(USE_CLANG)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
endif()

project(ce_engine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------
# Platform detection
# -------------------------
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

# -------------------------
# Paths
# -------------------------
set(ASSET_FOLDER assets)
set(SCRIPT_FOLDER scripts)
set(SHADER_DIR shaders)
set(DATA_FILE_NAME data.tcf)

# -------------------------
# Sources
# -------------------------
file(GLOB_RECURSE SOURCES
    source/*.cpp
    source/*.c
)

# -------------------------
# Shaders
# -------------------------
file(GLOB SHADER_SOURCES
    ${SHADER_DIR}/*.frag
    ${SHADER_DIR}/*.vert
)

set(SHADER_HEADERS)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)

    if(SHADER_EXT STREQUAL ".frag")
        set(HEADER ${CMAKE_SOURCE_DIR}/include/engine/renderers/shaders/${SHADER_NAME}_frag_spv.h)
    elseif(SHADER_EXT STREQUAL ".vert")
        set(HEADER ${CMAKE_SOURCE_DIR}/include/engine/renderers/shaders/${SHADER_NAME}_vert_spv.h)
    endif()

    list(APPEND SHADER_HEADERS ${HEADER})

    add_custom_command(
        OUTPUT ${HEADER}
        DEPENDS ${SHADER}
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_SOURCE_DIR}/include/engine/renderers/shaders
        COMMAND glslc ${SHADER} -o ${SHADER}.spv
        COMMAND xxd -i ${SHADER}.spv > ${HEADER}
        COMMAND ${CMAKE_COMMAND} -E rm ${SHADER}.spv
    )
endforeach()

add_custom_target(shaders DEPENDS ${SHADER_HEADERS})

# -------------------------
# Assets
# -------------------------
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/${DATA_FILE_NAME}
    COMMAND python ${CMAKE_SOURCE_DIR}/${SCRIPT_FOLDER}/tcf.py pack
            ${CMAKE_SOURCE_DIR}/${ASSET_FOLDER}
            ${DATA_FILE_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(assets DEPENDS ${CMAKE_BINARY_DIR}/${DATA_FILE_NAME})

# -------------------------
# Executable
# -------------------------
add_executable(ce_engine ${SOURCES})

add_dependencies(ce_engine shaders assets)

target_include_directories(ce_engine PRIVATE
    include
    include/third_party
    include/third_party/imgui
    include/third_party/lua
)

target_compile_definitions(ce_engine PRIVATE
    ENGINE_BUILT_ON_OS="${HOST_OS}"
    CE_DATA_FILE_NAME="${DATA_FILE_NAME}"
    ${PLATFORM_FLAGS}
)

# -------------------------
# Compiler flags
# -------------------------
target_compile_options(ce_engine PRIVATE
    -Wall
    -Wextra
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ce_engine PRIVATE -g)
endif()

# -------------------------
# SDL3 (CROSS-PLATFORM FIX)
# -------------------------
find_package(SDL3 CONFIG QUIET)

if(NOT SDL3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED sdl3)

    target_include_directories(ce_engine PRIVATE ${SDL3_INCLUDE_DIRS})
    target_link_libraries(ce_engine PRIVATE ${SDL3_LIBRARIES})
else()
    target_link_libraries(ce_engine PRIVATE SDL3::SDL3)
endif()

# -------------------------
# Vulkan (needed for your renderer)
# -------------------------
find_package(Vulkan REQUIRED)
target_link_libraries(ce_engine PRIVATE Vulkan::Vulkan)

# -------------------------
# Windows-only extras
# -------------------------
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

# -------------------------
# Install
# -------------------------
install(TARGETS ce_engine DESTINATION bin)

# -------------------------
# Run
# -------------------------
add_custom_target(run
    COMMAND ce_engine
    DEPENDS ce_engine
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)