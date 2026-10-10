if(NOT DEFINED ANGELSCRIPT_LINK_TARGET)
    set(ANGELSCRIPT_LINK_TARGET Angelscript::angelscript)
endif()

if(NOT DEFINED ANGELSCRIPT_ADDON_INCLUDE_DIR)
    find_path(ANGELSCRIPT_ADDON_INCLUDE_DIR
        NAMES "scriptstdstring/scriptstdstring.h"
        PATH_SUFFIXES "include/angelscript"
        REQUIRED
    )
endif()

if(NOT DEFINED ANGELSCRIPT_ADDON_SOURCE_DIR)
    set(ANGELSCRIPT_ADDON_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include/angelscript"
    )
endif()

set(ANGELSCRIPT_ADDON_SOURCES
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/datetime/datetime.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptmath/scriptmath.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scripthandle/scripthandle.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptstdstring/scriptstdstring.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptstdstring/scriptstdstring_utils.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptbuilder/scriptbuilder.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptarray/scriptarray.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptdictionary/scriptdictionary.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptany/scriptany.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/scriptgrid/scriptgrid.cpp"
    "${ANGELSCRIPT_ADDON_SOURCE_DIR}/debugger/debugger.cpp"
)

# Fail early if the expected add-on sources aren't present.
foreach(source IN LISTS ANGELSCRIPT_ADDON_SOURCES)
    if(NOT EXISTS "${source}")
        message(FATAL_ERROR
            "AngelScript add-on source not found: ${source}\n"
            "Set ANGELSCRIPT_ADDON_SOURCE_DIR to the directory "
            "containing the AngelScript add-on .cpp files."
        )
    endif()
endforeach()

# Collect runtime sources
file(GLOB_RECURSE CE_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${CE_RUNTIME_ROOT}/source/*.cpp"
    "${CE_RUNTIME_ROOT}/source/*.c"
)

# Exclude platform-specific sources
if(WIN32)
    list(FILTER CE_RUNTIME_SOURCES
        EXCLUDE REGEX "/platforms/linux/"
    )
elseif(APPLE)
    list(FILTER CE_RUNTIME_SOURCES
        EXCLUDE REGEX "/platforms/linux/"
    )
    list(FILTER CE_RUNTIME_SOURCES
        EXCLUDE REGEX "/platforms/windows/"
    )
elseif(UNIX)
    list(FILTER CE_RUNTIME_SOURCES
        EXCLUDE REGEX "/platforms/windows/"
    )
endif()

function(configure_ce_runtime target)
    set_target_properties(${target} PROPERTIES
        PREFIX ""
    )

    add_dependencies(${target}
        ce_generated_compiler_enum
        ce_generated_binding_registry
    )

    target_sources(${target} PRIVATE
        ${CE_RUNTIME_SOURCES}
        ${CE_GENERATED_BINDING_SOURCES}
        ${ANGELSCRIPT_ADDON_SOURCES}
    )

    target_include_directories(${target}
        PUBLIC
            "${CE_RUNTIME_ROOT}/include/public"
        PRIVATE
            "${CE_GENERATED_BINDINGS_DIR}"
            "${CE_RUNTIME_ROOT}/include"
            "${CE_RUNTIME_ROOT}/include/third_party"
            "${CE_RUNTIME_ROOT}/include/third_party/imgui"
            "${ANGELSCRIPT_ADDON_INCLUDE_DIR}"
            "${CE_GENERATED_DIR}"
    )

    target_compile_definitions(${target} PRIVATE
        CE_BUILDING_LIBRARY
        ENGINE_BUILT_ON_OS="${CE_HOST_OS}"
        CE_DATA_FILE_NAME="${CE_DATA_FILE_NAME}"
        ${CE_PLATFORM_DEFINE}
        TDF_MODE_CE
        $<$<CONFIG:Debug>:CE_DEBUG>
    )

    target_link_libraries(${target} PRIVATE
        ce_common
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        SDL3_mixer::SDL3_mixer
        assimp::assimp
        glm::glm
        zstd::libzstd
        lz4::lz4
        xxHash::xxhash
        imgui::imgui
        ${ANGELSCRIPT_LINK_TARGET}
    )
endfunction()

add_library(ce_runtime STATIC)
configure_ce_runtime(ce_runtime)

add_library(ce_runtime_shared SHARED)
configure_ce_runtime(ce_runtime_shared)

set_target_properties(ce_runtime ce_runtime_shared PROPERTIES
    OUTPUT_NAME ce_runtime
)
