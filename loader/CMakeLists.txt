cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SDL3 CONFIG REQUIRED)

project(ce_loader VERSION 1.0.0 LANGUAGES C CXX)

add_executable(ce_loader
    ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp
)

target_link_libraries(ce_loader PRIVATE
    ce_runtime_shared
    SDL3::SDL3
)