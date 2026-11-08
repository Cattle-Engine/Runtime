cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(tcf VERSION 1.0.0 LANGUAGES CXX)

add_executable(tcf
    ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp
)

set_target_properties(tcf PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

target_link_libraries(tcf PRIVATE
    ce_common
)