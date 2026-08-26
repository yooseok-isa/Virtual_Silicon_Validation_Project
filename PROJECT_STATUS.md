# PROJECT_STATUS

Last updated: 2026-08-26

## Current Position

The project is currently in **Milestone 5 - Revision B and Python Validation**, early/in-progress.

Milestones 0 through 4 have enough implementation to support guest-based end-to-end testing through `vnpuctl`, but they are not all formally closed. The main remaining M4 gaps are `MockVnpuDevice`, GoogleTest unit tests, stable JSON schema documentation/evidence, and CMake verification. M5 has started with pytest functional and fault tests, but Revision B and full regression automation are not implemented yet.

## Milestone 0 - Reproducible QEMU Guest Environment

### Goal

Create a pinned, reproducible baseline environment that can boot an x86_64 Buildroot Linux guest under QEMU and run basic PCI inspection.

### Required Work

- Define pinned QEMU/Linux/Buildroot versions.
- Build an unmodified baseline QEMU and Buildroot guest.
- Provide a serial QEMU boot script.
- Record boot and `lspci` evidence.
- Keep generated build outputs outside version control.
- Add source-fetch/setup automation and CI skeleton.

### Completed

- Pinned versions and artifact hashes are recorded in `docs/versions.md`.
- Buildroot-generated kernel/rootfs artifacts are present under `output/images/`.
- Baseline guest boot, root shell, and baseline `lspci` were re-recorded with matching output.
- Serial boot script exists at `scripts/run-qemu.sh`.
- Source-fetch/setup automation exists at `scripts/setup-sources.sh`.
- Reproducible Buildroot defconfig exists at `buildroot/configs/vnpu_qemu_x86_64_defconfig`.
- CI skeleton exists at `.github/workflows/ci.yml`.

### In Progress

- End-to-end setup from a completely clean checkout has not been executed yet.
- CI has been added, but has not run on GitHub Actions yet.

### Not Completed

- Clean-checkout reproduction evidence is not recorded.
- GitHub Actions run evidence is not recorded.

### Evidence

- Local syntax check: `sh -n scripts/run-test.sh scripts/device_reboot.sh`
- Existing boot/evidence references are recorded in `docs/versions.md`.

## Milestone 1 - Minimal QEMU PCI Enumeration

### Goal

Add a minimal custom QEMU PCI device that enumerates in the guest with a local PCI ID and exposes a 4 KiB BAR0 register window.

### Required Work

- Add QEMU VNPU PCI device skeleton.
- Define local PCI vendor/device IDs.
- Register BAR0 as a 4 KiB MMIO window.
- Implement fixed `DEVICE_ID`, `REVISION`, and reserved `CAPABILITIES` registers.
- Add basic enumeration and register reset-value tests.

### Completed

- QEMU VNPU PCI device source exists at `qemu-device/src/vnpu.c`.
- Reproducible QEMU patch exists at `qemu-device/patches/0001-hw-misc-add-vnpu-pci-device.patch`.
- Patch covers `hw/misc/Kconfig`, `hw/misc/meson.build`, and `hw/misc/vnpu.c`.
- `scripts/run-qemu.sh` launches QEMU with `-device vnpu`.
- PCI IDs are defined in source: vendor `0x1B36`, device `0x1000`.
- BAR0 is implemented in source as a 4 KiB MMIO region.
- Guest `lspci -nnvv` and QEMU monitor evidence were previously recorded by the user.

### In Progress

- Runtime enumeration is manually verified, but not yet represented as a pytest or CI test.

### Not Completed

- Automated PCI enumeration test is not implemented.
- Automated reset-value test for `DEVICE_ID`, `REVISION`, and `CAPABILITIES` is not implemented.

### Evidence

- QEMU patch dry-run was previously verified against pristine QEMU 10.2.0.
- Guest evidence previously confirmed VNPU PCI ID `1b36:1000`.
- QEMU monitor evidence previously showed a 4 KiB BAR0 window.

## Milestone 2 - Functional QEMU VNPU Device Model

### Goal

Implement Revision A VNPU behavior: MMIO command submission, signed INT8 dot product, asynchronous completion, legacy INTx signaling, and deterministic fault injection.

### Required Work

- Implement input, control, status, result, IRQ, error, and fault registers.
- Implement signed INT8 dot product for Revision A vector length 8.
- Implement timer-based asynchronous completion.
- Implement legacy INTx signaling.
- Implement `IRQ_DROP`, `STUCK_BUSY`, `CORRUPT_RESULT`, and `FORCE_ERROR`.
- Document the state machine and add runtime behavior tests.

### Completed

- Revision A register model is implemented in `qemu-device/src/vnpu.c`.
- MMIO input/control/status/result/error/IRQ/fault registers are implemented in source.
- Signed INT8 dot product for vector length 8 is implemented in source.
- Timer-based asynchronous completion is implemented in source.
- Legacy INTx-style interrupt signaling is implemented through `pci_set_irq()`.
- Runtime faults `IRQ_DROP`, `STUCK_BUSY`, `CORRUPT_RESULT`, and `FORCE_ERROR` are implemented in source.
- devmem-based smoke test exists at `qemu-device/tests/scripts/vnpu-devmem-smoke.sh`.
- userspace mmap smoke tool exists at `qemu-device/tests/src/vnpu-mmio-smoke.c`.
- Guest devmem smoke test passed for reset register reads, dot8 result, and `CORRUPT_RESULT` behavior.

### In Progress

- M2 runtime validation exists as smoke coverage, but not as complete regression coverage.
- State-machine documentation exists in `docs/register_map.md`, but still needs status cleanup from planned-only wording.

### Not Completed

- Interrupt-enabled completion test is not implemented at the MMIO-only level.
- Full fault behavior coverage is not complete at the MMIO-only level.
- Invalid/unsupported MMIO access robustness coverage is partial.
- `docs/register_map.md` has not been fully updated to match implemented/verified behavior.

### Evidence

- Local build: `make -C qemu-device/tests`
- Guest devmem smoke evidence previously recorded:
  - BDF: `0000:00:03.0`
  - BAR0: `0xfebd5000-0xfebd5fff` (`4096` bytes)
  - `DEVICE_ID`: `0x564e5055`
  - `REVISION`: `0x1`
  - `CAPABILITIES`: `0x0`
  - `VECTOR_LENGTH after reset`: `0x8`
  - `dot8 result`: `0x24`
  - `corrupt-result fault result`: `0x25`
  - Result: `PASS: VNPU devmem smoke test`

## Milestone 3 - Linux PCI Driver

### Goal

Implement a Linux PCI kernel driver and character-device UAPI that controls the VNPU device through BAR0 MMIO and interrupt-driven completion.

### Required Work

- Implement PCI probe/remove and BAR0 mapping.
- Implement IRQ handler.
- Register `/dev/vnpu0` character device.
- Define and implement stable `ioctl` ABI.
- Implement one-operation-at-a-time command submission.
- Implement timeout handling, reset recovery, stats, logging, and failure-path cleanup.
- Add driver build/test scripts and runtime evidence.

### Completed

- Out-of-tree kernel module Makefile exists at `linux-driver/Makefile`.
- Linux PCI driver source exists at `linux-driver/src/vnpu_drv.c`.
- UAPI header exists at `linux-driver/include/vnpu_uapi.h`.
- Kbuild integration builds `vnpu-drv.ko` with `make -C linux-driver`.
- PCI probe/remove callbacks, BAR0 mapping, IRQ handler, miscdevice registration, and ioctl dispatch are present in source.
- Userspace ioctl smoke test source exists at `linux-driver/tests/vnpu-ioctl-smoke.c`.
- `linux-driver/Makefile` builds both `vnpu-drv.ko` and `tests/vnpu-ioctl-smoke`.
- `VNPU_IOCTL_SET_FAULT` now copies the user request before validating `fault_mask`.
- Guest `insmod`, `/dev/vnpu0`, `dmesg`, `lspci -k`, ioctl behavior, and `rmmod` were reported working by the user.
- Guest-side PCI/device reinitialization helper exists at `scripts/device_reboot.sh`.

### In Progress

- Driver has working manual guest evidence, but the logs are not checked into the repository.
- `scripts/device_reboot.sh` has been syntax-checked locally, but has not yet been recorded as guest-passed evidence.
- Runtime test coverage is now moving from manual smoke checks into pytest-driven end-to-end checks.

### Not Completed

- Probe-time validation of BAR0 size, MMIO `DEVICE_ID`, MMIO `REVISION`, and `CAPABILITIES` is incomplete.
- Automated repeated module reload test is not implemented.
- Captured guest logs for current reorganized artifacts are not recorded.

### Evidence

- Local build: `make -C linux-driver`
- Observed result: `vnpu-drv.ko` built successfully.
- Kernel build warning remains: Buildroot compiler string differs as `2026.02.2` vs `2026.02.2-dirty`, but module build succeeds.

## Milestone 4 - C++ HAL and Validation CLI

### Goal

Provide a C++ HAL and `vnpuctl` userspace CLI on top of `/dev/vnpu0` so manual validation and pytest automation can use stable high-level commands instead of raw ioctl packing.

### Required Work

- Define the `IVnpuDevice` C++ HAL interface.
- Implement `LinuxVnpuDevice` using `/dev/vnpu0` ioctl calls.
- Implement a deterministic `MockVnpuDevice` backend for unit tests.
- Implement stable `vnpuctl` JSON commands.
- Add CMake integration for HAL and CLI builds.
- Add HAL unit tests and CLI validation tests.

### Completed

- Repository layout has been aligned toward `CONTEXT.md` with `cpp-hal/include/vnpu/`, `cpp-hal/tests/`, `python-tests/`, `guest/`, `ci/helpers/`, `LICENSES/`, and `docs/adr/`.
- Root `tools/` is treated as an output directory for the built `vnpuctl` binary.
- Canonical HAL header exists at `cpp-hal/include/vnpu/vnpu-hal.hpp`.
- Compatibility wrapper header exists at `cpp-hal/include/vnpu-hal.hpp`.
- `LinuxVnpuDevice` implementation exists at `cpp-hal/src/vnpu-hal.cpp`.
- `vnpuctl` source exists at `cpp-hal/src/vnpuctl.cpp`.
- Direct Makefile build exists at `cpp-hal/Makefile` and emits `cpp-hal/src/vnpu-hal.o`, `cpp-hal/src/vnpuctl.o`, and `tools/vnpuctl`.
- CMake build definition exists at `cpp-hal/CMakeLists.txt` as an optional build path.
- `vnpuctl run-dot` input contract is JSON-file-only through `--input <json-file>`.
- `vnpuctl inject-fault` now emits valid JSON string output for `fault_mask`.
- `vnpuctl reset --json` now emits valid JSON.
- User reported `vnpuctl` guest execution working for `info`, `run-dot`, `stats`, `inject-fault`, `clear-faults`, and `reset`.
- User reported success and error outputs are JSON-formatted.

### In Progress

- CLI JSON output is usable by pytest, but the schema is not yet formally documented and locked.
- Error JSON is functional, but command outputs still need schema consistency cleanup and tests for all commands.
- CMake project definition exists, but CMake execution could not be verified on the current host because `cmake` is not installed.

### Not Completed

- `MockVnpuDevice` backend is not implemented.
- GoogleTest unit tests are not implemented.
- HAL API documentation is incomplete.
- Exact guest CLI JSON output logs are not checked into the repository.

### Evidence

- Local build: `make -C cpp-hal`
- Observed result: `tools/vnpuctl` build path is valid.

## Milestone 5 - Revision B and Python Validation

### Goal

Demonstrate reusable validation across hardware revisions using pytest, a Python reference model, fault injection, deterministic inputs, and failure artifacts.

### Required Work

- Support Revision B vector length 16.
- Support `JOB_ID`.
- Implement a Python reference model.
- Implement pytest functional, fault, and revision tests.
- Parameterize the same test logic across revisions.
- Export JUnit XML and preserve failure artifacts.

### Completed

- Python test directory exists at `python-tests/`.
- pytest configuration exists at `python-tests/pytest.ini`.
- Shared pytest helper fixture exists at `python-tests/conftest.py`.
- Python reference model exists at `python-tests/reference_model.py`.
- `test_info.py` validates basic `vnpuctl info` JSON fields.
- `test_run_dot.py` runs JSON-file-based dot-product checks against `input*.json` files only.
- `fault.json` exists as a dedicated fault-test input and is no longer included in normal dot-product regression.
- `test_inject_fault.py` exists and covers `irq-drop`, `stuck-busy`, `corrupt-result`, and `force-error` through `vnpuctl`.
- Fault tests reset the device before and after each fault case to reduce stale device-state leakage.
- Guest test runner exists at `scripts/run-test.sh` and is now POSIX `sh` compatible for BusyBox-style guests.

### In Progress

- M5 has started with end-to-end pytest tests through `vnpuctl`.
- Fault tests are implemented in the repository, but need a fresh full guest run after the latest fixes.
- The current pytest path validates the full stack from CLI through HAL, driver, MMIO, IRQ, and QEMU device; failures still need lower-level triage when they occur.

### Not Completed

- Revision B is not implemented in QEMU, driver, HAL, or tests.
- `JOB_ID` behavior is not implemented.
- Randomized seeded differential tests are not implemented.
- INT8 boundary tests are not implemented.
- Device enumeration pytest is not implemented.
- Driver probe/reload pytest is not implemented.
- JUnit XML generation and failure artifact collection are not implemented.
- pytest markers such as `fault`, `revision_a`, `revision_b`, and `slow` are not configured.

### Evidence

- Local syntax/compile checks:
  - `sh -n scripts/run-test.sh scripts/device_reboot.sh`
  - `python3 -m pytest -c python-tests/pytest.ini python-tests --collect-only -q`
- Current pytest collection:
  - `test_info.py::test_info`
  - `test_inject_fault.py::test_inject_irq_drop`
  - `test_inject_fault.py::test_inject_stuck_busy`
  - `test_inject_fault.py::test_inject_corrupt_result`
  - `test_inject_fault.py::test_inject_force_error`
  - `test_run_dot.py::test_run_dot_from_json_file[input0]`
  - `test_run_dot.py::test_run_dot_from_json_file[input1]`
  - `test_run_dot.py::test_run_dot_from_json_file[input2]`
- Local result: `8 tests collected`.
- Full pytest execution requires guest Linux with `/dev/vnpu0` available.

## Milestone 6 - CI, Documentation, and Portfolio Completion

### Goal

Make the project reproducible, reviewable, and suitable for technical interviews.

### Required Work

- Complete fast CI.
- Complete full QEMU integration CI.
- Add caching where useful.
- Complete requirements traceability.
- Document actual debugging cases.
- Complete limitations and architecture documentation.
- Prepare demo script and accurate project summary.

### Completed

- CI skeleton exists at `.github/workflows/ci.yml`.
- Basic patch dry-run and script syntax checks are represented in CI.
- Core documentation exists for versions, register map, UAPI, and HAL design.

### In Progress

- Documentation exists but is stale in several places after implementation progressed.
- `PROJECT_STATUS.md` now tracks M0 through M6.

### Not Completed

- `README.md` is not implemented.
- `docs/architecture.md` is not implemented.
- `docs/driver-design.md` is not implemented.
- `docs/validation-plan.md` is not implemented.
- `docs/requirements-traceability.md` is not implemented.
- `docs/revision-matrix.md` is not implemented.
- `docs/limitations.md` is not implemented.
- `docs/debugging-notes.md` is not implemented.
- ADR for the implemented architecture/layout is not written.
- Full QEMU integration CI is not implemented.
- CI artifact collection is not implemented.

## Repository Hygiene Notes

- The working tree currently contains generated Python cache files under `python-tests/__pycache__/`, `.cache/`, and `compile_commands.json`.
- These generated artifacts should not be treated as source or milestone evidence.
- Some implementation/test files are currently untracked and should be reviewed before committing, including `python-tests/test_inject_fault.py`, `cpp-hal/tests/inputs/fault.json`, and `scripts/device_reboot.sh`.
