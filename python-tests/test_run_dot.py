from pathlib import Path
import json

from reference_model import dot_i8


REPO_ROOT = Path(__file__).resolve().parents[1]
INPUT_DIR = REPO_ROOT / "cpp-hal" / "tests" / "inputs"
RUN_DOT_CASES = sorted(INPUT_DIR.glob("*.json"))


def pytest_generate_tests(metafunc):
    if "input_file" in metafunc.fixturenames:
        metafunc.parametrize(
            "input_file",
            RUN_DOT_CASES,
            ids=[path.stem for path in RUN_DOT_CASES],
        )


def test_run_dot_from_json_file(run_vnpuctl, input_file):
    with input_file.open() as file:
        input_data = json.load(file)

    proc, data = run_vnpuctl("run-dot", "--input", str(input_file))

    assert proc.returncode == 0
    assert data["result"] == dot_i8(input_data["input_a"], input_data["input_b"])
    assert data["driver_status"] == "ok"
    assert data["device_error"] == 0
