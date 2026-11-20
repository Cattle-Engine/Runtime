file(GLOB entries "${EDITOR_DATA_DIR}/*")

foreach(entry IN LISTS entries)
    if(NOT entry STREQUAL "${EDITOR_DATA_DIR}/shaders")
        file(REMOVE_RECURSE "${entry}")
    endif()
endforeach()