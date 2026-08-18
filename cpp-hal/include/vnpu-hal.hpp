#pragma once

#include <cstdint>

struct DeviceInfo{
	std::uint32_t abi_version;
	std::uint32_t device_id;
}

struct DotProductResult{
	std::int32_t result;
	std::int32_t device_error;
	enum driver_status{
		DRIVER_OK,
		DRIVER_TIMEOUT,
		DRIVER_ERROR,
	};
}

enum class FaultType : std::uint32_t{
	irq_drop = 0,
	stuck_busy = 1u << 1;
	corrupt_result = 1u << 2;
	force_error = 1u << 3;
};

struct DeviceStats {
	std::uint64_t submitted;
	std::uint64_t completed;
	std::uint64_t timed_out;
	std::uint64_t device_error;
	std::uint64_t resets;
}

class IVnpuDevice {
public:
	virtual void vnpu_init() = 0;
	virtual DeviceInfo vnpu_get_info() = 0;
	virtual DotProductResult vnpu_run_dot() = 0;
	virtual void vnpu_reset() = 0;
	virtual void vnpu_set_fault(FaultType Ftype) = 0;
	virtual DeviceStats vnpu_get_stats() = 0;

	virtual ~IVnpuDevice() = default;
};

