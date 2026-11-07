cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ce_griller LANGUAGES CXX)

find_package(yaml-cpp CONFIG REQUIRED)
find_package(LibArchive REQUIRED)
find_package(CURL REQUIRED)

if(WIN32)
    set(PLATFORM_DIR windows)
elseif(UNIX)
    set(PLATFORM_DIR linux)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/platforms/${PLATFORM_DIR}/*.cpp"
)

add_executable(ce_griller
    ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp
    ${PLATFORM_SOURCES}
)

target_include_directories(ce_griller PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/include/engine/common/misc
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ce_griller PRIVATE
    yaml-cpp::yaml-cpp
    LibArchive::LibArchive
    CURL::libcurl
)

# curl on Linux usually needs threads; sometimes also dl/rt depending on the
# distro's static curl. Harmless to add on other platforms via the guard.
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(ce_griller PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
endif()