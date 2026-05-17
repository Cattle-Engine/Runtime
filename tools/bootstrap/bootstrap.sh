#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_SCRIPT="${ROOT_DIR}/tools/build/build.py"

if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 is required for bootstrap on Unix-like systems."
    exit 1
fi

if ! command -v git >/dev/null 2>&1; then
    echo "git is required for bootstrap."
    exit 1
fi

python3 "$BUILD_SCRIPT" "${@:-bootstrap}"
