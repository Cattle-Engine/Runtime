find_package(Git QUIET)

if (GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CE_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CE_GIT_HASH_FULL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CE_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CE_GIT_TAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_STATUS STREQUAL "")
        set(CE_GIT_ISDIRTY "false")
    else()
        set(CE_GIT_ISDIRTY "true")
    endif()
else()
    set(CE_GIT_HASH "unknown")
    set(CE_GIT_HASH_FULL "unknown")
    set(CE_GIT_BRANCH "unknown")
    set(CE_GIT_TAGS "unknown")
    set(CE_GIT_ISDIRTY "unknown")
endif()

file(MAKE_DIRECTORY "${CE_GENERATED_DIR}")
configure_file(
    "${CE_RUNTIME_ROOT}/include/git_version.hpp.in"
    "${CE_GENERATED_DIR}/git_version.hpp"
    @ONLY
)

set(CE_COMPILER_ENUM_SOURCE "${CE_RUNTIME_ROOT}/include/engine/compilers/gcc/enum_to_string_impl.inl")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CE_COMPILER_ENUM_SOURCE "${CE_RUNTIME_ROOT}/include/engine/compilers/gcc/enum_to_string_impl.inl")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CE_COMPILER_ENUM_SOURCE "${CE_RUNTIME_ROOT}/include/engine/compilers/clang/enum_to_string_impl.inl")
elseif(MSVC)
    set(CE_COMPILER_ENUM_SOURCE "${CE_RUNTIME_ROOT}/include/engine/compilers/msvc/enum_to_string_impl.inl")
endif()

add_custom_command(
    OUTPUT "${CE_GENERATED_DIR}/enum_to_string_impl.inl"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CE_COMPILER_ENUM_SOURCE}" "${CE_GENERATED_DIR}/enum_to_string_impl.inl"
    DEPENDS "${CE_COMPILER_ENUM_SOURCE}"
    COMMENT "Generating compiler enum impl"
    VERBATIM
)

add_custom_target(ce_generated_compiler_enum ALL DEPENDS "${CE_GENERATED_DIR}/enum_to_string_impl.inl")

add_custom_target(ce_clean_generated
    COMMAND ${CMAKE_COMMAND} -E rm -f
        "${CE_GENERATED_DIR}/git_version.hpp"
        "${CE_GENERATED_DIR}/enum_to_string_impl.inl"
        "${CE_GENERATED_BINDINGS_DIR}/binding_registry.hpp"
        ${CE_GENERATED_BINDING_OUTPUTS}
    COMMENT "Cleaning generated runtime files"
)

set(CE_BINDING_REGISTRY_HEADER "${CE_GENERATED_BINDINGS_DIR}/binding_registry.hpp")
add_custom_command(
    OUTPUT "${CE_BINDING_REGISTRY_HEADER}"
    COMMAND "${Python3_EXECUTABLE}" "${CE_RUNTIME_ROOT}/cmake/generate_binding_registry.py" "${CE_GENERATED_BINDINGS_DIR}"
    DEPENDS
        "${CE_RUNTIME_ROOT}/cmake/generate_binding_registry.py"
        ${CE_GENERATED_BINDING_METADATA}
    WORKING_DIRECTORY "${CE_RUNTIME_ROOT}"
    COMMENT "Generating binding registry"
    VERBATIM
)

add_custom_target(ce_generated_binding_registry ALL DEPENDS "${CE_BINDING_REGISTRY_HEADER}")
add_dependencies(ce_generated_binding_registry ce_generated_idl)
