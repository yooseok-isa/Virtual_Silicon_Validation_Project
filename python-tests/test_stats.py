import json
from contextlib import contextmanager
from pathlib import Path

import pytest

from reference_model import dot_i8


REPO_ROOT = Path(__file__).resolve().parents[1]
INPUT_DIR = REPO_ROOT / "cpp-hal" / "tests" / "inputs"
INPUT_FILE = INPUT_DIR / "stats_input.json"
STAT_KEYS = ("submitted", "completed", "timed_out", "device_error", "resets")


def load_input():
    with INPUT_FILE.open() as file:
        return json.load(file)


def read_stats(run_vnpuctl):
    _, data = run_vnpuctl("stats")

    assert data["command"] == "stats"
    assert data["status"] == "success"

    return {key: data[key] for key in STAT_KEYS}


def assert_stats_delta(before, after, **expected_delta):
    actual_delta = {
        key: after[key] - before[key]
        for key in STAT_KEYS
    }
    expected_delta = {
        key: expected_delta.get(key, 0)
        for key in STAT_KEYS
    }

    assert actual_delta == expected_delta, (
        f"before={before}, after={after}, actual_delta={actual_delta}"
    )


@contextmanager
def expect_stats_delta(run_vnpuctl, **expected_delta):
    before = read_stats(run_vnpuctl)
    yield
    after = read_stats(run_vnpuctl)
    assert_stats_delta(before, after, **expected_delta)


def run_with_stats_delta(run_vnpuctl, *args, check=True, **expected_delta):
    with expect_stats_delta(run_vnpuctl, **expected_delta):
        proc, data = run_vnpuctl(*args, check=check)
    return proc, data


def assert_successful_run_dot(data):
    input_data = load_input()

    assert data["command"] == "run-dot"
    assert data["status"] == "success"
    assert data["result"] == dot_i8(input_data["input_a"], input_data["input_b"])
    assert data["driver_status"] == "ok"
    assert data["device_error"] == 0


def test_reset_updates_stats(run_vnpuctl):
    _, data = run_with_stats_delta(
        run_vnpuctl,
        "reset",
        resets=1,
    )

    assert data["command"] == "reset"
    assert data["status"] == "success"


def test_run_dot_success_updates_stats(run_vnpuctl):
    run_vnpuctl("reset", check=False)

    _, data = run_with_stats_delta(
        run_vnpuctl,
        "run-dot",
        "--input",
        str(INPUT_FILE),
        submitted=1,
        completed=1,
    )

    assert_successful_run_dot(data)


def test_multiple_run_dot_successes_update_stats(run_vnpuctl):
    run_vnpuctl("reset", check=False)

    with expect_stats_delta(run_vnpuctl, submitted=3, completed=3):
        for _ in range(3):
            _, data = run_vnpuctl("run-dot", "--input", str(INPUT_FILE))
            assert_successful_run_dot(data)


@pytest.mark.parametrize(
    ("fault", "expected_delta", "expected_error_type", "expected_message"),
    [
        (
            "force-error",
            {"submitted": 1, "device_error": 1},
            "system_error",
            "Device force error",
        ),
        (
            "irq-drop",
            {"submitted": 1, "timed_out": 1},
            "device_error",
            "Device time out",
        ),
        (
            "stuck-busy",
            {"submitted": 1, "timed_out": 1},
            "device_error",
            "Device time out",
        ),
    ],
)
def test_fault_run_dot_updates_stats(
    run_vnpuctl,
    fault,
    expected_delta,
    expected_error_type,
    expected_message,
):
    run_vnpuctl("reset", check=False)
    run_vnpuctl("inject-fault", fault)

    try:
        _, data = run_with_stats_delta(
            run_vnpuctl,
            "run-dot",
            "--input",
            str(INPUT_FILE),
            check=False,
            **expected_delta,
        )

        assert data["command"] == "run-dot"
        assert data["status"] == "error"
        assert data["error_type"] == expected_error_type
        assert data["message"] == expected_message
    finally:
        run_vnpuctl("reset", check=False)


def test_corrupt_result_updates_stats(run_vnpuctl):
    input_data = load_input()
    expected = dot_i8(input_data["input_a"], input_data["input_b"])

    run_vnpuctl("reset", check=False)
    run_vnpuctl("inject-fault", "corrupt-result")

    try:
        _, data = run_with_stats_delta(
            run_vnpuctl,
            "run-dot",
            "--input",
            str(INPUT_FILE),
            submitted=1,
            completed=1,
        )

        assert data["command"] == "run-dot"
        assert data["status"] == "success"
        assert data["driver_status"] == "ok"
        assert data["device_error"] == 0
        assert data["result"] != expected
    finally:
        run_vnpuctl("reset", check=False)
