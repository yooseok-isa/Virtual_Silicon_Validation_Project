#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PYTEST_CONFIG="$REPO_ROOT/python-tests/pytest.ini"
PYTEST_DIR="$REPO_ROOT/python-tests"
VNPUCTL="$REPO_ROOT/tools/vnpuctl"
ARTIFACT_DIR="${VNPU_TEST_ARTIFACT_DIR:-$REPO_ROOT/artifacts/pytest}"
JUNIT_XML="${VNPU_JUNIT_XML:-$ARTIFACT_DIR/vnpu-revision-a.xml}"
JUNIT_ARG="--junitxml=$JUNIT_XML"

for arg in "$@"; do
    case "$arg" in
        --junitxml|--junitxml=*)
            JUNIT_ARG=""
            ;;
    esac
done

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
if [ -n "$JUNIT_ARG" ]; then
    mkdir -p "$(dirname "$JUNIT_XML")"
    echo "info: writing JUnit XML to $JUNIT_XML" >&2
    exec pytest -c "$PYTEST_CONFIG" "$PYTEST_DIR" "$JUNIT_ARG" "$@"
fi

exec pytest -c "$PYTEST_CONFIG" "$PYTEST_DIR" "$@"
