import json
import subprocess
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[1]
VNPUCTL = REPO_ROOT / "tools" / "vnpuctl"

@pytest.fixture
def run_vnpuctl():
  def _run_vnpuctl(*args, check=True):
    proc = subprocess.run(
        [str(VNPUCTL), *args, "--json"],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    try:
        data = json.loads(proc.stdout)
    except json.JSONDecodeError as exc:
        raise AssertionError(
            f"vnpuctl did not return JSON\n"
            f"exit={proc.returncode}\n"
            f"stdout={proc.stdout}\n"
            f"stderr={proc.stderr}"
        ) from exc

    if check and proc.returncode != 0:
        raise AssertionError(
            f"vnpuctl failed\n"
            f"exit={proc.returncode}\n"
            f"json={data}\n"
            f"stderr={proc.stderr}"
        )

    return proc, data

  return _run_vnpuctl
