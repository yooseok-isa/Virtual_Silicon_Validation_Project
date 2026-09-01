from pathlib import Path
import json

from reference_model import dot_i8


REPO_ROOT = Path(__file__).resolve().parents[1]
INPUT_DIR = REPO_ROOT / "cpp-hal" / "tests" / "inputs"
INPUT_FILE = INPUT_DIR / "fault.json"


def load_input():
    with INPUT_FILE.open() as file:
        return json.load(file)


def reset_device(run_vnpuctl):
    run_vnpuctl("reset", check=False)

def test_inject_fault_succes_json_contract(run_vnpuctl):
    _, data = run_vnpuctl("inject-fault", "irq-drop")

    assert data["command"] == "inject-fault"
    assert data["status"] == "success"
    assert data["set_fault"] == "irq-drop"


def test_clear_fault_succes_json_contract(run_vnpuctl):
    run_vnpuctl("inject-fault", "corrupt-result")
    _, data = run_vnpuctl("clear-faults")

    assert data["command"] == "clear-faults"
    assert data["status"] == "success"

def test_inject_irq_drop(run_vnpuctl):
    reset_device(run_vnpuctl)
    run_vnpuctl("inject-fault", "irq-drop")

    try:
        _, data = run_vnpuctl("run-dot", "--input", str(INPUT_FILE), check=False)
        
        assert data["command"] == "run-dot"
        assert data["status"] == "error"
        assert data["error_type"] == "device_error"
        assert data["message"] == "Device time out"
    finally:
        reset_device(run_vnpuctl)


def test_inject_stuck_busy(run_vnpuctl):
    reset_device(run_vnpuctl)
    run_vnpuctl("inject-fault", "stuck-busy")

    try:
        _, data = run_vnpuctl("run-dot", "--input", str(INPUT_FILE), check=False)
        
        assert data["command"] == "run-dot"
        assert data["status"] == "error"
        assert data["error_type"] == "device_error"
        assert data["message"] == "Device time out"
    finally:
        reset_device(run_vnpuctl)


def test_inject_corrupt_result(run_vnpuctl):
    reset_device(run_vnpuctl)
    input_data = load_input()
    expected = dot_i8(input_data["input_a"], input_data["input_b"])

    run_vnpuctl("inject-fault", "corrupt-result")

    try:
        _, data = run_vnpuctl("run-dot", "--input", str(INPUT_FILE))

        assert data["command"] == "run-dot"
        assert data["status"] == "success"
        # assert data["device_error"] == 0
        assert data["result"] != expected
    finally:
        reset_device(run_vnpuctl)


def test_inject_force_error(run_vnpuctl):
    reset_device(run_vnpuctl)
    run_vnpuctl("inject-fault", "force-error")

    try:
        _, data = run_vnpuctl("run-dot", "--input", str(INPUT_FILE), check=False)

        assert data["command"] == "run-dot"
        assert data["status"] == "error"
        # assert data["device_error"] == 3
    finally:
        reset_device(run_vnpuctl)
