#!/usr/bin/env bash
set -euo pipefail

VCPKG_ROOT="${VCPKG_ROOT:-"${PWD}/vcpkg"}"

if [[ ! -d "${VCPKG_ROOT}" ]]; then
  git clone --depth 1 https://github.com/microsoft/vcpkg "${VCPKG_ROOT}"
fi

if [[ ! -x "${VCPKG_ROOT}/vcpkg" ]]; then
  "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics
fi

echo "VCPKG_ROOT=${VCPKG_ROOT}"
