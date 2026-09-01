from pathlib import Path
import json
import random
import pytest

from reference_model import dot_i8

NUM_TEST=5

REPO_ROOT = Path(__file__).resolve().parents[1]
INPUT_DIR = REPO_ROOT / "cpp-hal" / "tests" / "inputs" / "temp"
INPUT_FILE = INPUT_DIR / "random.json"


def make_json_input_file():
    return {
        "input_a" : [random.randint(-128, 127) for _ in range(8)],
        "input_b" : [random.randint(-128, 127) for _ in range(8)],
        "timeout_ms" : random.randint(0, 1000),
    } 

def reset_device(run_vnpuctl):
    run_vnpuctl("reset", check=False)

test_cases = [make_json_input_file() for _ in range(NUM_TEST)]

@pytest.mark.parametrize(
        "json_data, case_num",
        [(json_data, i) for i, json_data in enumerate(test_cases, start=1)],
        ids=[f"{i}" for i in range(1, NUM_TEST + 1)],
        )
def test_random_seed_run(run_vnpuctl, json_data, case_num):
    # create json file
    INPUT_JSON_FILE = INPUT_DIR / "random_input.json"
    expected = dot_i8(test_cases[case_num]["input_a"], test_cases[case_num]["input_b"])

    with INPUT_JSON_FILE.open("w", encoding="utf-8") as f:
        json.dump(json_data, f, indent=2)
    
    proc, data = run_vnpuctl("run-dot", "--input", str(INPUT_JSON_FILE), check=False)
    
    assert data["command"] == "run-dot"
    if(data["status"] == "success"):
        assert data["result"] == expected
        assert data["driver_status"] == "ok"
        assert data["device_error"] == 0
    # assert data["status"] == "success"

    # check invalid execute assert

    reset_device(run_vnpuctl);
