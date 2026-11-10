
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# CMake target names. Override on the command line if yours differ.
set(CE_RELEASE_TCF_TARGET     "tcf"        CACHE STRING "CMake target producing ce_tcf")
set(CE_RELEASE_SCRIPTC_TARGET "ce_scriptc" CACHE STRING "CMake target producing ce_scriptc")
set(CE_RELEASE_LOADER_TARGET  "ce_loader"  CACHE STRING "CMake target producing ce_loader")
set(CE_RELEASE_RUNTIME_TARGET "ce_runtime_shared" CACHE STRING "CMake target producing ce_runtime")
set(CE_RELEASE_COMMON_TARGET  "ce_common"  CACHE STRING "CMake target producing ce_common")

set(CE_RELEASE_VERSION "${PROJECT_VERSION}" CACHE STRING "Version used in the release archive name")

# platform string, must match ce_griller's kKnownPlatforms
if(CE_ARCH STREQUAL "x86_64")
    set(_ce_rel_arch "x64")
elseif(CE_ARCH STREQUAL "arm64")
    set(_ce_rel_arch "arm64")
else()
    set(_ce_rel_arch "${CE_ARCH}")
endif()
set(CE_RELEASE_PLATFORM "${CE_OS_NAME}-${_ce_rel_arch}")

foreach(_t
    ${CE_RELEASE_TCF_TARGET} ${CE_RELEASE_SCRIPTC_TARGET}
    ${CE_RELEASE_LOADER_TARGET} ${CE_RELEASE_RUNTIME_TARGET}
    ${CE_RELEASE_COMMON_TARGET})
    if(NOT TARGET ${_t})
        message(FATAL_ERROR
            "package_release: target '${_t}' does not exist. "
            "Set CE_RELEASE_*_TARGET to the right target name.")
    endif()
endforeach()

set(CE_RELEASE_DIR     "${CMAKE_BINARY_DIR}/release")
set(CE_RELEASE_STAGE   "${CE_RELEASE_DIR}/stage")
set(CE_RELEASE_NAME    "ce_toolchain-${CE_RELEASE_VERSION}-${CE_RELEASE_PLATFORM}")
set(CE_RELEASE_ARCHIVE "${CE_RELEASE_DIR}/${CE_RELEASE_NAME}.tar.gz")
set(CE_RELEASE_TAR_PY  "${CE_RELEASE_DIR}/make_tarball.py")

# copy_directory fails if the source is missing (e.g. no engine shaders)
file(MAKE_DIRECTORY "${CE_SHADER_GENERATED_DIR}")

file(WRITE "${CE_RELEASE_TAR_PY}" [==[
import argparse, hashlib, os, sys, tarfile

LIB_EXTS = (".dll", ".so", ".dylib", ".lib", ".a", ".pdb")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stage", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()

    stage = os.path.abspath(args.stage)
    if not os.path.isdir(os.path.join(stage, "bin")):
        sys.exit("package_release: staging dir has no bin/ directory")

    def norm(ti):
        ti.uid = ti.gid = 0
        ti.uname = ti.gname = ""
        if ti.isdir():
            ti.mode = 0o755
        else:
            is_tool = ti.name.startswith("bin/") and not ti.name.lower().endswith(LIB_EXTS)
            ti.mode = 0o755 if is_tool else 0o644
        return ti

    tmp = args.output + ".tmp"
    os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)

    count = 0
    with tarfile.open(tmp, "w:gz", format=tarfile.PAX_FORMAT) as tf:
        for dirpath, dirnames, filenames in os.walk(stage):
            dirnames.sort()
            rel_dir = os.path.relpath(dirpath, stage)
            if rel_dir != ".":
                tf.add(dirpath, arcname=rel_dir.replace(os.sep, "/"),
                       recursive=False, filter=norm)
            for name in sorted(filenames):
                full = os.path.join(dirpath, name)
                rel = os.path.relpath(full, stage).replace(os.sep, "/")
                tf.add(full, arcname=rel, recursive=False, filter=norm)
                count += 1

    os.replace(tmp, args.output)

    h = hashlib.sha256()
    with open(args.output, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    digest = h.hexdigest()
    with open(args.output + ".sha256", "w") as f:
        f.write("%s  %s\n" % (digest, os.path.basename(args.output)))

    print("package_release: %d files -> %s" % (count, args.output))
    print("package_release: sha256 %s" % digest)

main()
]==])

set(_bin "${CE_RELEASE_STAGE}/bin")
set(_exe "${CMAKE_EXECUTABLE_SUFFIX}")
set(_lib "${CMAKE_SHARED_LIBRARY_SUFFIX}")

add_custom_target(package_release
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CE_RELEASE_STAGE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory   "${_bin}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory   "${CE_RELEASE_STAGE}/shaders/Compiled/SPIRV"

    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${CE_RELEASE_SCRIPTC_TARGET}>" "${_bin}/ce_scriptc${_exe}"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${CE_RELEASE_TCF_TARGET}>"     "${_bin}/ce_tcf${_exe}"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${CE_RELEASE_LOADER_TARGET}>"  "${_bin}/ce_loader${_exe}"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${CE_RELEASE_RUNTIME_TARGET}>" "${_bin}/ce_runtime${_lib}"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${CE_RELEASE_COMMON_TARGET}>"  "${_bin}/ce_common${_lib}"

    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CE_SHADER_GENERATED_DIR}"
            "${CE_RELEASE_STAGE}/shaders/Compiled/SPIRV"

    COMMAND "${Python3_EXECUTABLE}" "${CE_RELEASE_TAR_PY}"
            --stage  "${CE_RELEASE_STAGE}"
            --output "${CE_RELEASE_ARCHIVE}"

    COMMENT "Packaging ${CE_RELEASE_NAME}.tar.gz"
    VERBATIM
)

add_dependencies(package_release
    ${CE_RELEASE_TCF_TARGET}
    ${CE_RELEASE_SCRIPTC_TARGET}
    ${CE_RELEASE_LOADER_TARGET}
    ${CE_RELEASE_RUNTIME_TARGET}
    ${CE_RELEASE_COMMON_TARGET}
    compile_shaders
)

message(STATUS "Release archive: ${CE_RELEASE_ARCHIVE}")