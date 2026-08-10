
#include <linux/types.h>
#include <linux/ioctl.h>

#define VNPU_IOCTL_MAGIC 'VNPU'

#define VNPU_IOCTL_GET_INFO __IOR(VNPU_IOCTL_MAGIC, 0x00, struct vnpu_info)


struct vnpu_info {
	__u32 abi_version;
	__u32 device_id;
//	__u32 revision; // reserved
//	__u32 capabilities; // reserved
};

struct vnpu_dot_request {
	__u32 abi_version;
	__u32_job_id;
	__u32 vector_length;
	__u32 timeout_ms;
	__s8 input_a[16];
	__s8 input_b[16];
	__s32 result;
	__s32 driver_status;
	__u32 device_error;
};

struct vnpu_fault_reques {
	__u32 abi_version;
	__u32 fault_mask;
};

struct vnpu_status {
	__u32 abi_version;
	__u64 submitted;
	__u64 completed;
	__u64 timed_out;
	__u64 device_errors;
	__u64 resets;
};
