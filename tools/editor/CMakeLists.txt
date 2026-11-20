
cmake_minimum_required(VERSION 3.16)

project(ce_editor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE CE_EDITOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)

add_executable(ce_editor ${CE_EDITOR_SOURCES})

if(WIN32)
    enable_language(RC)

    target_sources(ce_editor PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/editor.rc"
    )

    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/editor.rc"
        PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/editor.ico"
    )
endif()

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

# Keep the editor runnable directly from the build output.  The runtime mounts
# this directory as its VFS root, so compile the engine shaders beside it rather
# than requiring a separate shader compiler or an asset-packaging step.
if(NOT TARGET ce_shaderc)
    message(FATAL_ERROR
        "The ce_shaderc target does not exist. Add tools/shaderc before tools/editor."
    )
endif()

set(CE_EDITOR_RUNTIME_SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/runtime/shaders")
set(CE_EDITOR_DATA_DIR "${CE_BIN_DIR}/$<CONFIG>/editor_data")
set(CE_EDITOR_SHADER_OUTPUT_DIR "${CE_EDITOR_DATA_DIR}/shaders/Compiled/SPIRV")
set(CE_EDITOR_ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

add_custom_target(ce_editor_assets
    COMMAND "${CMAKE_COMMAND}"
        "-DEDITOR_DATA_DIR=${CE_EDITOR_DATA_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clean_editor_assets.cmake"

    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CE_EDITOR_DATA_DIR}"

    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CE_EDITOR_ASSETS_SOURCE_DIR}"
        "${CE_EDITOR_DATA_DIR}"

    COMMENT "Synchronising editor assets"
    VERBATIM
)

add_dependencies(ce_editor ce_editor_assets)

file(GLOB_RECURSE CE_EDITOR_RUNTIME_SHADER_SOURCES CONFIGURE_DEPENDS
    LIST_DIRECTORIES FALSE
    "${CE_EDITOR_RUNTIME_SHADER_SOURCE_DIR}/*.vert"
    "${CE_EDITOR_RUNTIME_SHADER_SOURCE_DIR}/*.frag"
    "${CE_EDITOR_RUNTIME_SHADER_SOURCE_DIR}/*.comp"
)

set(CE_EDITOR_RUNTIME_SHADER_OUTPUTS)
foreach(shader_source IN LISTS CE_EDITOR_RUNTIME_SHADER_SOURCES)
    file(RELATIVE_PATH shader_relative
        "${CE_EDITOR_RUNTIME_SHADER_SOURCE_DIR}"
        "${shader_source}"
    )

    set(shader_output "${CE_EDITOR_SHADER_OUTPUT_DIR}/${shader_relative}.spv")
    get_filename_component(shader_output_dir "${shader_output}" DIRECTORY)

    add_custom_command(
        OUTPUT "${shader_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${shader_output_dir}"
        COMMAND "$<TARGET_FILE:ce_shaderc>"
            "${shader_source}"
            --formats spv
            --output-dir "${shader_output_dir}"
        DEPENDS ce_shaderc "${shader_source}"
        COMMENT "Compiling editor runtime shader: ${shader_relative}"
        VERBATIM
    )

    list(APPEND CE_EDITOR_RUNTIME_SHADER_OUTPUTS "${shader_output}")
endforeach()

add_custom_target(ce_editor_runtime_shaders
    DEPENDS ${CE_EDITOR_RUNTIME_SHADER_OUTPUTS}
)
add_dependencies(ce_editor ce_editor_runtime_shaders)
