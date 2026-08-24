#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTEST_CONFIG="$REPO_ROOT/python-tests/pytest.ini"
PYTEST_DIR="$REPO_ROOT/python-tests"
VNPUCTL="$REPO_ROOT/tools/vnpuctl"

if ! command -v pytest >/dev/null 2>&1; then
    echo "error: pytest not found in PATH" >&2
    echo "hint: install pytest in the guest or run from an environment that has pytest" >&2
    exit 127
fi

if [ ! -x "$VNPUCTL" ]; then
    echo "error: vnpuctl is missing or not executable: $VNPUCTL" >&2
    echo "hint: run 'make -C cpp-hal' first" >&2
    exit 1
fi

if [ ! -e /dev/vnpu0 ]; then
    echo "error: /dev/vnpu0 not found" >&2
    echo "hint: load the driver first, e.g. insmod linux-driver/vnpu-drv.ko" >&2
    exit 1
fi

cd "$REPO_ROOT"
exec pytest -c "$PYTEST_CONFIG" "$PYTEST_DIR" "$@"
