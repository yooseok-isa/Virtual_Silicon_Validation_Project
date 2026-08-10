# VNPU Userspace UAPI

Status: planned specification, not implemented yet.

This document defines the planned userspace ABI between the Linux VNPU driver
and applications. The UAPI is implemented by the guest Linux kernel driver and
is exposed through a character device.

## Driver Identity

| Item | Value |
|---|---|
| Kernel module | `vnpu_drv` |
| Device node | `/dev/vnpu0` |
| Class name | `vnpu` |
| UAPI header | `driver/vnpu_uapi.h` until a final include layout is chosen |

The driver binds to the QEMU VNPU PCI device:

| Field | Value |
|---|---:|
| PCI vendor ID | `0x1B36` |
| PCI device ID | `0x1000` |
| Supported revision in MVP | Revision A, register value `1` |

## ABI Rules

- All UAPI structures use fixed-width Linux integer types such as `__u32`,
  `__u64`, `__s8`, and `__s32`.
- Every userspace-visible structure includes `abi_version`.
- The initial ABI version is `1`.
- The MVP supports one outstanding operation per device.
- The driver must validate all userspace input before writing MMIO registers.
- The driver must not expose raw MMIO access to userspace.
- The driver must return bounded errors instead of waiting forever.

## Ioctl Commands

The planned ioctl set is:

```c
#define VNPU_IOCTL_GET_INFO   _IOR(VNPU_IOCTL_MAGIC, 0x00, struct vnpu_info)
#define VNPU_IOCTL_RUN_DOT    _IOWR(VNPU_IOCTL_MAGIC, 0x01, struct vnpu_dot_request)
#define VNPU_IOCTL_RESET      _IO(VNPU_IOCTL_MAGIC, 0x02)
#define VNPU_IOCTL_SET_FAULT  _IOW(VNPU_IOCTL_MAGIC, 0x03, struct vnpu_fault_request)
#define VNPU_IOCTL_GET_STATS  _IOR(VNPU_IOCTL_MAGIC, 0x04, struct vnpu_stats)
```

The exact `VNPU_IOCTL_MAGIC` value will be selected in `driver/vnpu_uapi.h`
before implementation. Once driver code exists, changing command numbers or
structure layout requires a UAPI documentation update and ABI compatibility
reasoning.

## Data Structures

### `struct vnpu_info`

```c
struct vnpu_info {
    __u32 abi_version;
    __u32 device_id;
    __u32 revision;
    __u32 capabilities;
};
```

Returned by `VNPU_IOCTL_GET_INFO`.

| Field | Meaning |
|---|---|
| `abi_version` | UAPI version returned by the driver |
| `device_id` | MMIO `DEVICE_ID`, expected `0x564E5055` |
| `revision` | MMIO `REVISION`; MVP accepts `1` |
| `capabilities` | MMIO `CAPABILITIES`; MVP expects `0` |

### `struct vnpu_dot_request`

```c
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
```

Used by `VNPU_IOCTL_RUN_DOT`.

| Field | Direction | Meaning |
|---|---|---|
| `abi_version` | in | Must be `1` |
| `job_id` | in | Reserved for Revision B; ignored for Revision A |
| `vector_length` | in | Revision A supports `8` only |
| `timeout_ms` | in | Maximum wait time for completion |
| `input_a` | in | Signed INT8 vector A |
| `input_b` | in | Signed INT8 vector B |
| `result` | out | Signed INT32 dot-product result |
| `driver_status` | out | Driver-level result code |
| `device_error` | out | MMIO `ERROR_CODE` on device failure |

For Revision A, only `input_a[0..7]` and `input_b[0..7]` are consumed.
Userspace should zero the remaining elements for deterministic logs.

### `struct vnpu_fault_request`

```c
struct vnpu_fault_request {
    __u32 abi_version;
    __u32 fault_mask;
};
```

Used by `VNPU_IOCTL_SET_FAULT`.

| Bit | Name | Meaning |
|---:|---|---|
| 0 | `VNPU_FAULT_IRQ_DROP` | Complete without raising an IRQ |
| 1 | `VNPU_FAULT_STUCK_BUSY` | Leave device busy until reset |
| 2 | `VNPU_FAULT_CORRUPT_RESULT` | Deterministically alter the result |
| 3 | `VNPU_FAULT_FORCE_ERROR` | Complete with device error |

The device model accepts only one active fault bit. If multiple bits are set,
the lowest-numbered bit is selected by the device.

### `struct vnpu_stats`

```c
struct vnpu_stats {
    __u32 abi_version;
    __u64 submitted;
    __u64 completed;
    __u64 timed_out;
    __u64 device_errors;
    __u64 resets;
};
```

Returned by `VNPU_IOCTL_GET_STATS`.

| Field | Meaning |
|---|---|
| `submitted` | Number of accepted `RUN_DOT` submissions |
| `completed` | Number of successful completions |
| `timed_out` | Number of timeout paths |
| `device_errors` | Number of device-reported failures |
| `resets` | Number of driver-initiated resets |

## Driver Status Values

`driver_status` in `struct vnpu_dot_request` reports driver-level completion.
The exact numeric values will be defined in `driver/vnpu_uapi.h`; the planned
semantic set is:

| Name | Meaning |
|---|---|
| `VNPU_STATUS_OK` | Operation completed and `result` is valid |
| `VNPU_STATUS_TIMEOUT` | Completion did not arrive before `timeout_ms` |
| `VNPU_STATUS_DEVICE_ERROR` | Device entered `STATUS_ERROR` |
| `VNPU_STATUS_INVALID_ARGUMENT` | Userspace request failed validation |
| `VNPU_STATUS_BUSY` | Another operation is already active |
| `VNPU_STATUS_UNSUPPORTED` | Requested revision, length, or feature is unsupported |

Kernel syscall return values still use normal Linux error codes. For example,
invalid userspace pointers return `-EFAULT`, bad ABI versions return `-EINVAL`,
and unsupported operations return `-EOPNOTSUPP` or `-ENODEV` depending on the
failure point.

## `VNPU_IOCTL_GET_INFO`

Purpose: return stable device identity and UAPI version.

Driver behavior:

1. Validate the userspace pointer.
2. Read cached probe-time identity values.
3. Copy `struct vnpu_info` to userspace.

This ioctl must not start hardware work.

## `VNPU_IOCTL_RUN_DOT`

Purpose: submit one signed INT8 dot-product operation and wait for completion.

Driver behavior:

1. Copy `struct vnpu_dot_request` from userspace.
2. Validate `abi_version`.
3. Validate `vector_length` against the probed hardware revision.
4. Validate `timeout_ms` is nonzero and within the driver maximum.
5. Acquire the device submission mutex.
6. Clear stale completion state.
7. Program input registers and `VECTOR_LENGTH`.
8. Enable completion/error IRQs.
9. Write `CONTROL_START`.
10. Wait for IRQ-driven completion with timeout.
11. On success, read `RESULT` and clear/acknowledge IRQ status.
12. On timeout or stuck busy, issue `CONTROL_RESET` and report timeout.
13. Copy the updated request back to userspace.

The MVP supports one active request at a time.

## `VNPU_IOCTL_RESET`

Purpose: reset runtime device state.

Driver behavior:

1. Acquire the device submission mutex.
2. Write `CONTROL_RESET`.
3. Clear driver completion state.
4. Increment reset stats.

This ioctl is valid even after timeout or device error.

## `VNPU_IOCTL_SET_FAULT`

Purpose: configure deterministic runtime fault injection for validation.

Driver behavior:

1. Copy `struct vnpu_fault_request` from userspace.
2. Validate `abi_version`.
3. Mask unsupported bits.
4. Write `FAULT_CONTROL`.

This ioctl is test-only behavior and must not be used to imply production
hardware behavior.

## `VNPU_IOCTL_GET_STATS`

Purpose: return driver-maintained counters for validation and debugging.

Driver behavior:

1. Snapshot counters safely.
2. Copy `struct vnpu_stats` to userspace.

Stats are diagnostic counters, not a stable performance interface.

## Safety Requirements

The driver must:

- validate `abi_version`;
- use `copy_from_user()` and `copy_to_user()`;
- never dereference userspace pointers directly;
- validate vector length before MMIO writes;
- reject unsupported revision values during probe;
- protect command submission with a mutex;
- use a completion object or wait queue for IRQ-driven completion;
- acknowledge `IRQ_STATUS` using RW1C semantics;
- avoid sleeping in the IRQ handler;
- recover or return a clear error after timeout;
- unwind resources consistently in probe failure paths and `remove()`.

## Compatibility Policy

Before the first driver implementation, this document may be refined. After
`driver/vnpu_uapi.h` and userspace tests exist, changes to ioctl numbers,
structure layout, field meaning, or ABI version handling require:

- an update to this document;
- matching driver and userspace test changes;
- a short ADR if compatibility or reproducibility is affected.
