#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/tests/build"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

qmake "${ROOT_DIR}/tests/tests.pro"
make -j"$(nproc)"
./kylin_service_tests

if command -v gcovr >/dev/null 2>&1; then
  gcovr \
    --root "${ROOT_DIR}" \
    --filter "${ROOT_DIR}/service/cryptohelper.cpp" \
    --filter "${ROOT_DIR}/service/asyncmonitor.cpp" \
    --exclude-directories "${ROOT_DIR}/tests" \
    --print-summary \
    --fail-under-line 70
else
  echo "gcovr not installed; skipping coverage threshold check."
fi
