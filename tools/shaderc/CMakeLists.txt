cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ce_shaderc VERSION 1.0.0 LANGUAGES CXX)

find_package(glslang CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_shadercross CONFIG REQUIRED)  

add_executable(ce_shaderc
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set_target_properties(ce_shaderc PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

target_link_libraries(ce_shaderc PRIVATE
    glslang::glslang
    glslang::SPIRV
    glslang::glslang-default-resource-limits
    SDL3::SDL3
    SDL3_shadercross::SDL3_shadercross
)