cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(yaml-cpp CONFIG REQUIRED)

add_executable(ce_griller
    ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp
)

target_include_directories(ce_griller PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/include/engine/common/misc # literally just to get gameinfo.hpp 
)

target_link_libraries(ce_griller PRIVATE
    yaml-cpp::yaml-cpp
)