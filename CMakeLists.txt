cmake_minimum_required(VERSION 3.16)

# Options
option(USE_CLANG "Use Clang compiler" ON)

# Set compiler before project
if(USE_CLANG)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
else()
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
endif()

project(ce_engine VERSION 1.0.0 LANGUAGES C CXX)

# Detect OS
if(WIN32)
    set(HOST_OS "Windows")
    set(PLATFORM_FLAGS -DPLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    set(HOST_OS "Linux")
elseif(APPLE)
    set(HOST_OS "macOS")
else()
    set(HOST_OS "Unknown OS")
endif()

# Constants
set(ASSET_FOLDER assets)
set(SCRIPT_FOLDER scripts)
set(SHADER_DIR shaders)
set(DATA_FILE_NAME data.tcf)

# Find sources
file(GLOB_RECURSE SOURCES
    source/*.cpp
    source/*.c
)

# Find shaders
file(GLOB SHADER_SOURCES
    ${SHADER_DIR}/*.frag
    ${SHADER_DIR}/*.vert
)

# Shader headers
set(SHADER_HEADERS)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)
    if(SHADER_EXT STREQUAL ".frag")
        list(APPEND SHADER_HEADERS ${CMAKE_SOURCE_DIR}/include/engine/renderers/shaders/${SHADER_NAME}_frag_spv.h)
    elseif(SHADER_EXT STREQUAL ".vert")
        list(APPEND SHADER_HEADERS ${CMAKE_SOURCE_DIR}/include/engine/renderers/shaders/${SHADER_NAME}_vert_spv.h)
    endif()
endforeach()

# Shader compilation commands
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)
    if(SHADER_EXT STREQUAL ".frag")
        set(HEADER ${CMAKE_SOURCE_DIR}/include/engine/renderers/shaders/${SHADER_NAME}_frag_spv.h)
    elseif(SHADER_EXT STREQUAL ".vert")
        set(HEADER ${CMAKE_SOURCE_DIR}/include/engine/renderers/shaders/${SHADER_NAME}_vert_spv.h)
    endif()
    add_custom_command(
        OUTPUT ${HEADER}
        DEPENDS ${SHADER}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/engine/renderers/shaders
        COMMAND glslc ${SHADER} -o ${SHADER}.spv
        COMMAND xxd -i ${SHADER}.spv > ${HEADER}
        COMMAND ${CMAKE_COMMAND} -E rm ${SHADER}.spv
        COMMENT "Compiling shader ${SHADER}"
    )
endforeach()

# Shader target
add_custom_target(shaders DEPENDS ${SHADER_HEADERS})

# Asset packing command
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/${DATA_FILE_NAME}
    COMMAND python ${CMAKE_SOURCE_DIR}/${SCRIPT_FOLDER}/tcf.py pack ${CMAKE_SOURCE_DIR}/${ASSET_FOLDER} ${DATA_FILE_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Packing assets"
)

# Asset target
add_custom_target(assets DEPENDS ${CMAKE_BINARY_DIR}/${DATA_FILE_NAME})

# Executable
add_executable(ce_engine ${SOURCES})

# Dependencies
add_dependencies(ce_engine shaders assets)

# Include directories
target_include_directories(ce_engine PRIVATE
    include
    include/third_party
    include/third_party/imgui
    include/third_party/lua
)

# Compile definitions
target_compile_definitions(ce_engine PRIVATE
    ENGINE_BUILT_ON_OS="${HOST_OS}"
    CE_DATA_FILE_NAME="${DATA_FILE_NAME}"
    ${PLATFORM_FLAGS}
)

# Graphics backend
if(UNIX AND NOT APPLE)
    if(GRAPHICS_BACKEND_X11)
        target_compile_definitions(ce_engine PRIVATE GRAPHICS_BACKEND_X11)
    elseif(GRAPHICS_BACKEND_WAYLAND)
        target_compile_definitions(ce_engine PRIVATE GRAPHICS_BACKEND_WAYLAND)
    endif()
endif()

# Compile options
target_compile_options(ce_engine PRIVATE
    -Wall
    -Wextra
)

# Debug options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ce_engine PRIVATE -g)
endif()

# Link libraries
if(WIN32)
    target_link_libraries(ce_engine PRIVATE
        SDL3
        SDL3_image
        raylib
        opengl32
        ole32
        uuid
        comdlg32
        imm32
        oleaut32
    )
    # Windows subsystem
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(ce_engine PRIVATE -Wl,-subsystem,windows)
    endif()
else()
    target_link_libraries(ce_engine PRIVATE
        SDL3
        SDL3_image
        raylib
        m
        GL
        X11
        pthread
        dl
        rt
        Xi
    )
endif()

# Install
install(TARGETS ce_engine DESTINATION bin)

# Custom targets
add_custom_target(run
    COMMAND ce_engine
    DEPENDS ce_engine
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/*
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/${DATA_FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E rm -f ${SHADER_HEADERS}
)