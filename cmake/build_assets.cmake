set(CE_ASSET_SOURCE_DIR
    "${CMAKE_SOURCE_DIR}/assets"
    CACHE PATH "Source asset directory"
)

set(CE_ASSET_DATAFILE_NAME
    "data.tcf"
    CACHE STRING "Name of the generated TCF archive"
)

set(CE_SCRIPT_BASEFILE
    "startup.as"
    CACHE STRING "Startup script source filename"
)

option(CE_SCRIPT_DEBUG
    "Enable script compiler debug information"
    OFF
)

set(CE_ASSET_OUTPUT_DIR
    "${CE_BIN_DIR}/$<CONFIG>"
)

set(CE_ASSET_STAGING_DIR
    "${CMAKE_BINARY_DIR}/staged"
)

set(CE_ASSET_ARCHIVE
    "${CE_ASSET_OUTPUT_DIR}/${CE_ASSET_DATAFILE_NAME}"
)

set(CE_ASSET_PATH_CONFIG
    "${CE_BIN_DIR}/$<CONFIG>/path.cfg"
)

set(CE_SHADER_SOURCE_DIR
    "${CMAKE_SOURCE_DIR}/runtime/shaders"
)

set(CE_SHADER_GENERATED_DIR
    "${CMAKE_BINARY_DIR}/generated/shaders"
)

set(CE_JSON_TO_TDF_SCRIPT
    "${CMAKE_SOURCE_DIR}/scripts/json_to_tdf.py"
)

set(CE_PACK_SCRIPT
    "${CMAKE_CURRENT_LIST_DIR}/pack_assets.cmake"
)

set(CE_SCRIPT_BYTECODE_NAME
    "startup"
    CACHE STRING "Compiled script bytecode filename without extension"
)

set(CE_SCRIPT_BYTECODE_EXTENSION
    ".cbc"
    CACHE STRING "Compiled script bytecode file extension"
)


if(NOT IS_DIRECTORY "${CE_ASSET_SOURCE_DIR}")
    message(FATAL_ERROR
        "Asset directory does not exist: ${CE_ASSET_SOURCE_DIR}"
    )
endif()

if(NOT EXISTS "${CE_JSON_TO_TDF_SCRIPT}")
    message(FATAL_ERROR
        "JSON-to-TDF script does not exist: ${CE_JSON_TO_TDF_SCRIPT}"
    )
endif()

if(NOT EXISTS "${CE_PACK_SCRIPT}")
    message(FATAL_ERROR
        "Asset pack script does not exist: ${CE_PACK_SCRIPT}"
    )
endif()

if("${CE_ASSET_DATAFILE_NAME}" STREQUAL "")
    message(FATAL_ERROR
        "CE_ASSET_DATAFILE_NAME cannot be empty."
    )
endif()

get_filename_component(
    CE_ASSET_DATAFILE_BASENAME
    "${CE_ASSET_DATAFILE_NAME}"
    NAME
)

if(NOT "${CE_ASSET_DATAFILE_BASENAME}" STREQUAL
       "${CE_ASSET_DATAFILE_NAME}")
    message(FATAL_ERROR
        "CE_ASSET_DATAFILE_NAME must be a filename, not a path."
    )
endif()

if(NOT TARGET tcf)
    message(FATAL_ERROR
        "The tcf target does not exist. Add tools/tcf before build_assets.cmake."
    )
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(TARGET ce_scriptc)
    set(CE_SCRIPTC_EXECUTABLE "$<TARGET_FILE:ce_scriptc>")
else()
    find_program(
        CE_SCRIPTC_EXECUTABLE
        NAMES ce_scriptc ce_scriptc.exe
    )

    if(NOT CE_SCRIPTC_EXECUTABLE)
        message(FATAL_ERROR
            "Could not find ce_scriptc. Build the ce_scriptc target "
            "or add the executable to PATH."
        )
    endif()
endif()

find_program(
    CE_GLSLC
    NAMES glslc glslc.exe
    HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "$ENV{VULKAN_SDK}/bin"
)

if(NOT CE_GLSLC)
    message(FATAL_ERROR
        "Could not find glslc. Install the Vulkan SDK or add glslc to PATH."
    )
endif()

file(
    GLOB_RECURSE CE_SHADER_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES FALSE
    "${CE_SHADER_SOURCE_DIR}/*.vert"
    "${CE_SHADER_SOURCE_DIR}/*.frag"
)

set(CE_SHADER_SPV_FILES "")

foreach(shader_source IN LISTS CE_SHADER_SOURCES)
    file(
        RELATIVE_PATH shader_relative
        "${CE_SHADER_SOURCE_DIR}"
        "${shader_source}"
    )

    set(shader_output
        "${CE_SHADER_GENERATED_DIR}/${shader_relative}.spv"
    )

    get_filename_component(
        shader_output_dir
        "${shader_output}"
        DIRECTORY
    )

    add_custom_command(
        OUTPUT "${shader_output}"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${shader_output_dir}"

        COMMAND
            "${CE_GLSLC}"
            "${shader_source}"
            -o "${shader_output}"

        DEPENDS
            "${shader_source}"

        COMMENT
            "Compiling shader: ${shader_relative}"

        VERBATIM
    )

    list(APPEND CE_SHADER_SPV_FILES "${shader_output}")
endforeach()

add_custom_target(
    compile_shaders
    DEPENDS ${CE_SHADER_SPV_FILES}
)

file(
    GLOB_RECURSE CE_ASSET_FILES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES FALSE
    "${CE_ASSET_SOURCE_DIR}/*"
)

set(CE_ASSET_MANIFEST_CONTENT
    "Archive: ${CE_ASSET_DATAFILE_NAME}\n"
    "Assets:\n"
)

foreach(asset_file IN LISTS CE_ASSET_FILES)
    string(APPEND CE_ASSET_MANIFEST_CONTENT "${asset_file}\n")
endforeach()

string(APPEND CE_ASSET_MANIFEST_CONTENT "Shaders:\n")

foreach(shader_source IN LISTS CE_SHADER_SOURCES)
    string(APPEND CE_ASSET_MANIFEST_CONTENT "${shader_source}\n")
endforeach()

set(CE_ASSET_INPUT_MANIFEST
    "${CMAKE_BINARY_DIR}/asset_inputs.txt"
)

file(
    GENERATE
    OUTPUT "${CE_ASSET_INPUT_MANIFEST}"
    CONTENT "${CE_ASSET_MANIFEST_CONTENT}"
)

set(CE_PATH_CONFIG_SCRIPT
    "${CMAKE_SOURCE_DIR}/cmake/write_path_config.cmake"
)

add_custom_command(
    OUTPUT "${CE_ASSET_PATH_CONFIG}"

    COMMAND
        "${CMAKE_COMMAND}"
        "-DCE_ASSET_PATH_CONFIG=${CE_ASSET_PATH_CONFIG}"
        "-DCE_ASSET_DATAFILE_NAME=${CE_ASSET_DATAFILE_NAME}"
        -P "${CE_PATH_CONFIG_SCRIPT}"

    COMMENT "Generating path.cfg"

    VERBATIM
)

add_custom_command(
    OUTPUT "${CE_ASSET_ARCHIVE}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${CE_ASSET_OUTPUT_DIR}"

    COMMAND
        "${CMAKE_COMMAND}"
        "-DCE_ASSET_SOURCE_DIR=${CE_ASSET_SOURCE_DIR}"
        "-DCE_ASSET_OUTPUT_DIR=${CE_ASSET_OUTPUT_DIR}"
        "-DCE_ASSET_STAGING_DIR=${CE_ASSET_STAGING_DIR}"
        "-DCE_ASSET_ARCHIVE=${CE_ASSET_ARCHIVE}"
        "-DCE_SHADER_GENERATED_DIR=${CE_SHADER_GENERATED_DIR}"
        "-DCE_SHADER_SPV_FILES=${CE_SHADER_SPV_FILES}"
        "-DCE_JSON_TO_TDF_SCRIPT=${CE_JSON_TO_TDF_SCRIPT}"
        "-DCE_PYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DCE_TCF_EXECUTABLE=$<TARGET_FILE:tcf>"
        "-DCE_SCRIPTC_EXECUTABLE=$<TARGET_FILE:ce_scriptc>"
        "-DCE_SCRIPT_BASEFILE=${CE_SCRIPT_BASEFILE}"
        "-DCE_SCRIPT_DEBUG=${CE_SCRIPT_DEBUG}"
        "-DCE_SCRIPT_BYTECODE_NAME=${CE_SCRIPT_BYTECODE_NAME}"
        "-DCE_SCRIPT_BYTECODE_EXTENSION=${CE_SCRIPT_BYTECODE_EXTENSION}"
        -P "${CE_PACK_SCRIPT}"

    DEPENDS
        tcf
        ce_scriptc
        ${CE_ASSET_FILES}
        ${CE_SHADER_SPV_FILES}
        "${CE_JSON_TO_TDF_SCRIPT}"
        "${CE_PACK_SCRIPT}"
        "${CE_ASSET_INPUT_MANIFEST}"

    COMMENT "Packing assets into ${CE_ASSET_DATAFILE_NAME}"

    VERBATIM
)

add_custom_target(
    pack_assets
    DEPENDS
        "${CE_ASSET_ARCHIVE}"
        "${CE_ASSET_PATH_CONFIG}"
)

message(STATUS "TCF archive: ${CE_ASSET_ARCHIVE}")
message(STATUS "Asset path config: ${CE_ASSET_PATH_CONFIG}")
message(STATUS "Shader compiler: ${CE_GLSLC}")