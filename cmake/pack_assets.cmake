foreach(required_var IN ITEMS
    CE_ASSET_SOURCE_DIR
    CE_ASSET_OUTPUT_DIR
    CE_ASSET_STAGING_DIR
    CE_ASSET_ARCHIVE
    CE_SHADER_GENERATED_DIR
    CE_JSON_TO_TDF_SCRIPT
    CE_PYTHON_EXECUTABLE
    CE_TCF_EXECUTABLE
    CE_SCRIPTC_EXECUTABLE
)
    if(NOT DEFINED ${required_var})
        message(FATAL_ERROR
            "Missing required variable: ${required_var}"
        )
    endif()
endforeach()

if(NOT DEFINED CE_SCRIPT_BASE_NAME)
    set(CE_SCRIPT_BASE_NAME "startup")
endif()

if(NOT IS_DIRECTORY "${CE_ASSET_SOURCE_DIR}")
    message(FATAL_ERROR
        "Asset directory does not exist: ${CE_ASSET_SOURCE_DIR}"
    )
endif()

if(NOT EXISTS "${CE_TCF_EXECUTABLE}")
    message(FATAL_ERROR
        "TCF executable does not exist: ${CE_TCF_EXECUTABLE}"
    )
endif()

if(NOT EXISTS "${CE_SCRIPTC_EXECUTABLE}")
    message(FATAL_ERROR
        "Script compiler does not exist: ${CE_SCRIPTC_EXECUTABLE}"
    )
endif()

message(STATUS "Staging assets...")

file(REMOVE_RECURSE "${CE_ASSET_STAGING_DIR}")
file(MAKE_DIRECTORY "${CE_ASSET_STAGING_DIR}")

file(
    COPY "${CE_ASSET_SOURCE_DIR}/"
    DESTINATION "${CE_ASSET_STAGING_DIR}"
    PATTERN "*.as" EXCLUDE
    PATTERN "*.ceas" EXCLUDE
)


set(staged_shader_dir
    "${CE_ASSET_STAGING_DIR}/shaders/Compiled/SPIRV"
)

file(REMOVE_RECURSE "${staged_shader_dir}")
file(MAKE_DIRECTORY "${staged_shader_dir}")

foreach(shader_spv IN LISTS CE_SHADER_SPV_FILES)
    if(NOT EXISTS "${shader_spv}")
        message(FATAL_ERROR
            "Compiled shader is missing: ${shader_spv}"
        )
    endif()

    file(
        RELATIVE_PATH shader_relative
        "${CE_SHADER_GENERATED_DIR}"
        "${shader_spv}"
    )

    set(staged_shader
        "${staged_shader_dir}/${shader_relative}"
    )

    get_filename_component(
        staged_shader_parent
        "${staged_shader}"
        DIRECTORY
    )

    file(MAKE_DIRECTORY "${staged_shader_parent}")

    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E copy
            "${shader_spv}"
            "${staged_shader}"
        RESULT_VARIABLE copy_result
    )

    if(NOT "${copy_result}" STREQUAL "0")
        message(FATAL_ERROR
            "Failed to copy shader: ${shader_spv}"
        )
    endif()
endforeach()

message(STATUS "Converting JSON assets to TDF...")

file(
    GLOB_RECURSE json_files
    LIST_DIRECTORIES FALSE
    "${CE_ASSET_SOURCE_DIR}/*.json"
)

foreach(json_file IN LISTS json_files)
    file(
        RELATIVE_PATH json_relative
        "${CE_ASSET_SOURCE_DIR}"
        "${json_file}"
    )

    get_filename_component(
        json_relative_dir
        "${json_relative}"
        DIRECTORY
    )

    set(json_output_dir
        "${CE_ASSET_STAGING_DIR}/${json_relative_dir}"
    )

    file(MAKE_DIRECTORY "${json_output_dir}")

    execute_process(
        COMMAND
            "${CE_PYTHON_EXECUTABLE}"
            "${CE_JSON_TO_TDF_SCRIPT}"
            --mode generic
            --out-dir "${json_output_dir}"
            "${json_file}"
        RESULT_VARIABLE tdf_result
    )

    if(NOT "${tdf_result}" STREQUAL "0")
        message(FATAL_ERROR
            "JSON-to-TDF conversion failed for: ${json_file}"
        )
    endif()
endforeach()

if(NOT DEFINED CE_SCRIPT_BASEFILE)
    set(CE_SCRIPT_BASEFILE "startup.as")
endif()

if(NOT DEFINED CE_SCRIPT_DEBUG)
    set(CE_SCRIPT_DEBUG OFF)
endif()

if(NOT IS_ABSOLUTE "${CE_SCRIPT_BASEFILE}")
    set(script_input "${CE_ASSET_SOURCE_DIR}/${CE_SCRIPT_BASEFILE}")
else()
    set(script_input "${CE_SCRIPT_BASEFILE}")
endif()

if(NOT EXISTS "${script_input}")
    message(FATAL_ERROR
        "Startup script does not exist: ${script_input}"
    )
endif()

set(script_output_dir
    "${CE_ASSET_STAGING_DIR}"
)

file(MAKE_DIRECTORY "${script_output_dir}")

set(script_output
    "${script_output_dir}/code.bin"
)

set(script_debug_args "")

if(CE_SCRIPT_DEBUG)
    list(APPEND script_debug_args -g)
endif()

message(STATUS "Compiling script: ${script_input}")
message(STATUS "Script bytecode: ${script_output}")

execute_process(
    COMMAND
        "${CE_SCRIPTC_EXECUTABLE}"
        compile
        "${script_input}"
        -o "${script_output}"
        -I "${CE_ASSET_SOURCE_DIR}"
        ${script_debug_args}
    RESULT_VARIABLE script_result
)

if(NOT "${script_result}" STREQUAL "0")
    message(FATAL_ERROR
        "Script compilation failed: ${script_input}"
    )
endif()


message(STATUS "Creating TCF archive: ${CE_ASSET_ARCHIVE}")

get_filename_component(
    archive_output_dir
    "${CE_ASSET_ARCHIVE}"
    DIRECTORY
)

file(MAKE_DIRECTORY "${archive_output_dir}")

execute_process(
    COMMAND
        "${CE_TCF_EXECUTABLE}"
        pack
        "${CE_ASSET_STAGING_DIR}"
        "${CE_ASSET_ARCHIVE}"
    RESULT_VARIABLE pack_result
)

if(NOT "${pack_result}" STREQUAL "0")
    message(FATAL_ERROR
        "TCF archive creation failed: ${CE_ASSET_ARCHIVE}"
    )
endif()

message(STATUS "TCF archive created successfully.")