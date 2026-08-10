# PROJECT_STATUS

Last updated: 2026-08-07

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
- QEMU/Linux/Buildroot versions are recorded in `docs/versions.md`.
- Buildroot-generated kernel/rootfs artifacts are present under `output/images/`.
- Baseline guest boot, root shell, and baseline `lspci` were re-recorded with the same observed output.
- Serial boot script exists at `scripts/run-qemu.sh`.
- Source-fetch/setup automation exists at `scripts/setup-sources.sh`.
- Reproducible Buildroot defconfig exists at `buildroot/configs/vnpu_qemu_x86_64_defconfig`.
- CI skeleton exists at `.github/workflows/ci.yml`.

### In Progress
- End-to-end setup from a completely clean checkout has not been executed yet.
- CI has been added, but has not run on GitHub Actions yet.

### Evidence
- Boot script: `scripts/run-qemu.sh`
- Kernel image: `output/images/bzImage`
- Root filesystem: `output/images/rootfs.ext4`
- Artifact hashes are current in `docs/versions.md`.
- Re-recorded baseline `lspci`: `00:1f.2 Class 0106: 8086:2922`
- Setup script syntax check: `bash -n scripts/setup-sources.sh`
- CI smoke checks: required-file checks, shell syntax checks, and QEMU patch dry-run

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
- Local QEMU build includes the VNPU device; `-device help` reported `name "vnpu", bus PCI`.
- `scripts/run-qemu.sh` currently launches QEMU with `-device vnpu`.
- PCI IDs are defined in source: vendor `0x1B36`, device `0x1000`.
- BAR0 is implemented in source as a 4 KiB MMIO region.
- Guest `lspci -nnvv` showed the VNPU vendor/device ID.
- QEMU monitor `info pci` showed VNPU BAR0 as a 4 KiB memory region.

### Not Completed
- Automated enumeration test is not implemented.
- Register reset-value test for `DEVICE_ID`, `REVISION`, and `CAPABILITIES` is not implemented.

### Evidence
- `qemu-device/patches/0001-hw-misc-add-vnpu-pci-device.patch` dry-run applied cleanly to pristine QEMU 10.2.0.
- Local QEMU build reported `name "vnpu", bus PCI` in `-device help`.
- Guest `lspci -nnvv` confirmed VNPU vendor/device ID `1b36:1000`.
- QEMU monitor `info pci` reported `BAR0: 32 bit memory at 0xfebfe000 [0xfebfefff]`, confirming a 4 KiB BAR0 window.

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
- Revision A register model is partially implemented in `qemu-device/src/vnpu.c`.
- MMIO input/control/status/result/error registers are implemented in source.
- Signed INT8 dot product for vector length 8 is implemented in source.
- Timer-based asynchronous completion is implemented in source.
- Legacy INTx-style interrupt signaling is implemented in source through `pci_set_irq()`.
- Runtime faults `IRQ_DROP`, `STUCK_BUSY`, `CORRUPT_RESULT`, and `FORCE_ERROR` are implemented in source.
- devmem-based smoke test exists at `qemu-device/tests/scripts/vnpu-devmem-smoke.sh`.
- userspace mmap smoke tool exists at `qemu-device/tests/src/vnpu-mmio-smoke.c`.
- Guest devmem smoke test passed for reset register reads, dot8 result, and `CORRUPT_RESULT` fault behavior.

### In Progress
- M2 runtime validation has started with the devmem smoke test.
- State-machine documentation exists in `docs/register_map.md`, but the document still marks the register map as planned/not implemented.

### Not Completed
- Interrupt-enabled completion test is not implemented.
- Full fault behavior test coverage is not implemented.
- Invalid/unsupported MMIO access robustness coverage is partial only.
- `docs/register_map.md` has not been updated from planned-only status to implemented/verified status.

### Evidence
- Source inspection confirms the implementation exists in `qemu-device/src/vnpu.c`.
- Local validation: `bash -n qemu-device/tests/scripts/vnpu-devmem-smoke.sh`.
- Local validation: `make -C qemu-device/tests clean all`.
- Guest devmem smoke test output:
  - VNPU BDF: `0000:00:03.0`
  - BAR0: `0xfebd5000-0xfebd5fff` (`4096` bytes)
  - `DEVICE_ID`: `0x564e5055`
  - `REVISION`: `0x1`
  - `CAPABILITIES`: `0x0`
  - `STATUS after reset`: `0x0`
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
- None.

### In Progress
- Not started.

### Not Completed
- Driver directory does not exist.
- Kernel module source does not exist.
- UAPI header does not exist.
- Kbuild/Makefile integration does not exist.
- Character device implementation does not exist.
- `ioctl` implementation does not exist.
- IRQ handler, timeout, reset recovery, stats, and driver tests do not exist.

### Entry Criteria
- Record M1 guest enumeration evidence before starting driver probe work.
- Add at least a minimal M2 MMIO smoke test before relying on the device model for driver debugging.
