# CONTEXT_KO.md — Virtual NPU Silicon Validation Platform

> **이 파일의 목적**  
> 이 문서는 **Silicon Validation Platform Engineer** 직무 지원을 위한 포트폴리오 프로젝트의 한국어 기준 명세이다. Codex가 후속 세션에서도 프로젝트 범위를 다시 해석하지 않고 구현을 이어갈 수 있도록 작성되었다.
>
> **프로젝트 현재 상태:** 계획 단계. 저장소에 코드와 재현 가능한 테스트 근거가 존재하기 전에는 어떤 구성요소도 구현 완료로 서술하면 안 된다.
>
> **원문과의 관계:** 이 파일은 `CONTEXT.md`의 한국어판이다. 두 파일의 기술 식별자, 수치, 레지스터 맵, UAPI, MVP 범위 및 마일스톤은 동일하게 유지해야 한다. 두 파일이 충돌하면 Codex는 임의로 하나를 선택하지 말고 차이를 보고해야 한다.
>
> **최종 갱신일:** 2026-07-27

---

## 0. Codex 작업 지침

Codex는 이 저장소에서 작업할 때마다 다음 규칙을 따라야 한다.

1. 코드를 변경하기 전에 이 파일과 `PROJECT_STATUS.md`를 모두 읽는다.
2. 이 문서를 프로젝트 명세로 취급한다. 아키텍처, 레지스터 맵, UAPI, 마일스톤 순서 또는 MVP 범위를 암묵적으로 변경하지 않는다.
3. 작업을 제안하기 전에 실제 저장소를 검사한다. 파일, 기능, 의존성, 테스트 또는 빌드 결과가 이미 존재한다고 가정하지 않는다.
4. 한 번에 하나의 마일스톤 또는 범위가 좁은 이슈 하나만 처리한다.
5. 수정 전에 다음을 명시한다.
   - 처리할 마일스톤;
   - 변경이 예상되는 파일;
   - 완료를 입증할 테스트 또는 Acceptance Condition.
6. 수정 후에는 다음을 보고한다.
   - 변경된 파일;
   - 실제 실행한 명령어;
   - 실제 관찰한 테스트 결과;
   - 해결되지 않은 실패 또는 제한사항;
   - 다음으로 권장하는 단일 작업.
7. 빌드나 테스트를 실제 실행하고 결과를 확인하지 않았다면 통과했다고 주장하지 않는다.
8. 동작을 변경할 때마다 테스트를 추가하거나 갱신한다. 단순히 컴파일된다는 이유만으로 기능이 완료된 것은 아니다.
9. Commit은 작고 검토 가능하게 유지한다. 하나의 논리적 변경을 하나의 Commit으로 구성하는 것을 우선한다.
10. 마일스톤 항목의 상태가 바뀔 때마다 `PROJECT_STATUS.md`를 갱신한다.
11. 중요한 설계 변경은 `docs/adr/` 아래 Architecture Decision Record로 기록한다.
12. MVP 완료 기준을 충족하기 전에는 DMA, MSI/MSI-X, Multi-device Scheduling, ARM64 지원, pybind11 또는 성능 최적화를 추가하지 않는다.
13. 이 프로젝트가 실제 Rebellions Hardware, 실제 NPU Silicon, 실제 Post-silicon Validation 또는 Production Driver Code를 사용한다고 암시하지 않는다.
14. QEMU 또는 Linux 관련 코드를 수정할 때 Upstream License를 보존한다. 구성요소별로 적절한 SPDX Identifier를 추가한다.
15. CI에서 실행할 수 있도록 결정적이고 Scriptable하며 Headless한 실행 방식을 우선한다.
16. 필요한 도구나 환경이 없어 막힌 경우, 부분 구현을 보존하고 정확한 Blocker와 최소 재현 가능한 다음 단계를 설명한다. 출력이나 결과를 만들어내지 않는다.

---

## 1. 프로젝트 동기

지원자는 다음 경험을 CV에서 직접 입증하고 있다.

- C/C++ 및 Low-level Assembly 분석;
- RISC-V, ARMv8, x86 ISA 이해;
- Linux Kernel 실행 흐름 및 Interrupt Path 분석;
- GDB/LLDB 기반 Debugging;
- Fuzzer 구조 분석 및 수정;
- Fault Analysis 및 Differential Testing;
- 자동화된 Validation Workflow;
- Low-level 비정상 동작 재현 및 Root Cause 규명.

이 프로젝트가 보완해야 하는 주요 갭은 다음과 같다.

- Linux Device Driver 직접 구현;
- Register Map 및 MMIO 설계;
- Interrupt-driven Device Control;
- QEMU Virtual Device 개발;
- C++ Hardware Abstraction Layer 설계;
- Python 기반 Validation Automation;
- TDD, Unit Test, Integration Test 및 CI;
- Hardware Revision Abstraction 및 Regression Test.

따라서 이 프로젝트는 기존의 Low-level 분석·검증 강점을 구체적인 End-to-end **Virtual Hardware → Linux Driver → C++ HAL → Automated Validation** 구현으로 확장해야 한다.

---

## 2. 한 문장 프로젝트 정의

**QEMU Virtual NPU-like PCI Device**, **Interrupt-driven Linux Device Driver**, **Revision-independent C++ HAL**, 그리고 Fault Injection과 Differential Testing을 포함한 **pytest 기반 Regression Framework**로 구성된 재현 가능한 Validation Platform을 구축한다.

---

## 3. 프로젝트 목표

완성된 프로젝트는 다음 역량을 입증해야 한다.

1. Register Map을 통해 Hardware/Software Contract를 정의하고 문서화한다.
2. QEMU에서 Virtual Accelerator Device를 구현한다.
3. Linux PCI Driver에서 Device를 Enumeration하고 제어한다.
4. User-space-only Simulation이 아니라 MMIO와 Interrupt를 사용한다.
5. Character Device와 `ioctl`을 통해 안정적인 Userspace UAPI를 제공한다.
6. Hardware Revision 차이를 C++ HAL 뒤에 숨긴다.
7. Software Reference Model과 비교해 기능적 정확성을 검증한다.
8. Fault를 주입하고 Timeout, Error Detection, Recovery 동작을 검증한다.
9. 두 개의 Simulated Hardware Revision에 동일한 Test Logic을 적용한다.
10. 전체 Stack을 CI에서 자동으로 Build하고 Test한다.
11. Architecture, Requirements, Debugging Case, Limitations 및 Test Evidence를 문서화한다.

---

## 4. 명시적 비목표(Non-goals)

다음 항목은 MVP 범위 밖이며 핵심 산출물을 방해해서는 안 된다.

- 실제 Neural-network Accelerator 구현;
- Rebellions Hardware 또는 Proprietary ISA 재현;
- 전체 PCIe Protocol Stack 또는 PHY 동작 구현;
- 실제 Silicon, Board 또는 Post-silicon Validation 경험 주장;
- Production-grade Security 또는 Performance Isolation 구현;
- MVP에서 DMA 또는 IOMMU/SMMU 구현;
- MVP에서 MSI/MSI-X 구현;
- MVP에서 여러 Outstanding Job 또는 Queue Scheduling 구현;
- Kernel Bypass, VFIO, SR-IOV 또는 Virtualization Passthrough 구현;
- x86_64 Reference Environment가 안정화되기 전 ARM64 Guest 지원;
- Compute Performance 최적화;
- GUI 추가;
- 완전한 Machine-learning Framework Integration 구축.

---

## 5. 목표 산출물

완성된 저장소에는 다음 항목이 모두 포함되어야 한다.

### 5.1 실행 가능한 구성요소

- QEMU Virtual NPU Device Model;
- Linux Out-of-tree PCI Driver;
- Character-device UAPI 및 `ioctl` 정의;
- C++ HAL Library;
- JSON 출력을 지원하는 `vnpuctl` Command-line Tool;
- Python Reference Model;
- pytest Functional, Fault 및 Revision-regression Suite;
- GoogleTest HAL Unit Test;
- Core Integration이 동작한 이후 선택적으로 추가하는 KUnit Test;
- 재현 가능한 Build, Run 및 Test Script;
- CI Workflow.

### 5.2 문서

- 프로젝트 개요 및 Quick Start;
- Architecture Diagram;
- Register Specification;
- Driver Design;
- UAPI Specification;
- Validation Plan;
- Requirements-to-test Traceability Matrix;
- Hardware Revision Matrix;
- 실제로 발생한 실패를 기반으로 한 Debugging Note;
- Limitations 및 Non-goals;
- Demo Instructions;
- 구성요소별 License Note.

### 5.3 증거 자료

- Device가 Enumeration됨을 보여주는 `lspci` 결과;
- Driver가 정상적으로 Probe/Remove됨을 보여주는 `dmesg` 결과;
- Functional Test Log;
- Differential Test 결과;
- Fault Injection 및 Recovery Log;
- Revision A/B Regression 결과;
- CI Log 및 보관된 Serial/Kernel Log;
- 프로젝트 완료 후 짧은 Demo Video.

---

## 6. 상위 수준 아키텍처

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

### 6.1 아키텍처 원칙

- Register Specification은 QEMU와 Driver 사이의 계약이다.
- UAPI는 Driver와 User Space 사이의 계약이다.
- HAL은 Application 및 Validation Logic이 사용하는 계약이다.
- HAL 위의 Test는 Register Offset을 Hard-code하면 안 된다.
- Revision-specific Behavior는 Test 전체에 중복하지 않고 Device/Driver/HAL 경계에서 처리한다.
- 동일한 Functional Test가 Revision A와 B 모두에서 실행되어야 한다.
- Fault Injection은 결정적이고 재현 가능해야 한다.
- MVP는 Device당 하나의 Outstanding Operation만 지원한다.

---

## 7. 기준 개발 환경

초기 Reference Environment는 의도적으로 단순하고 Hardware-independent하게 구성한다.

| 영역 | 기준 구성 |
|---|---|
| Host OS | Ubuntu 또는 동등한 Linux Distribution |
| Guest Architecture | x86_64 |
| QEMU Machine | `q35` |
| Acceleration | QEMU TCG; KVM은 선택 사항이며 필수가 아님 |
| Guest OS | Linux Kernel + Buildroot로 생성한 Root Filesystem |
| Virtual Device | q35 PCIe Topology에 연결되는 Custom QEMU PCI Endpoint |
| MVP Interrupt | Legacy INTx |
| Driver | Out-of-tree Linux PCI Kernel Module |
| User-space Language | C++20 |
| Build System | QEMU Meson/Ninja, Kernel Kbuild, HAL/CLI용 CMake |
| Python Test | Python 3 + pytest |
| C++ Unit Test | GoogleTest |
| Debugging | GDB, QEMU GDB Stub, `dmesg`, `lspci`, Serial Log |
| CI | GitHub Actions |
| Diagram | Mermaid 또는 PlantUML |

### 7.1 버전 정책

- QEMU, Linux, Buildroot, Compiler, CMake, Python, pytest의 정확한 버전은 Milestone 0에서 `docs/versions.md` 또는 Lock/Config File에 고정한다.
- 고정한 버전을 암묵적으로 변경하지 않는다.
- Patch, API 또는 재현성에 영향을 주는 버전 변경은 짧은 ADR을 요구한다.
- Source Tree는 `.deps/` 또는 `third_party/src/` 같은 Generated Directory에 내려받고 전체를 Commit하지 않는다.
- QEMU 변경은 고정된 Upstream Revision에 적용할 수 있는 작은 Patch Series로 관리하는 것을 우선한다.

---

## 8. Virtual NPU 기능 명세

### 8.1 연산 기능

Device는 작은 Signed INT8 Dot Product를 수행한다.

```text
result = Σ (input_a[i] × input_b[i]), for i = 0 ... vector_length - 1
```

- Input Element는 Signed 8-bit Integer이다.
- Output은 Signed 32-bit Integer이다.
- Revision A는 `vector_length = 16`만 지원한다.
- Revision B는 `vector_length = 8` 또는 `16`을 지원한다.
- MVP에서 Input Data는 MMIO Register로 전달한다.
- 연산은 의도적으로 작게 유지한다. 본 프로젝트의 평가 대상은 Accelerator 성능이 아니라 Software Stack과 Validation Design이다.

### 8.2 PCI Identity

구현 시 교육용 Local PCI Vendor/Device Pair를 정의한다.

- 선택한 Identifier가 고정한 QEMU Tree와 충돌하는지 확인한다.
- Identifier를 `docs/register-map.md`에 문서화한다.
- 저장소에는 해당 ID가 Local Virtual Device용이며 공식 Vendor Allocation이 아님을 명시한다.

권장 시작값은 다음과 같으며 사용 전에 충돌 여부를 검증해야 한다.

```text
Vendor ID: 0x1B36   # QEMU virtual-device vendor space
Device ID: 0x1000   # local project value; verify before use
Class:     processing accelerator or other documented experimental choice
```

### 8.3 BAR Layout

- BAR0 크기: 4 KiB.
- Register Width: 별도 명시가 없으면 32 bits.
- Endianness: Little-endian.
- 구현되지 않은 Offset은 Register Specification에서 다르게 정의하지 않는 한 0을 반환한다.
- 잘못된 Write로 QEMU가 Crash하면 안 된다.

### 8.4 Register Map

| Offset | Register | Access | Reset value | 목적 |
|---:|---|---|---:|---|
| `0x000` | `DEVICE_ID` | RO | `0x564E5055` | ASCII-like `VNPU` Device Signature |
| `0x004` | `REVISION` | RO | Revision-dependent | A는 `1`, B는 `2` |
| `0x008` | `CAPABILITIES` | RO | Revision-dependent | Feature Bitmap |
| `0x00C` | `CONTROL` | WO/W1S | `0` | Start 또는 Reset Command |
| `0x010` | `STATUS` | RO | `IDLE` | Device State |
| `0x014` | `IRQ_STATUS` | RW1C | `0` | Pending Completion/Error IRQ |
| `0x018` | `IRQ_ENABLE` | RW | `0` | IRQ Mask |
| `0x01C` | `ERROR_CODE` | RO | `0` | 마지막 Device Error |
| `0x020` | `JOB_ID` | RW | `0` | Revision B Job Identifier |
| `0x024` | `VECTOR_LENGTH` | RW | `16` | Revision-dependent Vector Length |
| `0x100`–`0x10C` | `INPUT_A[0..3]` | RW | `0` | 16개의 Packed INT8 Value |
| `0x120`–`0x12C` | `INPUT_B[0..3]` | RW | `0` | 16개의 Packed INT8 Value |
| `0x140` | `RESULT` | RO | `0` | Signed INT32 Dot-product Result |
| `0x180` | `FAULT_CONTROL` | RW | `0` | Runtime Fault-injection Bitmap |

### 8.5 Register Bit 정의

#### `CAPABILITIES`

| Bit | Name | 의미 |
|---:|---|---|
| 0 | `CAP_DOT_INT8` | INT8 Dot Product 지원 |
| 1 | `CAP_JOB_ID` | `JOB_ID` 지원 |
| 2 | `CAP_VARIABLE_LENGTH` | Vector Length 8 또는 16 지원 |
| 3 | `CAP_SEPARATE_ERROR_IRQ` | Error IRQ를 Completion IRQ와 구분 가능 |
| 4 | `CAP_FAULT_INJECTION` | Test-only Runtime Fault Injection 지원 |

권장 Capability Set:

```text
Revision A: CAP_DOT_INT8 | CAP_FAULT_INJECTION
Revision B: CAP_DOT_INT8 | CAP_JOB_ID | CAP_VARIABLE_LENGTH |
            CAP_SEPARATE_ERROR_IRQ | CAP_FAULT_INJECTION
```

#### `CONTROL`

| Bit | Name | 의미 |
|---:|---|---|
| 0 | `CONTROL_START` | 하나의 Operation 시작; Write-one Action |
| 1 | `CONTROL_RESET` | Device State Reset; Write-one Action |

#### `STATUS`

| Bit | Name | 의미 |
|---:|---|---|
| 0 | `STATUS_IDLE` | 새 Command를 받을 준비가 됨 |
| 1 | `STATUS_BUSY` | Operation 진행 중 |
| 2 | `STATUS_DONE` | 마지막 Operation이 성공적으로 완료됨 |
| 3 | `STATUS_ERROR` | 마지막 Operation이 실패함 |

#### `IRQ_STATUS` 및 `IRQ_ENABLE`

| Bit | Name | 의미 |
|---:|---|---|
| 0 | `IRQ_COMPLETION` | Operation 완료 |
| 1 | `IRQ_ERROR` | Device Error 발생 |

`IRQ_STATUS`는 Write-one-to-clear Semantics를 사용한다.

#### `FAULT_CONTROL`

| Bit | Name | 의미 |
|---:|---|---|
| 0 | `FAULT_IRQ_DROP` | Operation을 완료하지만 IRQ를 발생시키지 않음 |
| 1 | `FAULT_STUCK_BUSY` | Reset 전까지 Device를 BUSY 상태로 유지 |
| 2 | `FAULT_CORRUPT_RESULT` | 계산 결과를 의도적으로 변경 |
| 3 | `FAULT_FORCE_ERROR` | 성공 대신 Device Error로 종료 |

Invalid Device Signature 또는 Unsupported Revision처럼 Driver Probe에 영향을 주는 Boot-time Fault는 Runtime Register Bit가 아니라 QEMU Device Property로 구현한다.

권장 QEMU Property:

```text
revision=a|b|unknown
bad-device-id=on|off
operation-delay-us=<integer>
```

### 8.6 Error Code

| Value | Name | 의미 |
|---:|---|---|
| 0 | `VNPU_ERR_NONE` | Error 없음 |
| 1 | `VNPU_ERR_INVALID_LENGTH` | 지원하지 않는 Vector Length |
| 2 | `VNPU_ERR_BUSY` | 이미 Busy 상태에서 Start 요청 |
| 3 | `VNPU_ERR_FORCED` | Fault Injection에 의해 강제된 Error |
| 4 | `VNPU_ERR_UNSUPPORTED_REVISION` | 지원하지 않는 Revision |
| 5 | `VNPU_ERR_INTERNAL` | Internal Model Error |

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

Behavior Rule:

- `START`는 유효한 Operation을 시작하기 전에 이전 `DONE`, `ERROR`, IRQ Status 및 Error Code를 Clear한다.
- `RESET`이 Runtime Fault State도 Clear할지는 Specification에서 명시적으로 결정하고 문서화한다.
- QEMU Device는 QEMU Timer 또는 동등한 Non-blocking Mechanism을 사용해 Completion을 Asynchronous하게 모델링한다.
- 기본 Operation Latency는 결정적이고 설정 가능해야 한다.
- Revision A가 Separate Error IRQ Capability를 Advertise하지 않는 경우 모든 Error를 Completion Line으로 노출할 수 있다.
- Revision B는 Completion과 Error IRQ Status를 구분해야 한다.

### 8.8 Revision Matrix

| Feature | Revision A | Revision B |
|---|---|---|
| INT8 Dot Product | 지원 | 지원 |
| Vector Length | 16 고정 | 8 또는 16 |
| Job ID | 미지원; Read는 0, Write는 무시 | 지원 |
| Completion IRQ | 지원 | 지원 |
| Separate Error IRQ Status | 미지원 | 지원 |
| Fault Injection | 지원 | 지원, 더 명확한 Error Reporting |
| HAL-visible API | 동일 | 동일 |

HAL은 가능한 한 Revision Number Check를 여러 위치에 흩어 놓지 말고 `CAPABILITIES`를 사용해야 한다.

---
## 9. Linux Driver 명세

### 9.1 Driver Identity

권장 명칭:

```text
Kernel module: vnpu_drv
Device node:   /dev/vnpu0
Class name:    vnpu
UAPI header:   include/uapi/linux/vnpu.h or project-local equivalent
```

### 9.2 필수 Driver 기능

MVP Driver는 다음 기능을 구현해야 한다.

1. PCI ID Table 등록.
2. `probe()` 및 `remove()`.
3. `pci_enable_device()`.
4. `pci_request_regions()`.
5. `pci_iomap()` 또는 동등한 Managed API를 통한 BAR0 Mapping.
6. IRQ Request 및 Release.
7. Character-device 등록.
8. 안정적인 `ioctl` UAPI.
9. 한 번에 하나의 Operation을 제출하는 Command Submission.
10. Interrupt-driven Completion.
11. 상한이 있는 Timeout 처리.
12. Timeout/Error 이후 Reset 및 Recovery.
13. 모든 Failure Path에서 일관된 Cleanup.
14. 반복 가능한 Module Load/Unload.
15. Log Spam 없이 유용한 `dev_dbg`, `dev_info`, `dev_err` Logging.

### 9.3 Concurrency Model

MVP는 Device당 하나의 Outstanding Job만 지원한다.

권장 Synchronization:

- Mutex가 Command Submission 및 User-facing Device State를 보호한다.
- Completion Object 또는 Wait Queue가 IRQ-driven Completion을 처리한다.
- IRQ에서 접근하는 Field에는 적절한 Atomicity 또는 Locking을 적용한다.
- Reset 및 Recovery는 Process Context에서 수행한다.
- IRQ Handler는 짧게 유지하고 Sleep하면 안 된다.

Codex는 각 Synchronization Primitive를 선택한 이유를 문서화해야 한다.

### 9.4 UAPI 요구사항

UAPI는 Fixed-width Type을 사용하고 ABI Version을 포함해야 한다.

권장 Command:

```text
VNPU_IOCTL_GET_INFO
VNPU_IOCTL_RUN_DOT
VNPU_IOCTL_RESET
VNPU_IOCTL_SET_FAULT
VNPU_IOCTL_GET_STATS
```

권장 Structure:

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

정확한 ABI는 구현 전에 다듬을 수 있다. 그러나 코드가 존재한 이후 ABI를 변경하려면 다음이 필요하다.

- UAPI 문서 갱신;
- ABI Version 처리 근거;
- Test;
- Compatibility에 영향을 주는 경우 ADR.

### 9.5 Driver Validation 및 Safety Rule

Driver는 다음을 수행해야 한다.

- `abi_version` 검증;
- MMIO Access 전에 Vector Length 검증;
- 지원하지 않는 Feature를 명확하게 거부;
- `copy_from_user()` 및 `copy_to_user()`의 올바른 사용;
- Userspace Pointer를 직접 사용하지 않음;
- Arithmetic 및 Array Bound 확인;
- Submission 전에 이전 Completion State를 Clear;
- IRQ Status를 올바르게 Acknowledge;
- 무한 대기 방지;
- Timeout 후 Recovery하거나 명확한 Error 반환;
- 획득한 Resource를 역순으로 해제;
- Unsupported Device Signature 또는 Revision을 명확한 Log와 함께 거부;
- 실패한 Operation 이후에도 Module을 Unload할 수 있어야 함.

### 9.6 Timeout 및 Recovery 동작

권장 순서:

1. Request 검증;
2. Submission Path Lock;
3. 이전 IRQ/Completion State Clear;
4. Input, Vector Length, 지원되는 경우 Job ID, IRQ Mask Programming;
5. `START` 발행;
6. 상한이 있는 Timeout으로 Completion 대기;
7. 성공 시 Result 및 Status Read;
8. Device Error 발생 시 `ERROR_CODE`를 수집하고 Mapping된 Error 반환;
9. Timeout 시 Stats 증가, Device Reset 발행, IDLE 복귀 확인, `-ETIMEDOUT` 반환;
10. Unlock 후 반환.

### 9.7 Driver Test

최소한 다음을 입증해야 한다.

- 성공적인 Probe 및 Remove;
- 유효한 Device Info;
- 정확한 Operation Result;
- Signed Input 정확성;
- Invalid Vector Length 거부;
- Start-while-busy 동작;
- IRQ Completion;
- IRQ-drop Timeout;
- Stuck-busy Reset Recovery;
- Error Interrupt Handling;
- Unsupported Revision 거부;
- Bad Device Signature 거부;
- Resource Leak 또는 Kernel Warning 없이 반복되는 Module Load/Unload.

문서화할 Stress Check 목표:

```text
At least 50 module load/unload cycles with no kernel warning or stale device node.
```

이는 실행 전부터 주장할 결과가 아니라 Acceptance Target이다.

---

## 10. C++ HAL 및 CLI 명세

### 10.1 HAL 목표

HAL은 다음을 수행해야 한다.

- Raw `ioctl` 및 File Descriptor Handling을 숨김;
- RAII Resource Management 제공;
- Revision-independent Operation 노출;
- Capability를 Query하고 사용;
- Driver/Device Failure를 구조화된 C++ Error로 변환;
- Mock Backend를 통해 QEMU 없이 Unit Test 지원;
- Caller에게 Register Offset을 노출하지 않음.

### 10.2 권장 Interface

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

필수 구현체:

```text
LinuxVnpuDevice   # communicates with /dev/vnpu0
MockVnpuDevice    # deterministic unit-test backend
```

### 10.3 HAL 동작

- Initialization 과정에서 Capability를 한 번 Detect하고 Immutable Information을 Cache한다.
- Driver를 호출하기 전에 Vector Length를 검증한다.
- Revision A와 B에 동일한 Public API를 유지한다.
- `JOB_ID` Capability가 없는 경우 내부에서 처리한다.
- Linux `errno`, Driver Status, Device Error를 구분 가능한 Error Category로 Mapping한다.
- 명시적으로 문서화한 경우를 제외하고 숨겨진 Retry를 수행하지 않는다.
- Doxygen-compatible API Comment를 포함한다.

### 10.4 CLI

Executable Name:

```text
vnpuctl
```

필수 Command:

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

JSON Output은 pytest가 안정적으로 Parse할 수 있어야 한다. Human-readable Output을 추가로 지원할 수 있지만 자동 테스트는 JSON을 사용해야 한다.

### 10.5 HAL Unit Test

GoogleTest는 최소한 다음을 검증해야 한다.

- Capability Parsing;
- Revision A Fixed-length Behavior;
- Revision B Variable-length Behavior;
- Unsupported Length 거부;
- Driver Error Mapping;
- Device Error Mapping;
- Timeout Mapping;
- Mock Backend Success/Failure Path;
- Logic을 포함하는 경우 JSON Serialization Helper.

---

## 11. Python Validation Framework

### 11.1 Validation 전략

Python은 Orchestration 및 Validation에 사용하며 C++ HAL을 대체하지 않는다.

정상 경로:

```text
pytest → vnpuctl JSON CLI → C++ HAL → ioctl → Linux driver → QEMU device
```

### 11.2 Reference Model

결정적인 Software Reference를 구현한다.

```python
def dot_reference(a: list[int], b: list[int]) -> int:
    return sum(int(x) * int(y) for x, y in zip(a, b))
```

Reference는 다음을 검증해야 한다.

- 동일한 Length;
- 지원되는 Length;
- Signed INT8 Range;
- 명시적인 Random Seed를 사용한 결정적 Input Generation.

### 11.3 필수 pytest Case

| Test | 목적 |
|---|---|
| `test_device_enumeration` | QEMU가 기대한 PCI Device를 노출하는지 확인 |
| `test_driver_probe` | Driver가 Bind되고 `/dev/vnpu0`을 생성하는지 확인 |
| `test_device_info` | Device ID, Revision, Capability 정확성 확인 |
| `test_dot_product_basic` | 기본 정상 Operation |
| `test_dot_product_negative_values` | Signed INT8 처리 확인 |
| `test_dot_product_boundaries` | INT8 Min/Max 및 Result Range 확인 |
| `test_dot_product_randomized` | Seeded Randomized Functional Coverage |
| `test_result_differential` | Hardware-model Output과 Python Reference 일치 확인 |
| `test_irq_completion` | Interrupt-driven Completion 관찰 |
| `test_irq_drop_timeout` | IRQ 누락 시 상한이 있는 Timeout 발생 |
| `test_stuck_busy_recovery` | Timeout Recovery가 Device를 IDLE로 Reset |
| `test_corrupt_result_detection` | Differential Test가 잘못된 Result를 검출 |
| `test_forced_error` | Device Error가 올바르게 전달됨 |
| `test_unknown_revision` | Unsupported Revision이 명확히 실패 |
| `test_bad_device_id` | Bad Signature가 정상 Probe를 방지 |
| `test_module_reload` | 반복 Driver Reload 안정성 |
| `test_revision_regression` | 동일 Suite를 Revision A/B에 적용 |

### 11.4 Failure Artifact

Test 실패 시 가능한 범위에서 다음을 보존해야 한다.

- Test Seed;
- Test Parameter;
- QEMU Command Line;
- QEMU Serial Log;
- Guest `dmesg`;
- `lspci -nnvv` Output;
- CLI JSON Result;
- Driver가 안전하게 노출하는 경우 Register/Status Snapshot;
- JUnit XML.

### 11.5 Test Marker

권장 pytest Marker:

```text
unit
integration
fault
revision_a
revision_b
slow
```

Fast Suite와 Slow Suite를 분리할 수 있어야 한다.

---

## 12. Test-driven Development 기대사항

핵심 동작 여러 개에 대해 의미 있는 Test-first Development 이력을 보여줘야 한다.

권장 Commit Pattern:

```text
test: add failing capability parser test
feat: implement capability parser

test: add IRQ timeout integration case
feat: add driver timeout and reset recovery

test: add revision B variable-length regression
feat: implement revision B vector-length capability
```

모든 Setup Commit이 Test-first일 필요는 없다. 그러나 최소한 다음 기능은 Test가 Fix/Implementation보다 먼저 추가된 이력이 보여야 한다.

- Capability Parsing;
- IRQ Timeout Handling;
- Reset Recovery;
- Revision B Behavior;
- Corrupt-result Detection.

---

## 13. 저장소 구조

```text
virtual-npu-validation-platform/
├── CONTEXT.md
├── CONTEXT_KO.md
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

Generated Source Tree 및 Build Output은 다음과 같이 Ignore해야 한다.

```text
.deps/
out/
build/
artifacts/
```

---

## 14. 요구되는 Developer Experience

저장소는 최종적으로 다음 Command Interface를 제공해야 한다.

```bash
# Host dependency를 점검하고 누락된 package를 설명한다.
./scripts/bootstrap.sh

# 고정된 QEMU, Linux, Buildroot source를 가져온다.
./scripts/fetch-sources.sh

# 모든 Host 및 Guest component를 build한다.
./scripts/build-all.sh

# Revision A를 interactive 또는 headless mode로 실행한다.
./scripts/run-qemu.sh --revision a

# Revision B를 실행한다.
./scripts/run-qemu.sh --revision b

# 모든 automated test를 실행한다.
./scripts/run-tests.sh --revision all

# 빠른 host-side test만 실행한다.
./scripts/run-tests.sh --suite fast

# 재현성 관련 artifact를 수집한다.
./scripts/collect-logs.sh
```

요구사항:

- 적절한 Script에는 `set -euo pipefail`을 사용한다.
- Script는 실행 가능한 Error Message와 함께 실패해야 한다.
- 모든 Command를 `README.md`에 문서화한다.
- KVM 없이도 프로젝트가 동작해야 한다.
- Fetched Upstream Tree 내부에서 문서화되지 않은 Manual Edit를 요구하면 안 된다.
- Patch는 자동이고 Idempotent하게 적용되어야 한다.
- Dependency 설치 후 Clean Checkout만으로 Environment를 재현할 수 있어야 한다.

---

## 15. CI 설계

### 15.1 Fast CI

모든 Pull Request에서 다음을 실행한다.

- Formatting Check;
- 가능한 범위의 Static Analysis;
- 고정된 Kernel Header/Tree에 대한 Driver Compilation;
- C++ HAL 및 CLI Build;
- GoogleTest;
- Python Lint 및 Unit Test;
- Documentation Link Check;
- Patch Application Check.

### 15.2 Integration CI

Main Branch Push, Scheduled Build 또는 Manual Dispatch에서 실행한다.

1. 고정된 Source Fetch;
2. Patched QEMU Build;
3. Guest Kernel 및 Root Filesystem Build;
4. TCG로 QEMU Headless Boot;
5. Driver Load;
6. Revision A Regression;
7. Revision B Regression;
8. Fault-injection Test;
9. JUnit XML Export;
10. Serial Log, `dmesg`, `lspci`, CLI JSON 및 Test Report Archive.

유효한 경우 QEMU, Kernel, Buildroot Download 및 Compiler Output에 Cache를 사용한다. Cache Key에는 고정된 Revision 및 관련 Configuration Hash가 포함되어야 한다.

---

## 16. 문서화 요구사항

### 16.1 `README.md`

권장 순서:

1. Problem Statement;
2. 한 Paragraph의 Solution;
3. Architecture Diagram;
4. Key Feature;
5. Quick Start;
6. 현재 Implementation Status;
7. 실제 실행 결과만을 사용한 Sample Output;
8. Validation Strategy;
9. Repository Layout;
10. Limitation;
11. License Note.

### 16.2 `docs/debugging-notes.md`

중요한 실제 Debugging Case는 다음 구조를 사용한다.

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

좋은 후보 사례:

- 잘못된 IRQ Acknowledgement 순서;
- RW1C 구현 오류;
- Stale Completion State;
- Failed Probe의 Resource Cleanup;
- Timeout Recovery 후 BUSY가 남는 문제;
- Revision Capability Parsing Bug;
- MMIO Packing/Sign-extension Bug;
- Module Unload Race.

실제로 문제가 발생하기 전에는 Debugging Story를 미리 작성하지 않는다.

### 16.3 `docs/limitations.md`

다음을 명시적으로 기록한다.

- Device는 실제 NPU Silicon이 아닌 QEMU Model이다.
- PCIe PHY, Link Training, Timing, Signal Integrity 또는 Board Behavior를 검증하지 않는다.
- MVP는 Production DMA 대신 MMIO Input을 사용한다.
- Post-silicon Behavior를 재현하지 않는다.
- MVP는 Legacy INTx를 사용한다.
- Performance 결과는 실제 Hardware를 대표하지 않는다.
- Fault Injection은 Software-controlled이며 Deterministic하다.

---
## 17. Requirements-to-Test Traceability

`docs/requirements-traceability.md`에 다음과 유사한 표를 유지한다.

| Requirement ID | 요구사항 | 검증 방법 |
|---|---|---|
| `REQ-ENV-001` | Clean Checkout에서 고정된 Source를 Fetch하고 Build할 수 있어야 함 | CI Clean-build Job |
| `REQ-DEV-001` | 문서화한 PCI Identity로 Device가 Enumeration되어야 함 | `test_device_enumeration` |
| `REQ-DEV-002` | Register Reset Value가 Specification과 일치해야 함 | Register-level Test |
| `REQ-CMD-001` | 유효한 Dot-product Command가 정확한 Result를 반환해야 함 | `test_result_differential` |
| `REQ-CMD-002` | Signed INT8 Boundary를 올바르게 처리해야 함 | `test_dot_product_boundaries` |
| `REQ-IRQ-001` | 정상 완료 시 Enable된 IRQ가 발생해야 함 | `test_irq_completion` |
| `REQ-ERR-001` | IRQ Drop 시 상한이 있는 Timeout이 발생해야 함 | `test_irq_drop_timeout` |
| `REQ-REC-001` | Timeout Recovery 후 Device가 IDLE로 복귀해야 함 | `test_stuck_busy_recovery` |
| `REQ-REV-001` | Revision A는 Length 16만 지원해야 함 | Revision A Test |
| `REQ-REV-002` | Revision B는 Length 8과 16을 지원해야 함 | Revision B Test |
| `REQ-REV-003` | 하나의 HAL API가 두 Revision 모두에서 동작해야 함 | Parameterized HAL/Integration Test |
| `REQ-UAPI-001` | Invalid ABI Version을 거부해야 함 | UAPI Validation Test |
| `REQ-DRV-001` | Failed Probe가 획득한 모든 Resource를 해제해야 함 | Probe-failure Test/Log Inspection |
| `REQ-DRV-002` | Module을 반복 Load/Unload할 수 있어야 함 | `test_module_reload` |
| `REQ-DOC-001` | Public HAL API가 문서화되어야 함 | Doxygen Build Check |
| `REQ-CI-001` | Integration CI가 Failure Evidence를 Archive해야 함 | Workflow Artifact Check |

실행 가능한 Test 또는 문서화된 Inspection Procedure가 연결되지 않은 Requirement는 Verified로 표시하지 않는다.

---

## 18. 6주 마일스톤 계획

가정: 주당 약 15~20시간의 집중 작업.

### Milestone 0 — 저장소 및 재현 가능한 환경

**목표:** 프로젝트 Skeleton과 Clean/Pinned Build Environment를 구축한다.

작업:

- Repository Structure 생성;
- `PROJECT_STATUS.md` 생성;
- Pinned Version 정의;
- Source-fetch Script 구현;
- 수정하지 않은 QEMU Build;
- Baseline Buildroot Linux Guest Boot;
- Serial Logging 구성;
- 초기 Architecture 및 Register-map 문서 생성;
- CI Skeleton 생성.

Acceptance Criteria:

- Clean Checkout에서 필요한 Source를 Fetch할 수 있음;
- 문서화된 Command로 QEMU Guest가 Boot됨;
- Guest 내부에서 `lspci`가 실행됨;
- Generated Output이 Version Control 밖에 있음;
- Virtual NPU 구현이 완료된 것처럼 잘못 표시하지 않음.

### Milestone 1 — 최소 PCI Enumeration

**목표:** Guest에 최소 Custom Virtual Device를 표시한다.

작업:

- QEMU Device Skeleton 추가;
- Local PCI ID 정의;
- BAR0 추가;
- 고정된 `DEVICE_ID`, `REVISION`, `CAPABILITIES` Register 추가;
- 기본 Enumeration Test 생성.

Acceptance Criteria:

- `lspci -nn`에 Custom Device가 표시됨;
- BAR0 크기가 4 KiB임;
- Register Read가 문서화된 Reset Value를 반환함;
- Test Automation이 Enumeration Failure를 검출할 수 있음.

### Milestone 2 — 기능적 Device Model

**목표:** Revision A의 Device Behavior, MMIO Compute, Asynchronous Completion, IRQ 및 기본 Fault를 구현한다.

작업:

- Input, Control, Status, Result, IRQ, Fault Register 구현;
- Signed INT8 Dot Product 구현;
- Timer-based Asynchronous Completion 구현;
- Legacy INTx 구현;
- Revision A 구현;
- `IRQ_DROP`, `STUCK_BUSY`, `CORRUPT_RESULT`, `FORCE_ERROR` 구현;
- State Machine 문서화.

Acceptance Criteria:

- MMIO Command가 정확한 Result를 생성함;
- Enable된 경우 Completion Interrupt가 발생함;
- 각 Runtime Fault가 Deterministic함;
- Invalid Access로 QEMU가 Crash하지 않음;
- State Transition이 Specification과 일치함.

### Milestone 3 — Linux PCI Driver

**목표:** 실제 Kernel Driver와 Character-device UAPI를 통해 Device를 제어한다.

작업:

- Probe/Remove 및 BAR Mapping 구현;
- IRQ Handler 구현;
- Character Device 구현;
- `ioctl` ABI 구현;
- Timeout 및 Reset Recovery 구현;
- Stats 구현;
- Failure-path Cleanup 추가;
- Driver Test 및 Logging 추가.

Acceptance Criteria:

- Driver가 Probe되고 `/dev/vnpu0`을 생성함;
- Userspace가 하나의 Operation을 Submit할 수 있음;
- Completion이 Interrupt-driven임;
- IRQ-drop이 Timeout을 발생시킴;
- Stuck-busy를 Reset으로 Recovery함;
- Unsupported Device/Revision Probe가 명확히 실패함;
- 반복 Load/Unload가 안정적임.

### Milestone 4 — C++ HAL 및 CLI

**목표:** 깨끗하고 Revision-independent한 Userspace Interface를 제공한다.

작업:

- `IVnpuDevice` 구현;
- Linux 및 Mock Backend 구현;
- Error Model 구현;
- `vnpuctl` JSON CLI 구현;
- GoogleTest 추가;
- API Documentation 추가.

Acceptance Criteria:

- Application Code에 Register Offset이 없음;
- QEMU 없이 HAL Unit Test 실행 가능;
- CLI가 Info, Run, Reset, Fault, Stats를 지원함;
- JSON Output이 안정적이고 문서화됨;
- Validation, Driver, Timeout, Device Failure를 구분함.

### Milestone 5 — Revision B 및 Python Validation

**목표:** Hardware Revision 간 재사용 가능한 Validation을 입증한다.

작업:

- Revision B Capability 구현;
- Vector Length 8/16 지원;
- Job ID 지원;
- Separate Error IRQ Status 지원;
- Python Reference Model 구현;
- pytest Functional, Fault, Revision Test 구현;
- 동일 Test Logic을 Revision별로 Parameterize;
- JUnit XML 및 Failure Artifact Export.

Acceptance Criteria:

- 하나의 Test Suite가 A와 B에서 실행됨;
- Revision 차이가 불필요하게 High-level Test로 노출되지 않음;
- Randomized Differential Test를 Seed로 재현 가능;
- Corrupt Result를 검출함;
- Failure Evidence를 보존함.

### Milestone 6 — CI, 문서화 및 포트폴리오 완성

**목표:** 프로젝트를 재현 가능하고 검토 가능하며 기술면접에 사용할 수 있는 상태로 완성한다.

작업:

- Fast CI 완성;
- Full QEMU Integration CI 완성;
- Caching 추가;
- Requirements Traceability 완성;
- 실제 Debugging Case 문서화;
- Limitation 완성;
- Architecture Diagram 완성;
- Demo Script 및 Video 제작;
- 정확한 범위의 Resume-ready Project Summary 작성.

Acceptance Criteria:

- Clean CI가 전체 Stack을 Build함;
- CI가 두 Revision 및 Fault Test를 실행함;
- 필수 Artifact가 모두 Archive됨;
- Documentation이 실제 구현과 일치함;
- README Quick Start가 동작함;
- Project Claim이 Virtual Platform 범위로 명확히 제한됨.

---

## 19. MVP 완료 기준

아래 Mandatory Condition을 모두 충족한 경우에만 프로젝트를 완료했다고 부를 수 있다.

### Virtual Device

- Custom QEMU Device가 Enumeration됨;
- BAR0 Register Map이 문서와 일치함;
- Revision A 및 B 지원;
- Signed INT8 Dot Product 동작;
- Asynchronous Completion 동작;
- Interrupt Generation 동작;
- 최소 4개의 Deterministic Fault Mode 동작.

### Driver

- Probe/Remove 동작;
- BAR Mapping 동작;
- Character Device 존재;
- Versioned UAPI;
- 유효한 Operation이 IRQ로 완료됨;
- Timeout에 상한이 있음;
- Reset Recovery 동작;
- Unsupported Device/Revision 동작이 Test됨;
- Resource Cleanup이 검증됨;
- 반복 Module Reload가 Test됨.

### HAL 및 CLI

- C++ HAL이 Raw UAPI Detail을 숨김;
- Linux 및 Mock Backend 존재;
- Revision 차이는 Capability-driven 방식으로 처리;
- Unit Test 통과;
- JSON CLI를 pytest에서 사용 가능;
- API Documentation 생성 또는 Build Check.

### Validation

- Python Reference Model 존재;
- Deterministic Randomized Differential Test 존재;
- Functional Test 존재;
- Fault Test 존재;
- Revision Regression 존재;
- JUnit Output 존재;
- Failure Log 수집.

### Reproducibility

- Pinned Version 문서화;
- Clean Checkout에서 Build/Run/Test Script 동작;
- Fast CI 통과;
- Full Integration CI가 TCG에서 실행됨;
- Documentation이 실제 동작을 반영함;
- Generated Binary를 Commit하지 않음.

---

## 20. 확장 목표 — MVP 이전 착수 금지

MVP 완료 후 남은 시간에 따라 다음 중 최대 1~2개만 선택한다.

1. Coherent Buffer를 사용하는 Simple DMA Path.
2. MSI 또는 MSI-X Interrupt 지원.
3. 두 개의 Virtual Device와 기본 Resource Allocator.
4. ARM64 Guest Port.
5. 더 많은 Driver Logic에 대한 KUnit Coverage.
6. `ioctl` Fuzz Testing.
7. Direct Python HAL Access를 위한 pybind11 Binding.
8. Code Coverage Reporting.
9. qtest 기반 Register-model Test.
10. Concurrent Multi-process Rejection 또는 Serialization Test.

모든 Stretch Goal은 자체 Requirement, Test 및 Documentation을 가져야 한다. 완료된 MVP의 안정성을 약화하면 안 된다.

---

## 21. 리스크 및 Fallback

| 리스크 | 필수 대응 |
|---|---|
| QEMU Custom Device 작업이 예상보다 오래 걸림 | Enumeration → BAR Read → State Machine → IRQ 순서로 완료한다. 모든 Feature를 한 번에 구현하지 않는다. |
| IRQ 구현이 불안정함 | Polling은 임시 진단 용도로만 사용한다. 최종 MVP는 IRQ-driven Completion이어야 한다. |
| Buildroot Integration이 진행을 막음 | 진단 목적으로만 최소 Prebuilt Guest를 임시 사용하고, 이후 Pinned/Reproducible Guest Build로 복귀한다. |
| C++/Python Integration이 범위를 확장함 | Python은 JSON CLI를 호출하도록 유지한다. pybind11은 Stretch Goal이다. |
| CI Build Time이 과도함 | Fast와 Integration Workflow를 분리하고 Pinned Source/Build Artifact를 Cache한다. |
| DMA가 일정을 소모함 | MVP 완료 전 DMA를 구현하지 않는다. |
| Revision Behavior가 인위적으로 보임 | Capability Discovery, Job ID, Length Support, Error Reporting과 같은 현실적인 차이를 유지하고 근거를 문서화한다. |
| Driver Error Path Test가 어려움 | Probe 및 Execution Failure를 의도적으로 발생시키는 QEMU Boot-time Property와 Runtime Fault를 추가한다. |
| 프로젝트가 실제 Silicon 작업으로 오인됨 | README, Documentation, CV 문구 및 Demo에서 Virtual-platform Limitation을 반복해 명시한다. |

---

## 22. Coding 및 Review Convention

### 22.1 General

- 명확하고 설명적인 이름을 사용한다.
- 설명되지 않은 Magic Constant를 피한다.
- Shared Register 및 UAPI Definition은 문서화된 Generation 또는 엄격한 Duplication Check로 동기화한다.
- 가능한 경우 Compiler Warning을 Error로 취급한다.
- 사소한 코드가 아니라 Hardware Semantics, Synchronization, 비자명한 Error Handling에 Comment를 작성한다.
- Public API를 작게 유지한다.

### 22.2 C 및 Kernel Code

- Driver는 Linux Kernel Coding Style을 따른다.
- 적절한 Kernel Fixed-width/UAPI Type을 사용한다.
- Kernel Facility가 존재하는 경우 Custom Linked List 또는 Synchronization Primitive를 만들지 않는다.
- 모든 Allocation 및 Resource-acquisition Result를 확인한다.
- Managed PCI API의 Cleanup Semantics를 이해하고 문서화한 경우에만 사용한다.
- IRQ Context에서 Sleep하지 않는다.
- UAPI Structure Layout을 안정적이고 Architecture-independent하게 유지한다.

### 22.3 QEMU Code

- QEMU Style 및 Object Model Convention을 따른다.
- Device State를 명시적으로 유지한다.
- Asynchronous Behavior에 QEMU Timer/BH Facility를 사용한다.
- Reset Behavior를 완전하고 결정적으로 구현한다.
- QEMU License Header 및 Style Check를 보존한다.
- QEMU Main Loop를 Block하지 않는다.

### 22.4 C++

- C++20을 사용한다.
- RAII 및 Value Type을 우선한다.
- Global Mutable State를 피한다.
- 적절한 경우 Non-owning Buffer에 `std::span`을 사용한다.
- 명시적인 Error Category 또는 문서화된 Exception Hierarchy를 사용한다.
- Linux-specific Detail은 Linux Backend 내부에 제한한다.

### 22.5 Python

- Type Hint를 사용한다.
- Randomized Test를 재현 가능하게 만든다.
- Subprocess Failure를 숨기지 않는다.
- stdout, stderr, Exit Code 및 JSON Parse Error를 명확히 Capture한다.
- Reference-model Code는 단순하게 유지하고 Implementation Logic과 독립시킨다.

---

## 23. `PROJECT_STATUS.md` Template

Codex는 다음 형식의 별도 Status File을 생성하고 유지해야 한다.

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
# 실제 실행한 command만 추가한다.
```

## Last verified results
- 실제 관찰한 result만 추가한다.

## Next task
- 범위가 좁은 단일 작업 하나.

## Known limitations
- QEMU-only virtual platform.
````

Status Rule:

- 검증 후에만 `[x]` 사용;
- 실제 실행한 정확한 Command 포함;
- Expected Output을 Verified-results Section에 복사하지 않음;
- Next Task는 한 번의 Codex Iteration으로 처리할 수 있을 만큼 작게 유지.

---

## 24. 정확한 포트폴리오 및 이력서 포지셔닝

### 24.1 실제 완료 후 사용할 수 있는 표현

적절한 한국어 프로젝트 문구:

> QEMU 기반 가상 NPU PCI 디바이스와 Linux 드라이버를 구현하고, 하드웨어 Revision 차이를 추상화한 C++ HAL 및 pytest 기반 Fault-injection Regression Framework를 구축. MMIO, Interrupt, Timeout Recovery, Differential Testing 및 CI를 통해 Revision A/B의 기능과 오류 처리 동작을 자동 검증.

적절한 영문 프로젝트 문구:

> Developed a QEMU-based virtual NPU PCI device and Linux driver, and built a reusable C++ HAL and pytest-based validation framework. Automated MMIO, interrupt, timeout recovery, differential testing, and hardware-revision regression in CI.

### 24.2 해서는 안 되는 주장

이 프로젝트를 다음과 같이 설명하면 안 된다.

- Actual NPU Silicon Validation;
- Post-silicon Validation;
- Actual PCIe Board Bring-up;
- Production NPU Driver Development;
- Rebellions Hardware Development;
- Real JTAG Debugging;
- Physical Hardware Fault Analysis;
- Commercial Device-driver Experience;
- 실제로 구현·검증하지 않은 DMA/IOMMU Experience;
- 실제로 구현·검증하지 않은 MSI/MSI-X Experience.

정확한 대체 표현:

| 정확한 표현 | 부정확한 표현 |
|---|---|
| QEMU Virtual-device Validation | Real NPU Silicon Validation |
| Virtual PCI Device 및 Linux Driver Project | Production PCIe/NPU Driver |
| Simulated Hardware Revision | Post-silicon Revision Validation |
| Software-controlled Fault Injection | Silicon Fault Analysis |
| QEMU/GDB Debugging | JTAG Board Debugging |
| Project-based Driver Implementation | Professional Driver Employment Experience |

---

## 25. 권장 첫 Codex 작업

첫 Codex Session은 **Milestone 0만** 수행해야 한다.

권장 Prompt:

```text
CONTEXT_KO.md를 처음부터 끝까지 읽고 저장소를 검사하라. Milestone 0만 시작하라.

초기 repository skeleton, PROJECT_STATUS.md, docs/versions.md,
docs/architecture.md, docs/register-map.md,
docs/adr/0001-record-architecture-decisions.md,
그리고 source-fetch/build script skeleton을 생성하라.

아직 QEMU virtual device 또는 Linux driver를 구현하지 마라.
현재 환경에서 실제로 fetch 및 build할 수 있음을 확인한 뒤에만 candidate upstream version을 고정하라.
Generated source tree는 Git 밖에 유지하라.

작업 종료 시 다음을 보고하라.
1. 변경된 파일,
2. 실제 실행한 명령어,
3. 관찰한 결과,
4. 해결되지 않은 dependency,
5. Milestone 0의 다음 단일 작업.
```

Environment Setup 이후 첫 기술적 성공 기준:

```text
clean checkout → fetch pinned sources → build baseline QEMU/guest → boot Linux → run lspci
```

이 Baseline이 재현 가능해진 뒤에만 Codex가 Minimal Custom PCI-device Enumeration Milestone을 시작해야 한다.

---

## 26. 최종 범위 확인

이 프로젝트는 가장 많은 기능을 누적할 때가 아니라, 규율 있는 **System-software Validation Engineering**을 입증할 때 성공한 것이다.

우선순위:

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

일정 압박이 발생하면 완성된 Vertical Slice를 보존하고 선택 기능의 범위를 줄인다.
