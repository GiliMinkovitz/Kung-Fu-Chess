#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="${ROOT}/third_party/CTD26/cpp/OpenCV_451"

if [[ -d "${DEST}/include/opencv2" ]]; then
    echo "OpenCV already at ${DEST}"
    exit 0
fi

SOURCE=""
for candidate in \
    "${ROOT}/third_party/OpenCV_451-20260714T120224Z-1-001/OpenCV_451" \
    "${ROOT}/third_party/OpenCV_451"; do
    if [[ -d "${candidate}/include/opencv2" ]]; then
        SOURCE="${candidate}"
        break
    fi
done

if [[ -z "${SOURCE}" ]]; then
    echo "ERROR: Could not find OpenCV_451 (expected include/opencv2)." >&2
    exit 1
fi

mkdir -p "$(dirname "${DEST}")"
mv "${SOURCE}" "${DEST}"
echo "Moved OpenCV to ${DEST}"
