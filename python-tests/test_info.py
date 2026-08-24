def test_info(run_vnpuctl):
    proc, data = run_vnpuctl("info")

    assert proc.returncode == 0
    assert data["abi_version"] == 1
    assert data["device_id"] == 0x564E5055
