# CONTEXT.md — Virtual NPU Silicon Validation Platform

> **Purpose of this file**  
> This document is the source of truth for a portfolio project targeting a **Silicon Validation Platform Engineer** role. It is written so that Codex can continue implementation across follow-up sessions without reinterpreting the project scope.
>
> **Current project state:** Planning only. No component should be described as implemented until code and reproducible test evidence exist in the repository.
>
> **Last updated:** 2026-07-27

---

## 0. Instructions for Codex

Codex must follow these rules whenever it works on this repository.

1. Read this file and `PROJECT_STATUS.md` before changing code.
2. Treat this document as the project specification. Do not silently change the architecture, register map, UAPI, milestone order, or MVP scope.
3. Inspect the actual repository before proposing work. Never assume a file, feature, dependency, test, or build result already exists.
4. Work on one milestone or one narrowly scoped issue at a time.
5. Before editing, state:
   - the milestone being addressed;
   - the files expected to change;
   - the test or acceptance condition that will prove completion.
6. After editing, report:
   - changed files;
   - commands executed;
   - actual test results;
   - unresolved failures or limitations;
   - the next recommended task.
7. Never claim a build or test passed unless it was actually executed and its result was observed.
8. Add or update tests with every behavior change. A feature is not complete merely because it compiles.
9. Keep commits small and reviewable. Prefer one logical change per commit.
10. Update `PROJECT_STATUS.md` whenever a milestone item changes state.
11. Record material design changes as Architecture Decision Records under `docs/adr/`.
12. Do not add DMA, MSI/MSI-X, multi-device scheduling, ARM64 support, pybind11, or performance optimization before the MVP completion criteria are met.
13. Do not imply that this project uses actual Rebellions hardware, actual NPU silicon, actual post-silicon validation, or production driver code.
14. Preserve upstream licenses when modifying QEMU or Linux-related code. Add SPDX identifiers appropriate to each component.
15. Prefer deterministic, scriptable, headless execution suitable for CI.
16. When blocked by an unavailable tool or environment, preserve the partial implementation, explain the exact blocker, and provide the smallest reproducible next step. Do not invent output.

---

## 1. Project Motivation

The candidate already has directly demonstrated experience in:

- C/C++ and low-level Assembly analysis;
- RISC-V, ARMv8, and x86 ISA understanding;
- Linux kernel execution-flow and interrupt-path analysis;
- GDB/LLDB-based debugging;
- fuzzer structure analysis and modification;
- fault analysis and differential testing;
- automated validation workflows;
- reproducing low-level abnormal behavior and identifying root causes.

The main gaps this project is intended to address are:

- direct Linux device-driver implementation;
- register-map and MMIO design;
- interrupt-driven device control;
- QEMU virtual-device development;
- C++ hardware abstraction-layer design;
- Python-based validation automation;
- TDD, unit testing, integration testing, and CI;
- hardware-revision abstraction and regression testing.

This project must therefore extend existing low-level analysis and validation strengths into a concrete, end-to-end **virtual hardware → Linux driver → C++ HAL → automated validation** implementation.

---

## 2. One-Sentence Project Definition

Build a reproducible validation platform containing a **QEMU virtual NPU-like PCI device**, an **interrupt-driven Linux device driver**, a **revision-independent C++ HAL**, and a **pytest-based regression framework** with fault injection and differential testing.

---

## 3. Project Goals

The completed project must demonstrate the ability to:

1. Define and document a hardware/software contract through a register map.
2. Implement a virtual accelerator device in QEMU.
3. Enumerate and control the device from a Linux PCI driver.
4. Use MMIO and interrupts rather than a user-space-only simulation.
5. expose a stable userspace UAPI through a character device and `ioctl`.
6. hide hardware-revision differences behind a C++ HAL.
7. validate functional correctness against a software reference model.
8. inject faults and verify timeout, error detection, and recovery behavior.
9. run the same test logic against two simulated hardware revisions.
10. build and test the full stack automatically in CI.
11. document architecture, requirements, debugging cases, limitations, and test evidence.

---

## 4. Explicit Non-Goals

The following are outside the MVP and must not distract from the core deliverable:

- implementing a real neural-network accelerator;
- reproducing Rebellions hardware or a proprietary ISA;
- implementing a full PCIe protocol stack or PHY behavior;
- claiming actual silicon, board, or post-silicon validation experience;
- implementing production-grade security or performance isolation;
- implementing DMA or IOMMU/SMMU in the MVP;
- implementing MSI/MSI-X in the MVP;
- implementing multiple outstanding jobs or queue scheduling in the MVP;
- implementing kernel bypass, VFIO, SR-IOV, or virtualization passthrough;
- implementing ARM64 guest support before the x86_64 reference environment is stable;
- optimizing compute performance;
- adding a GUI;
- building a complete machine-learning framework integration.

---

## 5. Target Deliverables

A complete repository must contain all of the following.

### 5.1 Executable Components

- QEMU virtual NPU device model;
- Linux out-of-tree PCI driver;
- character-device UAPI and `ioctl` definitions;
- C++ HAL library;
- `vnpuctl` command-line tool with JSON output;
- Python reference model;
- pytest functional, fault, and revision-regression suites;
- GoogleTest HAL unit tests;
- optional KUnit tests for isolated kernel logic after the core integration works;
- reproducible build, run, and test scripts;
- CI workflows.

### 5.2 Documentation

- project overview and quick start;
- architecture diagram;
- register specification;
- driver design;
- UAPI specification;
- validation plan;
- requirements-to-test traceability matrix;
- hardware revision matrix;
- debugging notes based on actual encountered failures;
- limitations and non-goals;
- demo instructions;
- license notes for each component.

### 5.3 Evidence

- `lspci` evidence that the device enumerates;
- `dmesg` evidence that the driver probes and removes cleanly;
- functional test logs;
- differential-test results;
- fault-injection and recovery logs;
- revision A/B regression results;
- CI logs and archived serial/kernel logs;
- a short demo video after completion.

---

## 6. High-Level Architecture

```text
┌──────────────────────────────────────────────────────────────┐
│ Python Validation Layer                                      │
│ pytest / reference model / regression matrix / JUnit report  │
└──────────────────────────────┬───────────────────────────────┘
                               │ invokes JSON CLI
┌──────────────────────────────▼───────────────────────────────┐
│ C++ User-Space Layer                                         │
│ libvnpu HAL / revision abstraction / vnpuctl / mock backend  │
└──────────────────────────────┬───────────────────────────────┘
                               │ ioctl on /dev/vnpu0
┌──────────────────────────────▼───────────────────────────────┐
│ Linux Kernel Driver                                          │
│ PCI probe/remove / BAR MMIO / IRQ / timeout / reset/recovery │
└──────────────────────────────┬───────────────────────────────┘
                               │ PCI BAR0 + legacy INTx in MVP
┌──────────────────────────────▼───────────────────────────────┐
│ QEMU Virtual NPU Device                                      │
│ register map / INT8 dot product / revisions / fault injection│
└──────────────────────────────────────────────────────────────┘
```

### 6.1 Architecture Principles

- The register specification is the contract between QEMU and the driver.
- The UAPI is the contract between the driver and user space.
- The HAL is the contract used by application and validation logic.
- Tests above the HAL must not hard-code register offsets.
- Revision-specific behavior must be handled in the device/driver/HAL boundary, not duplicated throughout tests.
- The same functional tests must run against revisions A and B.
- Fault injection must be deterministic and reproducible.
- The MVP supports only one outstanding operation per device.

---

## 7. Reference Development Environment

The initial reference environment is intentionally simple and hardware-independent.

| Area | Baseline |
|---|---|
| Host OS | Ubuntu or a comparable Linux distribution |
| Guest architecture | x86_64 |
| QEMU machine | `q35` |
| Acceleration | QEMU TCG; KVM is optional and not required |
| Guest OS | Linux kernel plus Buildroot-generated root filesystem |
| Virtual device | Custom QEMU PCI endpoint attached within the q35 PCIe topology |
| MVP interrupt | Legacy INTx |
| Driver | Out-of-tree Linux PCI kernel module |
| User-space language | C++20 |
| Build system | QEMU Meson/Ninja, kernel Kbuild, CMake for HAL/CLI |
| Python test | Python 3 + pytest |
| C++ unit test | GoogleTest |
| Debugging | GDB, QEMU GDB stub, `dmesg`, `lspci`, serial logs |
| CI | GitHub Actions |
| Diagrams | Mermaid or PlantUML |

### 7.1 Version Policy

- Exact QEMU, Linux, Buildroot, compiler, CMake, Python, and pytest versions must be pinned in `docs/versions.md` or a lock/config file during Milestone 0.
- Do not silently change pinned versions.
- A version change requires a short ADR if it affects patches, APIs, or reproducibility.
- Source trees should be fetched into a generated directory such as `.deps/` or `third_party/src/` and must not be committed in full.
- QEMU changes should preferably be stored as a small patch series that can be applied to the pinned upstream revision.

---

## 8. Virtual NPU Functional Specification

### 8.1 Compute Operation

The device performs a small signed INT8 dot product:

```text
result = Σ (input_a[i] × input_b[i]), for i = 0 ... vector_length - 1
```

- Input elements are signed 8-bit integers.
- Output is a signed 32-bit integer.
- Revision A supports `vector_length = 16` only.
- Revision B supports `vector_length = 8` or `16`.
- Input data is transferred through MMIO registers in the MVP.
- The operation is intentionally small. The project evaluates the software stack and validation design, not accelerator performance.

### 8.2 PCI Identity

The implementation must define a local educational PCI vendor/device pair.

- The selected identifiers must be checked against the pinned QEMU tree for conflicts.
- The identifiers must be documented in `docs/register-map.md`.
- The repository must state that the IDs are for a local virtual device and are not an official vendor allocation.

Recommended starting values, subject to conflict verification:

```text
Vendor ID: 0x1B36   # QEMU virtual-device vendor space
Device ID: 0x1000   # local project value; verify before use
Class:     processing accelerator or other documented experimental choice
```

### 8.3 BAR Layout

- BAR0 size: 4 KiB.
- Register width: 32 bits unless explicitly stated.
- Endianness: little-endian.
- Unimplemented offsets return zero unless the register specification states otherwise.
- Invalid writes must not crash QEMU.

### 8.4 Register Map

| Offset | Register | Access | Reset value | Purpose |
|---:|---|---|---:|---|
| `0x000` | `DEVICE_ID` | RO | `0x564E5055` | ASCII-like `VNPU` device signature |
| `0x004` | `REVISION` | RO | revision-dependent | `1` for A, `2` for B |
| `0x008` | `CAPABILITIES` | RO | revision-dependent | Feature bitmap |
| `0x00C` | `CONTROL` | WO/W1S | `0` | Start or reset command |
| `0x010` | `STATUS` | RO | `IDLE` | Device state |
| `0x014` | `IRQ_STATUS` | RW1C | `0` | Pending completion/error IRQs |
| `0x018` | `IRQ_ENABLE` | RW | `0` | IRQ mask |
| `0x01C` | `ERROR_CODE` | RO | `0` | Last device error |
| `0x020` | `JOB_ID` | RW | `0` | Revision B job identifier |
| `0x024` | `VECTOR_LENGTH` | RW | `16` | Revision-dependent vector length |
| `0x100`–`0x10C` | `INPUT_A[0..3]` | RW | `0` | 16 packed INT8 values |
| `0x120`–`0x12C` | `INPUT_B[0..3]` | RW | `0` | 16 packed INT8 values |
| `0x140` | `RESULT` | RO | `0` | Signed INT32 dot-product result |
| `0x180` | `FAULT_CONTROL` | RW | `0` | Runtime fault-injection bitmap |

### 8.5 Register Bit Definitions

#### `CAPABILITIES`

| Bit | Name | Meaning |
|---:|---|---|
| 0 | `CAP_DOT_INT8` | INT8 dot product is supported |
| 1 | `CAP_JOB_ID` | `JOB_ID` is supported |
| 2 | `CAP_VARIABLE_LENGTH` | Vector length 8 or 16 is supported |
| 3 | `CAP_SEPARATE_ERROR_IRQ` | Error IRQ is distinguishable from completion IRQ |
| 4 | `CAP_FAULT_INJECTION` | Test-only runtime fault injection is supported |

Recommended capability sets:

```text
Revision A: CAP_DOT_INT8 | CAP_FAULT_INJECTION
Revision B: CAP_DOT_INT8 | CAP_JOB_ID | CAP_VARIABLE_LENGTH |
            CAP_SEPARATE_ERROR_IRQ | CAP_FAULT_INJECTION
```

#### `CONTROL`

| Bit | Name | Meaning |
|---:|---|---|
| 0 | `CONTROL_START` | Start one operation; write-one action |
| 1 | `CONTROL_RESET` | Reset device state; write-one action |

#### `STATUS`

| Bit | Name | Meaning |
|---:|---|---|
| 0 | `STATUS_IDLE` | Ready for a new command |
| 1 | `STATUS_BUSY` | Operation is in progress |
| 2 | `STATUS_DONE` | Last operation completed successfully |
| 3 | `STATUS_ERROR` | Last operation failed |

#### `IRQ_STATUS` and `IRQ_ENABLE`

| Bit | Name | Meaning |
|---:|---|---|
| 0 | `IRQ_COMPLETION` | Operation completed |
| 1 | `IRQ_ERROR` | Device error occurred |

`IRQ_STATUS` uses write-one-to-clear semantics.

#### `FAULT_CONTROL`

| Bit | Name | Meaning |
|---:|---|---|
| 0 | `FAULT_IRQ_DROP` | Complete operation without raising an IRQ |
| 1 | `FAULT_STUCK_BUSY` | Leave the device in BUSY state until reset |
| 2 | `FAULT_CORRUPT_RESULT` | Deliberately alter the computed result |
| 3 | `FAULT_FORCE_ERROR` | Finish with a device error instead of success |

Boot-time faults such as an invalid device signature or unsupported revision must be QEMU device properties, not runtime register bits, because they affect driver probe behavior.

Recommended QEMU properties:

```text
revision=a|b|unknown
bad-device-id=on|off
operation-delay-us=<integer>
```

### 8.6 Error Codes

| Value | Name | Meaning |
|---:|---|---|
| 0 | `VNPU_ERR_NONE` | No error |
| 1 | `VNPU_ERR_INVALID_LENGTH` | Unsupported vector length |
| 2 | `VNPU_ERR_BUSY` | Start requested while already busy |
| 3 | `VNPU_ERR_FORCED` | Fault-injection forced error |
| 4 | `VNPU_ERR_UNSUPPORTED_REVISION` | Unsupported revision |
| 5 | `VNPU_ERR_INTERNAL` | Internal model error |

### 8.7 State Machine

```text
RESET
  └─> IDLE
        ├─ START with valid parameters
        │    └─> BUSY
        │          ├─ normal completion ─> DONE ─> IDLE on next START/RESET
        │          ├─ forced error ──────> ERROR ─> IDLE on next START/RESET
        │          └─ STUCK_BUSY ────────> BUSY until RESET
        ├─ START while BUSY
        │    └─ retain BUSY and expose VNPU_ERR_BUSY
        └─ RESET from any state
             └─> IDLE, IRQ cleared, error cleared
```

Behavioral rules:

- `START` clears prior `DONE`, `ERROR`, IRQ status, and error code before beginning a valid operation.
- `RESET` clears runtime fault state only if the specification explicitly chooses that behavior; document the decision.
- The QEMU device must model completion asynchronously using a QEMU timer or equivalent non-blocking mechanism.
- Default operation latency should be deterministic and configurable.
- Revision A may expose all errors through the completion line if it does not advertise separate error IRQ capability.
- Revision B must distinguish completion and error IRQ status.

### 8.8 Revision Matrix

| Feature | Revision A | Revision B |
|---|---|---|
| INT8 dot product | Yes | Yes |
| Vector length | Fixed 16 | 8 or 16 |
| Job ID | No; reads zero, writes ignored | Yes |
| Completion IRQ | Yes | Yes |
| Separate error IRQ status | No | Yes |
| Fault injection | Yes | Yes, with clearer error reporting |
| HAL-visible API | Same | Same |

The HAL must use `CAPABILITIES`, not a scattered set of revision-number checks, wherever possible.

---

## 9. Linux Driver Specification

### 9.1 Driver Identity

Recommended names:

```text
Kernel module: vnpu_drv
Device node:   /dev/vnpu0
Class name:    vnpu
UAPI header:   include/uapi/linux/vnpu.h or project-local equivalent
```

### 9.2 Required Driver Functions

The MVP driver must implement:

1. PCI ID table registration.
2. `probe()` and `remove()`.
3. `pci_enable_device()`.
4. `pci_request_regions()`.
5. BAR0 mapping through `pci_iomap()` or an equivalent managed API.
6. IRQ request and release.
7. character-device registration.
8. stable `ioctl` UAPI.
9. one-operation-at-a-time command submission.
10. interrupt-driven completion.
11. bounded timeout handling.
12. reset and recovery after timeout/error.
13. consistent cleanup on all failure paths.
14. module load/unload repeatability.
15. useful `dev_dbg`, `dev_info`, and `dev_err` logging without log spam.

### 9.3 Concurrency Model

The MVP supports one outstanding job per device.

Recommended synchronization:

- a mutex protects command submission and user-facing device state;
- a completion object or wait queue handles IRQ-driven completion;
- IRQ-visible fields use appropriate atomicity or locking;
- process context performs reset and recovery;
- the IRQ handler must remain short and must not sleep.

Codex must document why each synchronization primitive is used.

### 9.4 UAPI Requirements

The UAPI must use fixed-width types and include an ABI version.

Recommended commands:

```text
VNPU_IOCTL_GET_INFO
VNPU_IOCTL_RUN_DOT
VNPU_IOCTL_RESET
VNPU_IOCTL_SET_FAULT
VNPU_IOCTL_GET_STATS
```

Recommended structures:

```c
struct vnpu_info {
    __u32 abi_version;
    __u32 device_id;
    __u32 revision;
    __u32 capabilities;
};

struct vnpu_dot_request {
    __u32 abi_version;
    __u32 job_id;
    __u32 vector_length;
    __u32 timeout_ms;
    __s8  input_a[16];
    __s8  input_b[16];
    __s32 result;
    __s32 driver_status;
    __u32 device_error;
};

struct vnpu_fault_request {
    __u32 abi_version;
    __u32 fault_mask;
};

struct vnpu_stats {
    __u32 abi_version;
    __u64 submitted;
    __u64 completed;
    __u64 timed_out;
    __u64 device_errors;
    __u64 resets;
};
```

The exact ABI may be refined before implementation, but any change after code exists requires:

- UAPI documentation update;
- ABI version reasoning;
- tests;
- an ADR if compatibility is affected.

### 9.5 Driver Validation and Safety Rules

The driver must:

- validate `abi_version`;
- validate vector length before MMIO access;
- reject unsupported features cleanly;
- use `copy_from_user()` and `copy_to_user()` correctly;
- avoid direct use of userspace pointers;
- check arithmetic and array bounds;
- clear stale completion state before submission;
- acknowledge IRQ status correctly;
- avoid infinite waits;
- recover or return a clear error after timeout;
- unwind all acquired resources in reverse order;
- refuse unsupported device signature or revision with a clear log;
- remain unloadable after failed operations.

### 9.6 Timeout and Recovery Behavior

Recommended sequence:

1. validate request;
2. lock submission path;
3. clear stale IRQ/completion state;
4. program input, vector length, job ID if supported, and IRQ mask;
5. issue `START`;
6. wait for completion with a bounded timeout;
7. on success, read result and status;
8. on device error, collect `ERROR_CODE` and return a mapped error;
9. on timeout, increment stats, issue device reset, verify return to IDLE, and return `-ETIMEDOUT`;
10. unlock and return.

### 9.7 Driver Tests

At minimum, prove:

- successful probe and remove;
- valid device info;
- correct operation result;
- signed-input correctness;
- invalid vector length rejection;
- start-while-busy behavior;
- IRQ completion;
- IRQ-drop timeout;
- stuck-busy reset recovery;
- error interrupt handling;
- unsupported revision rejection;
- bad device signature rejection;
- repeated module load/unload without leaked resources or kernel warnings.

Target for the documented stress check:

```text
At least 50 module load/unload cycles with no kernel warning or stale device node.
```

This is an acceptance target, not a result to claim before execution.

---

## 10. C++ HAL and CLI Specification

### 10.1 HAL Goals

The HAL must:

- hide raw `ioctl` and file-descriptor handling;
- provide RAII resource management;
- expose revision-independent operations;
- query and use capabilities;
- translate driver/device failures into structured C++ errors;
- support unit testing without QEMU through a mock backend;
- avoid exposing register offsets to callers.

### 10.2 Recommended Interface

```cpp
class IVnpuDevice {
public:
    virtual DeviceInfo get_info() const = 0;

    virtual DotProductResult run_dot_product(
        std::span<const std::int8_t> lhs,
        std::span<const std::int8_t> rhs,
        std::chrono::milliseconds timeout) = 0;

    virtual void reset() = 0;
    virtual void inject_fault(FaultType type) = 0;
    virtual DeviceStats get_stats() const = 0;

    virtual ~IVnpuDevice() = default;
};
```

Required implementations:

```text
LinuxVnpuDevice   # communicates with /dev/vnpu0
MockVnpuDevice    # deterministic unit-test backend
```

### 10.3 HAL Behavior

- Detect capabilities once during initialization and cache immutable information.
- Validate vector lengths before calling the driver.
- Keep the public API identical for revision A and B.
- Handle missing `JOB_ID` capability internally.
- Map Linux `errno`, driver status, and device error into distinguishable error categories.
- Produce no hidden retries except where explicitly documented.
- Include Doxygen-compatible API comments.

### 10.4 CLI

Executable name:

```text
vnpuctl
```

Required commands:

```bash
vnpuctl info --json
vnpuctl run-dot --input-a <file-or-list> --input-b <file-or-list> --timeout-ms 100 --json
vnpuctl inject-fault irq-drop
vnpuctl inject-fault stuck-busy
vnpuctl inject-fault corrupt-result
vnpuctl inject-fault force-error
vnpuctl clear-faults
vnpuctl reset
vnpuctl stats --json
```

JSON output must be stable enough for pytest to parse. Human-readable output may also be supported, but automated tests must use JSON.

### 10.5 HAL Unit Tests

GoogleTest must cover at least:

- capability parsing;
- revision A fixed-length behavior;
- revision B variable-length behavior;
- unsupported length rejection;
- driver-error mapping;
- device-error mapping;
- timeout mapping;
- mock-backend success and failure paths;
- JSON serialization helpers if they contain logic.

---

## 11. Python Validation Framework

### 11.1 Validation Strategy

Python is used for orchestration and validation, not as a replacement for the C++ HAL.

The normal path is:

```text
pytest → vnpuctl JSON CLI → C++ HAL → ioctl → Linux driver → QEMU device
```

### 11.2 Reference Model

Implement a deterministic software reference:

```python
def dot_reference(a: list[int], b: list[int]) -> int:
    return sum(int(x) * int(y) for x, y in zip(a, b))
```

The reference must validate:

- equal lengths;
- supported length;
- signed INT8 range;
- deterministic input generation using an explicit random seed.

### 11.3 Required pytest Cases

| Test | Purpose |
|---|---|
| `test_device_enumeration` | QEMU exposes the expected PCI device |
| `test_driver_probe` | Driver binds and creates `/dev/vnpu0` |
| `test_device_info` | Device ID, revision, and capabilities are correct |
| `test_dot_product_basic` | Basic successful operation |
| `test_dot_product_negative_values` | Signed INT8 handling |
| `test_dot_product_boundaries` | INT8 min/max and result range |
| `test_dot_product_randomized` | Seeded randomized functional coverage |
| `test_result_differential` | Hardware-model output equals Python reference |
| `test_irq_completion` | Interrupt-driven completion is observed |
| `test_irq_drop_timeout` | Missing IRQ produces bounded timeout |
| `test_stuck_busy_recovery` | Timeout recovery resets device to IDLE |
| `test_corrupt_result_detection` | Differential test catches wrong result |
| `test_forced_error` | Device error is propagated correctly |
| `test_unknown_revision` | Unsupported revision fails clearly |
| `test_bad_device_id` | Bad signature prevents normal probe |
| `test_module_reload` | Repeated driver reload is stable |
| `test_revision_regression` | Same suite runs against revisions A and B |

### 11.4 Failure Artifacts

On failure, the test runner must preserve as much of the following as practical:

- test seed;
- test parameters;
- QEMU command line;
- QEMU serial log;
- guest `dmesg`;
- `lspci -nnvv` output;
- CLI JSON result;
- register/status snapshot if the driver exposes it safely;
- JUnit XML.

### 11.5 Test Markers

Recommended pytest markers:

```text
unit
integration
fault
revision_a
revision_b
slow
```

Fast and slow suites must be separable.

---

## 12. Test-Driven Development Expectations

The project should show meaningful test-first development for several core behaviors.

Recommended commit pattern:

```text
test: add failing capability parser test
feat: implement capability parser

test: add IRQ timeout integration case
feat: add driver timeout and reset recovery

test: add revision B variable-length regression
feat: implement revision B vector-length capability
```

Not every setup commit needs to be test-first, but at least the following features should have visible test-before-fix history:

- capability parsing;
- IRQ timeout handling;
- reset recovery;
- revision B behavior;
- corrupt-result detection.

---

## 13. Repository Structure

```text
virtual-npu-validation-platform/
├── CONTEXT.md
├── PROJECT_STATUS.md
├── README.md
├── LICENSES/
├── Makefile
├── docs/
│   ├── architecture.md
│   ├── versions.md
│   ├── register-map.md
│   ├── revision-matrix.md
│   ├── driver-design.md
│   ├── uapi.md
│   ├── validation-plan.md
│   ├── requirements-traceability.md
│   ├── debugging-notes.md
│   ├── limitations.md
│   └── adr/
│       └── 0001-record-architecture-decisions.md
├── qemu-device/
│   ├── patches/
│   ├── include/
│   └── tests/
├── linux-driver/
│   ├── vnpu_drv.c
│   ├── vnpu_internal.h
│   ├── vnpu_uapi.h
│   ├── vnpu_kunit.c
│   └── Makefile
├── cpp-hal/
│   ├── include/vnpu/
│   ├── src/
│   ├── tests/
│   └── CMakeLists.txt
├── tools/
│   └── vnpuctl/
├── python-tests/
│   ├── conftest.py
│   ├── reference_model.py
│   ├── test_functional.py
│   ├── test_faults.py
│   ├── test_revisions.py
│   └── pytest.ini
├── guest/
│   ├── buildroot-external/
│   ├── kernel-config/
│   └── overlay/
├── scripts/
│   ├── bootstrap.sh
│   ├── fetch-sources.sh
│   ├── build-qemu.sh
│   ├── build-guest.sh
│   ├── build-all.sh
│   ├── run-qemu.sh
│   ├── run-tests.sh
│   ├── collect-logs.sh
│   └── clean.sh
├── ci/
│   └── helpers/
└── .github/
    └── workflows/
        ├── fast-ci.yml
        └── integration-ci.yml
```

Generated source trees and build outputs must be ignored, for example:

```text
.deps/
out/
build/
artifacts/
```

---

## 14. Required Developer Experience

The repository should converge on the following command interface.

```bash
# Check host dependencies and explain missing packages.
./scripts/bootstrap.sh

# Fetch pinned QEMU, Linux, and Buildroot sources.
./scripts/fetch-sources.sh

# Build all host and guest components.
./scripts/build-all.sh

# Run revision A interactively or headlessly.
./scripts/run-qemu.sh --revision a

# Run revision B.
./scripts/run-qemu.sh --revision b

# Run all automated tests.
./scripts/run-tests.sh --revision all

# Run only fast host-side tests.
./scripts/run-tests.sh --suite fast

# Collect reproducibility artifacts.
./scripts/collect-logs.sh
```

Requirements:

- scripts must use `set -euo pipefail` where appropriate;
- scripts must fail with actionable messages;
- commands must be documented in `README.md`;
- the project must work without KVM;
- no script should require undocumented manual edits inside fetched upstream trees;
- patches must be applied automatically and idempotently;
- a clean checkout should be sufficient to reproduce the environment once dependencies are installed.

---

## 15. CI Design

### 15.1 Fast CI

Run on every pull request:

- formatting checks;
- static analysis where practical;
- driver compilation against the pinned kernel headers/tree;
- C++ HAL and CLI build;
- GoogleTest;
- Python lint and unit tests;
- documentation link checks;
- patch-application check.

### 15.2 Integration CI

Run on main-branch pushes, scheduled builds, or manual dispatch:

1. fetch pinned sources;
2. build patched QEMU;
3. build guest kernel and root filesystem;
4. boot QEMU headlessly with TCG;
5. load the driver;
6. run revision A regression;
7. run revision B regression;
8. run fault-injection tests;
9. export JUnit XML;
10. archive serial log, `dmesg`, `lspci`, CLI JSON, and test reports.

Use caching for QEMU, kernel, Buildroot downloads, and compiler outputs where valid. Cache keys must include pinned revisions and relevant configuration hashes.

---

## 16. Documentation Requirements

### 16.1 `README.md`

Recommended order:

1. problem statement;
2. one-paragraph solution;
3. architecture diagram;
4. key features;
5. quick start;
6. current implementation status;
7. sample output based on real execution only;
8. validation strategy;
9. repository layout;
10. limitations;
11. license notes.

### 16.2 `docs/debugging-notes.md`

Every substantial real debugging case should use this structure:

```text
Symptom
→ Reproduction conditions
→ Evidence and logs
→ Initial hypotheses
→ Investigation steps
→ Root cause
→ Fix
→ Regression test added
→ Remaining limitation
```

Good candidate cases include:

- incorrect IRQ acknowledgement order;
- RW1C implementation error;
- stale completion state;
- resource cleanup on failed probe;
- timeout recovery leaving BUSY set;
- revision capability parsing bug;
- MMIO packing/sign-extension bug;
- module unload race.

Do not pre-fill a debugging story before the issue actually occurs.

### 16.3 `docs/limitations.md`

This file must explicitly state:

- the device is a QEMU model, not real NPU silicon;
- the project does not validate PCIe PHY, link training, timing, signal integrity, or board behavior;
- MMIO input is used instead of production DMA in the MVP;
- post-silicon behavior is not reproduced;
- legacy INTx is used in the MVP;
- performance results are not representative of hardware;
- fault injection is software-controlled and deterministic.

---

## 17. Requirements-to-Test Traceability

Maintain a table similar to the following in `docs/requirements-traceability.md`.

| Requirement ID | Requirement | Verification |
|---|---|---|
| `REQ-ENV-001` | Clean checkout can fetch and build pinned sources | CI clean-build job |
| `REQ-DEV-001` | Device enumerates with documented PCI identity | `test_device_enumeration` |
| `REQ-DEV-002` | Register reset values match specification | register-level test |
| `REQ-CMD-001` | Valid dot-product command returns correct result | `test_result_differential` |
| `REQ-CMD-002` | Signed INT8 boundaries are handled correctly | `test_dot_product_boundaries` |
| `REQ-IRQ-001` | Normal completion raises an enabled IRQ | `test_irq_completion` |
| `REQ-ERR-001` | IRQ drop produces bounded timeout | `test_irq_drop_timeout` |
| `REQ-REC-001` | Timeout recovery returns device to IDLE | `test_stuck_busy_recovery` |
| `REQ-REV-001` | Revision A supports length 16 only | revision A test |
| `REQ-REV-002` | Revision B supports lengths 8 and 16 | revision B test |
| `REQ-REV-003` | One HAL API works for both revisions | parameterized HAL/integration test |
| `REQ-UAPI-001` | Invalid ABI version is rejected | UAPI validation test |
| `REQ-DRV-001` | Failed probe releases all acquired resources | probe-failure test/log inspection |
| `REQ-DRV-002` | Module can be loaded/unloaded repeatedly | `test_module_reload` |
| `REQ-DOC-001` | Public HAL API is documented | Doxygen build check |
| `REQ-CI-001` | Integration CI archives failure evidence | workflow artifact check |

No requirement should be marked verified without a corresponding executable test or documented inspection procedure.

---

## 18. Six-Week Milestone Plan

Assumption: approximately 15–20 focused hours per week.

### Milestone 0 — Repository and Reproducible Environment

**Goal:** Establish the project skeleton and a clean, pinned build environment.

Tasks:

- create repository structure;
- create `PROJECT_STATUS.md`;
- define pinned versions;
- implement source-fetch scripts;
- build unmodified QEMU;
- boot a baseline Buildroot Linux guest;
- establish serial logging;
- create initial architecture and register-map documents;
- create CI skeleton.

Acceptance criteria:

- a clean checkout can fetch required sources;
- QEMU guest boots with a documented command;
- `lspci` runs inside the guest;
- generated outputs are outside version control;
- no virtual NPU implementation is falsely marked complete.

### Milestone 1 — Minimal PCI Enumeration

**Goal:** Make a minimal custom virtual device appear in the guest.

Tasks:

- add QEMU device skeleton;
- define local PCI ID;
- add BAR0;
- add fixed `DEVICE_ID`, `REVISION`, and `CAPABILITIES` registers;
- create a basic enumeration test.

Acceptance criteria:

- `lspci -nn` shows the custom device;
- BAR0 size is 4 KiB;
- register reads return documented reset values;
- test automation can detect enumeration failure.

### Milestone 2 — Functional Device Model

**Goal:** Implement revision A device behavior, MMIO compute, asynchronous completion, IRQ, and basic faults.

Tasks:

- implement input, control, status, result, IRQ, and fault registers;
- implement signed INT8 dot product;
- implement asynchronous timer-based completion;
- implement legacy INTx;
- implement revision A;
- implement `IRQ_DROP`, `STUCK_BUSY`, `CORRUPT_RESULT`, and `FORCE_ERROR`;
- document state machine.

Acceptance criteria:

- MMIO command produces correct result;
- completion interrupt fires when enabled;
- each runtime fault is deterministic;
- invalid accesses do not crash QEMU;
- state transitions match the specification.

### Milestone 3 — Linux PCI Driver

**Goal:** Control the device through a real kernel driver and character-device UAPI.

Tasks:

- implement probe/remove and BAR mapping;
- implement IRQ handler;
- implement character device;
- implement `ioctl` ABI;
- implement timeout and reset recovery;
- implement stats;
- add failure-path cleanup;
- add driver tests and logging.

Acceptance criteria:

- driver probes and creates `/dev/vnpu0`;
- userspace can submit one operation;
- completion is interrupt-driven;
- IRQ-drop produces timeout;
- stuck-busy is recovered by reset;
- unsupported device/revision probe fails clearly;
- repeated load/unload is stable.

### Milestone 4 — C++ HAL and CLI

**Goal:** Provide a clean, revision-independent userspace interface.

Tasks:

- implement `IVnpuDevice`;
- implement Linux and mock backends;
- implement error model;
- implement `vnpuctl` JSON CLI;
- add GoogleTest;
- add API documentation.

Acceptance criteria:

- application code contains no register offsets;
- HAL unit tests run without QEMU;
- CLI supports info, run, reset, fault, and stats;
- JSON output is stable and documented;
- errors distinguish validation, driver, timeout, and device failures.

### Milestone 5 — Revision B and Python Validation

**Goal:** Demonstrate reusable validation across hardware revisions.

Tasks:

- implement revision B capabilities;
- support vector length 8/16;
- support job ID;
- support separate error IRQ status;
- implement Python reference model;
- implement pytest functional, fault, and revision tests;
- parameterize the same test logic across revisions;
- export JUnit XML and failure artifacts.

Acceptance criteria:

- one test suite runs against A and B;
- revision differences do not leak into high-level tests unnecessarily;
- randomized differential tests are reproducible by seed;
- corrupt results are detected;
- failure evidence is preserved.

### Milestone 6 — CI, Documentation, and Portfolio Completion

**Goal:** Make the project reproducible, reviewable, and suitable for technical interviews.

Tasks:

- complete fast CI;
- complete full QEMU integration CI;
- add caching;
- complete requirements traceability;
- document actual debugging cases;
- complete limitations;
- prepare architecture diagrams;
- produce demo script and video;
- write resume-ready but accurate project summary.

Acceptance criteria:

- clean CI builds the complete stack;
- CI runs both revisions and fault tests;
- all required artifacts are archived;
- documentation matches implementation;
- README quick start works;
- project claims remain clearly limited to a virtual platform.

---

## 19. MVP Completion Criteria

The project may be called complete only when all mandatory conditions below are satisfied.

### Virtual Device

- custom QEMU device enumerates;
- BAR0 register map matches documentation;
- revision A and B are supported;
- signed INT8 dot product works;
- asynchronous completion works;
- interrupt generation works;
- at least four deterministic fault modes work.

### Driver

- probe/remove works;
- BAR mapping works;
- character device exists;
- UAPI is versioned;
- valid operation completes by IRQ;
- timeout is bounded;
- reset recovery works;
- unsupported device/revision behavior is tested;
- resource cleanup is verified;
- repeated module reload is tested.

### HAL and CLI

- C++ HAL hides raw UAPI details;
- Linux and mock backends exist;
- revision differences are capability-driven;
- unit tests pass;
- JSON CLI is usable by pytest;
- API documentation is generated or build-checked.

### Validation

- Python reference model exists;
- deterministic randomized differential testing exists;
- functional tests exist;
- fault tests exist;
- revision regression exists;
- JUnit output exists;
- failure logs are collected.

### Reproducibility

- pinned versions are documented;
- build/run/test scripts work from a clean checkout;
- fast CI passes;
- full integration CI runs under TCG;
- documentation reflects actual behavior;
- no generated binaries are committed.

---

## 20. Stretch Goals — Do Not Start Before MVP

After MVP completion, choose at most one or two of the following based on remaining time:

1. Simple DMA path using a coherent buffer.
2. MSI or MSI-X interrupt support.
3. Two virtual devices and a basic resource allocator.
4. ARM64 guest port.
5. KUnit coverage for more driver logic.
6. ioctl fuzz testing.
7. pybind11 binding for direct Python HAL access.
8. code coverage reporting.
9. qtest-based register-model tests.
10. concurrent multi-process rejection or serialization test.

Any stretch goal must have its own requirements, tests, and documentation. It must not weaken the completed MVP.

---

## 21. Risks and Fallbacks

| Risk | Required response |
|---|---|
| QEMU custom device work takes longer than expected | First complete enumeration, then BAR reads, then state machine, then IRQ. Do not implement all features at once. |
| IRQ implementation is unstable | Use polling only as a temporary diagnostic step. The final MVP must use IRQ-driven completion. |
| Buildroot integration blocks progress | Temporarily use a minimal prebuilt guest only for diagnosis, then return to a pinned reproducible guest build. |
| C++/Python integration expands scope | Keep Python calling the JSON CLI. pybind11 remains a stretch goal. |
| CI build time is excessive | Split fast and integration workflows and cache pinned source/build artifacts. |
| DMA consumes schedule | Do not implement DMA before MVP completion. |
| Revision behavior looks artificial | Keep differences realistic: capability discovery, job IDs, length support, and error reporting. Document rationale. |
| Driver error path is hard to test | Add QEMU boot-time properties and runtime faults that intentionally trigger probe and execution failures. |
| Project is mistaken for real silicon work | Repeat the virtual-platform limitation in README, documentation, CV wording, and demo. |

---

## 22. Coding and Review Conventions

### 22.1 General

- Use clear, descriptive names.
- Avoid unexplained magic constants.
- Keep shared register and UAPI definitions synchronized through documented generation or disciplined duplication checks.
- Treat compiler warnings as errors where practical.
- Add comments for hardware semantics, synchronization, and non-obvious error handling, not for trivial code.
- Keep public APIs small.

### 22.2 C and Kernel Code

- Follow Linux kernel coding style for the driver.
- Use kernel fixed-width/UAPI types where appropriate.
- Avoid custom linked lists or synchronization primitives when kernel facilities exist.
- Check every allocation and resource-acquisition result.
- Use managed PCI APIs only when their cleanup semantics are understood and documented.
- Do not sleep in IRQ context.
- Keep UAPI structures layout-stable and architecture-independent.

### 22.3 QEMU Code

- Follow QEMU style and object model conventions.
- Keep device state explicit.
- Use QEMU timer/BH facilities for asynchronous behavior.
- Ensure reset behavior is complete and deterministic.
- Preserve QEMU licensing headers and style checks.
- Do not block the QEMU main loop.

### 22.4 C++

- Use C++20.
- Prefer RAII and value types.
- Avoid global mutable state.
- Use `std::span` for non-owning buffers where appropriate.
- Use explicit error categories or a documented exception hierarchy.
- Keep Linux-specific details in the Linux backend.

### 22.5 Python

- Use type hints.
- Make randomized tests reproducible.
- Avoid hiding subprocess failures.
- Capture stdout, stderr, exit code, and JSON parse errors clearly.
- Keep reference-model code simple and independent from implementation logic.

---

## 23. `PROJECT_STATUS.md` Template

Codex should create and maintain a separate status file using this shape:

````markdown
# PROJECT_STATUS.md

## Current milestone
M0 — Repository and Reproducible Environment

## Overall state
- Planning: active
- Implementation: not started
- MVP complete: no

## Completed
- [ ] Repository skeleton
- [ ] Pinned versions
- [ ] Baseline guest boot

## In progress
- [ ] ...

## Blocked
- None

## Last verified commands
```bash
# Add only commands that were actually run.
```

## Last verified results
- Add only observed results.

## Next task
- One narrowly scoped task.

## Known limitations
- QEMU-only virtual platform.
````

Status rules:

- use `[x]` only after verification;
- include exact commands that were actually executed;
- do not copy expected output into verified-results sections;
- keep the next task small enough for one Codex iteration.

---

## 24. Accurate Portfolio and Resume Positioning

### 24.1 Allowed After Actual Completion

A suitable Korean project line is:

> QEMU 기반 가상 NPU PCI 디바이스와 Linux 드라이버를 구현하고, 하드웨어 Revision 차이를 추상화한 C++ HAL 및 pytest 기반 Fault-injection Regression Framework를 구축. MMIO, Interrupt, Timeout Recovery, Differential Testing 및 CI를 통해 Revision A/B의 기능과 오류 처리 동작을 자동 검증.

A suitable English project line is:

> Developed a QEMU-based virtual NPU PCI device and Linux driver, and built a reusable C++ HAL and pytest-based validation framework. Automated MMIO, interrupt, timeout recovery, differential testing, and hardware-revision regression in CI.

### 24.2 Claims That Must Not Be Made

Do not describe this project as:

- actual NPU silicon validation;
- post-silicon validation;
- actual PCIe board bring-up;
- production NPU driver development;
- Rebellions hardware development;
- real JTAG debugging;
- physical hardware fault analysis;
- commercial device-driver experience;
- DMA/IOMMU experience unless those features are actually implemented and tested;
- MSI/MSI-X experience unless implemented and tested.

Use precise alternatives:

| Accurate wording | Inaccurate wording |
|---|---|
| QEMU virtual-device validation | Real NPU silicon validation |
| Virtual PCI device and Linux driver project | Production PCIe/NPU driver |
| Simulated hardware revisions | Post-silicon revision validation |
| Software-controlled fault injection | Silicon fault analysis |
| QEMU/GDB debugging | JTAG board debugging |
| Project-based driver implementation | Professional driver employment experience |

---

## 25. Recommended First Codex Task

The first Codex session should implement **Milestone 0 only**.

Suggested prompt:

```text
Read CONTEXT.md in full and inspect the repository. Start Milestone 0 only.

Create the initial repository skeleton, PROJECT_STATUS.md, docs/versions.md,
docs/architecture.md, docs/register-map.md, docs/adr/0001-record-architecture-decisions.md,
and the source-fetch/build script skeletons.

Do not implement the QEMU virtual device or Linux driver yet.
Pin candidate upstream versions only after verifying that they can be fetched and built
in the current environment. Keep generated source trees outside Git.

At the end, report:
1. files changed,
2. commands actually executed,
3. observed results,
4. unresolved dependencies,
5. the single next task for Milestone 0.
```

The first technical success criterion after environment setup is:

```text
clean checkout → fetch pinned sources → build baseline QEMU/guest → boot Linux → run lspci
```

Only after that baseline is reproducible should Codex begin the minimal custom PCI-device enumeration milestone.

---

## 26. Final Scope Reminder

This project is successful when it demonstrates disciplined **system-software validation engineering**, not when it accumulates the most features.

The priority order is:

```text
Reproducibility
→ documented hardware/software contract
→ correct virtual device behavior
→ safe Linux driver
→ revision-independent HAL
→ automated validation
→ fault detection and recovery
→ CI and evidence
→ optional extensions
```

When schedule pressure occurs, preserve the complete vertical slice and remove optional breadth.
