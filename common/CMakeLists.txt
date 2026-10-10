
cmake_minimum_required(VERSION 3.16)

project(ce_common VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(zstd CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

file(GLOB_RECURSE CE_COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.c"
)

add_library(ce_common SHARED ${CE_COMMON_SOURCES})

target_link_libraries(ce_common PRIVATE zstd::libzstd lz4::lz4)

set_target_properties(ce_common PROPERTIES
    PREFIX ""
)

target_compile_definitions(ce_common PRIVATE
    $<$<CONFIG:Debug>:CE_DEBUG>
)

target_include_directories(ce_common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)