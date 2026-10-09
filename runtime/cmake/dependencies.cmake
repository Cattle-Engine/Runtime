
find_package(Angelscript CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)

find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

set(ANGELSCRIPT_ADDON_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include/angelscript"
)

set(ANGELSCRIPT_ADDON_SOURCES
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/datetime/datetime.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptmath/scriptmath.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scripthandle/scripthandle.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptstdstring/scriptstdstring.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptstdstring/scriptstdstring_utils.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptbuilder/scriptbuilder.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptarray/scriptarray.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptdictionary/scriptdictionary.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptany/scriptany.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptgrid/scriptgrid.cpp"
    "${ANGELSCRIPT_ADDON_INCLUDE_DIR}/debugger/debugger.cpp"
)