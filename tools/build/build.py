#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = ROOT / "build"
ASSET_DIR = ROOT / "assets"
SCRIPT_DIR = ROOT / "scripts"
SHADER_SOURCE_DIR = ROOT / "shaders"
SHADER_HEADER_DIR = ROOT / "include" / "engine" / "renderers" / "shaders"
SHADER_ASSET_DIR = ASSET_DIR / "shaders" / "Compiled" / "SPIRV"
DATA_FILE_NAME = "data.tcf"
VCPKG_ROOT = ROOT / "tools" / "cache" / "vcpkg" / "vcpkg"


def log(message: str) -> None:
    print(f"[build.py] {message}")


def run(cmd: list[str], *, cwd: Path | None = None, env: dict[str, str] | None = None) -> None:
    pretty = " ".join(str(part) for part in cmd)
    log(f"Running: {pretty}")
    subprocess.run(cmd, cwd=cwd or ROOT, env=env, check=True)


def detect_triplet() -> str:
    if sys.platform.startswith("win"):
        return "x64-windows"
    if sys.platform == "darwin":
        return "x64-osx"
    return "x64-linux"


def vcpkg_executable() -> Path:
    return VCPKG_ROOT / ("vcpkg.exe" if os.name == "nt" else "vcpkg")


def vcpkg_env() -> dict[str, str]:
    env = os.environ.copy()
    env["VCPKG_ROOT"] = str(VCPKG_ROOT)
    return env


def ensure_vcpkg() -> None:
    if not VCPKG_ROOT.exists():
        run(["git", "clone", "--depth", "1", "https://github.com/microsoft/vcpkg", str(VCPKG_ROOT)])

    exe = vcpkg_executable()
    if exe.exists():
        return

    bootstrap = VCPKG_ROOT / ("bootstrap-vcpkg.bat" if os.name == "nt" else "bootstrap-vcpkg.sh")
    if os.name == "nt":
        run(["cmd", "/c", str(bootstrap), "-disableMetrics"], cwd=VCPKG_ROOT)
    else:
        run([str(bootstrap), "-disableMetrics"], cwd=VCPKG_ROOT)


def ensure_vcpkg_dependencies(triplet: str) -> None:
    ensure_vcpkg()
    run([str(vcpkg_executable()), "install", "--triplet", triplet], cwd=ROOT, env=vcpkg_env())


def locate_glslc(triplet: str) -> Path:
    candidates = [
        os.environ.get("GLSLC"),
        shutil.which("glslc"),
        ROOT / "vcpkg_installed" / triplet / "tools" / "shaderc" / ("glslc.exe" if os.name == "nt" else "glslc"),
        ROOT / "vcpkg_installed" / triplet / "tools" / "shaderc" / "glslc",
        VCPKG_ROOT / "downloads" / "tools" / "shaderc" / ("glslc.exe" if os.name == "nt" else "glslc"),
    ]

    for candidate in candidates:
        if not candidate:
            continue
        path = Path(candidate)
        if path.exists():
            return path

    raise FileNotFoundError(
        "Could not find 'glslc'. Install the VCPKG dependencies first or set the GLSLC environment variable."
    )


def iter_shader_sources() -> Iterable[Path]:
    return sorted(
        [*SHADER_SOURCE_DIR.glob("*.vert"), *SHADER_SOURCE_DIR.glob("*.frag")],
        key=lambda path: path.name,
    )


def shader_symbol_name(shader_path: Path) -> str:
    return f"{shader_path.stem}_{shader_path.suffix.lstrip('.')}_spv"


def generated_shader_header_path(shader_path: Path) -> Path:
    return SHADER_HEADER_DIR / f"{shader_symbol_name(shader_path)}.h"


def write_shader_header(symbol_name: str, payload: bytes, header_path: Path) -> None:
    lines = [f"unsigned char {symbol_name}[] = {{"]
    for offset in range(0, len(payload), 12):
        chunk = payload[offset : offset + 12]
        values = ", ".join(f"0x{byte:02x}" for byte in chunk)
        suffix = "," if offset + len(chunk) < len(payload) else ""
        lines.append(f"  {values}{suffix}")
    lines.append("};")
    lines.append(f"unsigned int {symbol_name}_len = {len(payload)};")
    lines.append("")

    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text("\n".join(lines), encoding="utf-8")


def build_shaders(triplet: str) -> None:
    glslc = locate_glslc(triplet)
    SHADER_ASSET_DIR.mkdir(parents=True, exist_ok=True)

    for shader_path in iter_shader_sources():
        symbol_name = shader_symbol_name(shader_path)
        tmp_spv = BUILD_DIR / "generated" / "shaders" / f"{shader_path.name}.spv"
        tmp_spv.parent.mkdir(parents=True, exist_ok=True)

        run([str(glslc), str(shader_path), "-o", str(tmp_spv)])

        spv_bytes = tmp_spv.read_bytes()
        header_path = generated_shader_header_path(shader_path)
        asset_path = SHADER_ASSET_DIR / f"{shader_path.stem}.{shader_path.suffix.lstrip('.')}.spv"

        write_shader_header(symbol_name, spv_bytes, header_path)
        shutil.copyfile(tmp_spv, asset_path)
        log(f"Generated {header_path.relative_to(ROOT)} and {asset_path.relative_to(ROOT)}")


def stage_assets(build_dir: Path) -> Path:
    staged_assets = build_dir / "assets"
    if staged_assets.exists():
        shutil.rmtree(staged_assets)
    shutil.copytree(ASSET_DIR, staged_assets)
    return staged_assets


def generate_tdf_assets(staged_assets: Path) -> None:
    json_files = sorted(ASSET_DIR.rglob("*.json"))
    for json_file in json_files:
        relative_parent = json_file.relative_to(ASSET_DIR).parent
        out_dir = staged_assets / relative_parent
        run(
            [
                sys.executable,
                str(SCRIPT_DIR / "json_to_tdf.py"),
                "--mode",
                "generic",
                "--out-dir",
                str(out_dir),
                str(json_file),
            ]
        )


def pack_assets(build_dir: Path, staged_assets: Path) -> None:
    run(
        [
            sys.executable,
            str(SCRIPT_DIR / "tcf.py"),
            "pack",
            str(staged_assets),
            str(build_dir / DATA_FILE_NAME),
        ]
    )


def build_assets(build_dir: Path) -> None:
    staged_assets = stage_assets(build_dir)
    generate_tdf_assets(staged_assets)
    pack_assets(build_dir, staged_assets)
    log(f"Packed assets into {build_dir / DATA_FILE_NAME}")


def configure_cmake(build_dir: Path, triplet: str) -> None:
    toolchain = VCPKG_ROOT / "scripts" / "buildsystems" / "vcpkg.cmake"
    cmd = [
        "cmake",
        "-S",
        str(ROOT),
        "-B",
        str(build_dir),
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        f"-DVCPKG_TARGET_TRIPLET={triplet}",
    ]
    run(cmd, env=vcpkg_env())


def cmake_build(build_dir: Path, config: str) -> None:
    cmd = ["cmake", "--build", str(build_dir)]
    if sys.platform.startswith("win"):
        cmd.extend(["--config", config])
    run(cmd)


def clean_generated_artifacts(build_dir: Path, *, remove_build_dir: bool) -> None:
    if build_dir.exists():
        if remove_build_dir:
            shutil.rmtree(build_dir)
            log(f"Removed {build_dir}")
        else:
            generated_dir = build_dir / "generated"
            if generated_dir.exists():
                shutil.rmtree(generated_dir)
                log(f"Removed {generated_dir}")

            data_file = build_dir / DATA_FILE_NAME
            if data_file.exists():
                data_file.unlink()
                log(f"Removed {data_file}")

            staged_assets = build_dir / "assets"
            if staged_assets.exists():
                shutil.rmtree(staged_assets)
                log(f"Removed {staged_assets}")

    if SHADER_ASSET_DIR.exists():
        shutil.rmtree(SHADER_ASSET_DIR)
        log(f"Removed {SHADER_ASSET_DIR}")

    compiled_root = SHADER_ASSET_DIR.parent
    if compiled_root.exists() and not any(compiled_root.iterdir()):
        compiled_root.rmdir()
        log(f"Removed {compiled_root}")

    for shader_path in iter_shader_sources():
        header_path = generated_shader_header_path(shader_path)
        if header_path.exists():
            header_path.unlink()
            log(f"Removed {header_path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build helper for CE using VCPKG + CMake.")
    parser.add_argument(
        "command",
        nargs="?",
        default="full",
        choices=["bootstrap", "shaders", "assets", "configure", "build", "full", "clean", "clean-generated"],
        help="Operation to run. Defaults to the full build pipeline.",
    )
    parser.add_argument("--build-dir", type=Path, default=BUILD_DIR, help="CMake build directory.")
    parser.add_argument("--triplet", default=detect_triplet(), help="VCPKG target triplet.")
    parser.add_argument("--config", default="Debug", help="CMake configuration on multi-config generators.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    build_dir = args.build_dir.resolve()
    if args.command == "clean":
        clean_generated_artifacts(build_dir, remove_build_dir=True)
        return 0

    if args.command == "clean-generated":
        clean_generated_artifacts(build_dir, remove_build_dir=False)
        return 0

    if args.command in {"bootstrap", "full", "configure", "build"}:
        ensure_vcpkg_dependencies(args.triplet)

    if args.command in {"shaders", "full"}:
        build_shaders(args.triplet)

    if args.command in {"assets", "full"}:
        build_assets(build_dir)

    if args.command in {"configure", "full"}:
        configure_cmake(build_dir, args.triplet)

    if args.command == "bootstrap":
        return 0

    if args.command in {"build", "full"}:
        if args.command == "build" and not build_dir.exists():
            configure_cmake(build_dir, args.triplet)
        cmake_build(build_dir, args.config)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
