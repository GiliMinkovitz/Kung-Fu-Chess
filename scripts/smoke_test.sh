#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

READY_URL="${KFC_GATEWAY_READY_URL:-http://localhost:8080/ready}"
READY_INTERVAL="${KFC_SMOKE_READY_INTERVAL_S:-2}"
READY_TIMEOUT="${KFC_SMOKE_READY_TIMEOUT_S:-120}"

echo "==> Kung Fu Chess smoke test"
echo "==> Waiting for gateway readiness: ${READY_URL}"

deadline=$((SECONDS + READY_TIMEOUT))
while true; do
  if curl -fsS "${READY_URL}" >/dev/null 2>&1; then
    echo "==> Gateway is ready"
    break
  fi

  if (( SECONDS >= deadline )); then
    echo "ERROR: gateway not ready after ${READY_TIMEOUT}s (${READY_URL})" >&2
    exit 1
  fi

  echo "    not ready yet, retrying in ${READY_INTERVAL}s..."
  sleep "${READY_INTERVAL}"
done

cd "${REPO_ROOT}"

if command -v python3 >/dev/null 2>&1; then
  PYTHON=python3
elif command -v python >/dev/null 2>&1; then
  PYTHON=python
else
  echo "ERROR: python3 or python not found in PATH" >&2
  exit 1
fi

echo "==> Running matchmaking smoke test"
"${PYTHON}" tests/smoke/matchmaking_smoke.py
