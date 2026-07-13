#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:?build directory required}"
SOURCE_DIR="${2:?source directory required}"
TEST_EXECUTABLE="${BUILD_DIR}/KungFuChessTests"
REPORT_DIR="${SOURCE_DIR}/coverage_report"
COVERAGE_INFO="${BUILD_DIR}/coverage.info"

cmake --build "${BUILD_DIR}" --target KungFuChessTests

rm -rf "${REPORT_DIR}" "${COVERAGE_INFO}"
find "${BUILD_DIR}" -name '*.gcda' -delete
lcov --directory "${BUILD_DIR}" --zerocounters

run_tests() {
    local executable="$1"
    if [[ "${executable}" == /mnt/* ]] && grep -qE 'drvfs|9p' /proc/mounts 2>/dev/null; then
        local tmp_executable="/tmp/KungFuChessTests_coverage_$$"
        cp "${executable}" "${tmp_executable}"
        chmod +x "${tmp_executable}"
        "${tmp_executable}"
        rm -f "${tmp_executable}"
    else
        "${executable}"
    fi
}

run_tests "${TEST_EXECUTABLE}"

lcov --directory "${BUILD_DIR}" \
    --capture \
    --output-file "${COVERAGE_INFO}" \
    --ignore-errors source \
    --rc geninfo_unexecuted_blocks=1

lcov --remove "${COVERAGE_INFO}" \
    '/usr/*' \
    '*/tests/*' \
    '*/_deps/*' \
    '*/third_party/*' \
    --output-file "${COVERAGE_INFO}" \
    --ignore-errors unused,inconsistent

lcov --extract "${COVERAGE_INFO}" \
    '*/src/*' \
    --output-file "${COVERAGE_INFO}" \
    --ignore-errors unused,inconsistent

genhtml "${COVERAGE_INFO}" \
    --output-directory "${REPORT_DIR}" \
    --ignore-errors source

echo "Coverage report written to ${REPORT_DIR}/index.html"
