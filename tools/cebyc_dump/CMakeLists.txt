cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Angelscript CONFIG REQUIRED)

project(cebyc_dump VERSION 1.0.0 LANGUAGES C CXX)

add_executable(cebyc_dump
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

if(CE_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cebyc_dump PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

    target_link_options(cebyc_dump PRIVATE
        -fsanitize=address,undefined
    )
endif()

set_target_properties(cebyc_dump PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

target_link_libraries(cebyc_dump PRIVATE
    ce_common
    Angelscript::angelscript
)