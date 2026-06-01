cmake_minimum_required(VERSION 3.16)

option(USE_CLANG "Use Clang compiler" ON)

set(CE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CE_BUILD_SCRIPT "${CE_ROOT_DIR}/tools/build/build.py")
set(CE_BOOTSTRAP_SH "${CE_ROOT_DIR}/tools/bootstrap/bootstrap.sh")
set(CE_BOOTSTRAP_PS1 "${CE_ROOT_DIR}/tools/bootstrap/bootstrap.ps1")
set(CE_VCPKG_ROOT "${CE_ROOT_DIR}/tools/cache/vcpkg/vcpkg")
set(CE_VCPKG_TOOLCHAIN "${CE_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
if(WIN32)
    set(CE_VCPKG_EXE "${CE_VCPKG_ROOT}/vcpkg.exe")
else()
    set(CE_VCPKG_EXE "${CE_VCPKG_ROOT}/vcpkg")
endif()

if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg target triplet")
    elseif(APPLE)
        set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "Vcpkg target triplet")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "Vcpkg target triplet")
    endif()
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    else()
        if(NOT EXISTS "${CE_VCPKG_TOOLCHAIN}" OR NOT EXISTS "${CE_VCPKG_EXE}")
            message(STATUS "Bootstrapping VCPKG before project configuration...")
            if(WIN32)
                execute_process(
                    COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File "${CE_BOOTSTRAP_PS1}" bootstrap
                    WORKING_DIRECTORY "${CE_ROOT_DIR}"
                    RESULT_VARIABLE CE_BOOTSTRAP_RESULT
                )
            else()
                execute_process(
                    COMMAND bash "${CE_BOOTSTRAP_SH}" bootstrap
                    WORKING_DIRECTORY "${CE_ROOT_DIR}"
                    RESULT_VARIABLE CE_BOOTSTRAP_RESULT
                )
            endif()

            if(NOT CE_BOOTSTRAP_RESULT EQUAL 0)
                message(FATAL_ERROR "Failed to bootstrap VCPKG for this project.")
            endif()
        endif()

        set(CMAKE_TOOLCHAIN_FILE "${CE_VCPKG_TOOLCHAIN}"
            CACHE STRING "Vcpkg toolchain file")
    endif()
endif()

if(USE_CLANG)
    find_program(CE_CLANG_EXE clang)
    find_program(CE_CLANGXX_EXE clang++)
    if(CE_CLANG_EXE AND CE_CLANGXX_EXE)
        set(CMAKE_C_COMPILER "${CE_CLANG_EXE}" CACHE FILEPATH "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "${CE_CLANGXX_EXE}" CACHE FILEPATH "C++ compiler" FORCE)
    else()
        message(WARNING "USE_CLANG is ON but clang/clang++ were not found; using default compilers.")
    endif()
endif()

project(ce_engine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32 AND EXISTS "${CE_ROOT_DIR}/tools/cache/python/windows-embed/python.exe")
    set(Python3_EXECUTABLE "${CE_ROOT_DIR}/tools/cache/python/windows-embed/python.exe"
        CACHE FILEPATH "Python interpreter for CE build helpers")
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

if(WIN32)
    set(HOST_OS "Windows")
    set(PLATFORM_FLAGS -DPLATFORM_WINDOWS)

elseif(APPLE)
    set(HOST_OS "macOS")
    set(PLATFORM_FLAGS -DPLATFORM_MACOS)

elseif(UNIX)
    set(HOST_OS "Linux")
    set(PLATFORM_FLAGS -DPLATFORM_LINUX)
endif()

set(DATA_FILE_NAME data.tcf)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    source/*.cpp
    source/*.c
)

if(WIN32)
    list(FILTER SOURCES EXCLUDE REGEX "/platforms/linux/")
elseif(APPLE)
    list(FILTER SOURCES EXCLUDE REGEX "/platforms/windows/")
    list(FILTER SOURCES EXCLUDE REGEX "/platforms/linux/")
elseif(UNIX)
    list(FILTER SOURCES EXCLUDE REGEX "/platforms/windows/")
endif()

file(GLOB CE_SHADER_SOURCES CONFIGURE_DEPENDS
    "${CE_ROOT_DIR}/shaders/*.frag"
    "${CE_ROOT_DIR}/shaders/*.vert"
)

file(GLOB_RECURSE CE_ASSET_INPUTS CONFIGURE_DEPENDS
    "${CE_ROOT_DIR}/assets/*"
)

file(GLOB_RECURSE CE_JSON_ASSET_INPUTS CONFIGURE_DEPENDS
    "${CE_ROOT_DIR}/assets/*.json"
)

find_package(Angelscript CONFIG REQUIRED)

find_path(
    ANGELSCRIPT_ADDON_INCLUDE_DIR
    NAMES scriptstdstring/scriptstdstring.h
    PATH_SUFFIXES include/angelscript
)

if(NOT ANGELSCRIPT_ADDON_INCLUDE_DIR)
    message(FATAL_ERROR
        "AngelScript add-on sources were not found. "
        "Install the 'angelscript[addons]' vcpkg feature before configuring."
    )
endif()

set(ANGELSCRIPT_ADDON_SOURCES
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}/datetime/datetime.cpp
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptmath/scriptmath.cpp
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scripthandle/scripthandle.cpp
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptstdstring/scriptstdstring.cpp
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptbuilder/scriptbuilder.cpp
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptarray/scriptarray.cpp
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptdictionary/scriptdictionary.cpp
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptany/scriptany.cpp
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}/scriptgrid/scriptgrid.cpp
)

foreach(ANGELSCRIPT_ADDON_SOURCE ${ANGELSCRIPT_ADDON_SOURCES})
    if(NOT EXISTS "${ANGELSCRIPT_ADDON_SOURCE}")
        message(FATAL_ERROR
            "Missing AngelScript add-on source '${ANGELSCRIPT_ADDON_SOURCE}'. "
            "The vcpkg port must be installed with the 'addons' feature."
        )
    endif()
endforeach()

execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CE_BUILD_SCRIPT}" shaders
    WORKING_DIRECTORY "${CE_ROOT_DIR}"
    RESULT_VARIABLE CE_SHADER_PREPARE_RESULT
)

if(NOT CE_SHADER_PREPARE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate shader headers during CMake configure.")
endif()

add_executable(ce_engine ${SOURCES})

target_sources(ce_engine PRIVATE ${ANGELSCRIPT_ADDON_SOURCES})

target_include_directories(ce_engine PRIVATE
    include
    include/third_party
    include/third_party/imgui
    ${ANGELSCRIPT_ADDON_INCLUDE_DIR}
)

target_compile_definitions(ce_engine PRIVATE
    ENGINE_BUILT_ON_OS="${HOST_OS}"
    CE_DATA_FILE_NAME="${DATA_FILE_NAME}"
    ${PLATFORM_FLAGS}
    TDF_MODE_CE
)

if (MSVC)
    target_compile_options(ce_engine PRIVATE
        /W4
        /arch:AVX2
    )
else()
    target_compile_options(ce_engine PRIVATE
        -Wall
        -Wextra
        -mavx2
    )
endif()

if (NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ce_engine PRIVATE -g)
endif()

find_package(SDL3 CONFIG QUIET)

if(SDL3_FOUND)
    target_link_libraries(ce_engine PRIVATE SDL3::SDL3)
else()
    message(FATAL_ERROR "SDL3 not found.")
endif()

find_package(SDL3_image CONFIG QUIET)

if(SDL3_image_FOUND)
    target_link_libraries(ce_engine PRIVATE SDL3_image::SDL3_image)
else()
    message(FATAL_ERROR "SDL3_image not found.")
endif()

find_package(SDL3_ttf CONFIG QUIET)

if(SDL3_ttf_FOUND)
    target_link_libraries(ce_engine PRIVATE SDL3_ttf::SDL3_ttf)
else()
    message(FATAL_ERROR "SDL3_ttf not found.")
endif()

find_package(SDL3_mixer CONFIG QUIET)

if(SDL3_mixer_FOUND)
    target_link_libraries(ce_engine PRIVATE SDL3_mixer::SDL3_mixer)
else()
    message(FATAL_ERROR "SDL3_mixer not found.")
endif()


find_package(glm CONFIG REQUIRED)
target_link_libraries(ce_engine PRIVATE glm::glm)

set(ANGELSCRIPT_LINK_TARGET "")
foreach(CANDIDATE_TARGET Angelscript::angelscript angelscript AngelScript::angelscript AngelScript)
    if(TARGET ${CANDIDATE_TARGET})
        set(ANGELSCRIPT_LINK_TARGET ${CANDIDATE_TARGET})
        break()
    endif()
endforeach()

if(NOT ANGELSCRIPT_LINK_TARGET)
    message(FATAL_ERROR "Could not find an exported AngelScript CMake target.")
endif()

target_link_libraries(ce_engine PRIVATE ${ANGELSCRIPT_LINK_TARGET})

set(CE_SHADER_STAMP "${CMAKE_BINARY_DIR}/generated/shaders.stamp")
add_custom_command(
    OUTPUT "${CE_SHADER_STAMP}"
    COMMAND "${Python3_EXECUTABLE}" "${CE_BUILD_SCRIPT}" shaders
    COMMAND "${CMAKE_COMMAND}" -E touch "${CE_SHADER_STAMP}"
    DEPENDS
        "${CE_BUILD_SCRIPT}"
        ${CE_SHADER_SOURCES}
    WORKING_DIRECTORY "${CE_ROOT_DIR}"
    VERBATIM
)

add_custom_target(ce_generated_shaders DEPENDS "${CE_SHADER_STAMP}")

set(CE_DATA_FILE_PATH "${CMAKE_BINARY_DIR}/${DATA_FILE_NAME}")
add_custom_command(
    OUTPUT "${CE_DATA_FILE_PATH}"
    COMMAND "${Python3_EXECUTABLE}" "${CE_BUILD_SCRIPT}" assets "--build-dir" "${CMAKE_BINARY_DIR}"
    DEPENDS
        "${CE_BUILD_SCRIPT}"
        "${CE_ROOT_DIR}/scripts/json_to_tdf.py"
        "${CE_ROOT_DIR}/scripts/tcf.py"
        "${CE_SHADER_STAMP}"
        ${CE_ASSET_INPUTS}
        ${CE_JSON_ASSET_INPUTS}
    WORKING_DIRECTORY "${CE_ROOT_DIR}"
    VERBATIM
)

add_custom_target(ce_assets ALL DEPENDS "${CE_DATA_FILE_PATH}")
add_dependencies(ce_assets ce_generated_shaders)
add_dependencies(ce_engine ce_generated_shaders ce_assets)

add_custom_target(ce_clean_generated
    COMMAND "${Python3_EXECUTABLE}" "${CE_BUILD_SCRIPT}" clean-generated "--build-dir" "${CMAKE_BINARY_DIR}"
    WORKING_DIRECTORY "${CE_ROOT_DIR}"
    VERBATIM
)

if(WIN32)

    if(TARGET SDL3::SDL3main)
        target_link_libraries(ce_engine PRIVATE SDL3::SDL3main)
    endif()

    target_link_libraries(ce_engine PRIVATE
        opengl32
        comdlg32
        ole32
        oleaut32
        uuid
        imm32
        user32
        gdi32
        shell32
    )

endif()

install(TARGETS ce_engine DESTINATION bin)

add_custom_target(run
    COMMAND ce_engine
    DEPENDS ce_engine ce_assets
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
