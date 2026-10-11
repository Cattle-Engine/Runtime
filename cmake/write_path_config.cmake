if(NOT DEFINED CE_ASSET_PATH_CONFIG OR
   NOT DEFINED CE_ASSET_DATAFILE_NAME)
    message(FATAL_ERROR "Missing path.cfg configuration variables")
endif()

get_filename_component(
    output_dir
    "${CE_ASSET_PATH_CONFIG}"
    DIRECTORY
)

file(MAKE_DIRECTORY "${output_dir}")

file(
    WRITE "${CE_ASSET_PATH_CONFIG}"
    "${CE_ASSET_DATAFILE_NAME}\n"
)