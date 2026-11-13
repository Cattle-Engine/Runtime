
cmake_minimum_required(VERSION 3.16)

project(ce_editor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE CE_EDITOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)

add_executable(ce_editor ${CE_EDITOR_SOURCES})

if(CE_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ce_editor PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

    target_link_options(ce_editor PRIVATE
        -fsanitize=address,undefined
    )
endif()

target_include_directories(ce_editor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(ce_editor PRIVATE
    ce_common
    ce_runtime_shared
)

set_target_properties(ce_editor PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    OUTPUT_NAME ce_editor
)
