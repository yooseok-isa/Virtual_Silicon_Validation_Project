#include <cstdint>
#include <span>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <array>

#include "include/vnpu-hal"
#include "vnpu-uapi.h"

LinuxVnpuDevice::LinuxVnpuDevice(std::string path)
{
	fd_ = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
	if(fd_ < 0){
		throw std::system_error(errno, std::generic_category(), "open error /dev/vnpu0");
	}
}

LinuxVnpuDevice::~LinuxVnpuDevice()
{
	if(fd_ >= 0)
		::close(fd_);
}

DeviceInfo LinuxVnpuDevice::vnpu_get_info() const
{
	struct vnpu_info info = {};

	if(::ioctl(fd_, VNPU_IOCTL_GET_INFO, &info) < 0){
		throw::std::system_error(errno, std::generic_category(), "error IOCTL GET INFO");
	}

	return DeviceInfo{
		.abi_version = info.abi_version,
		.device_id = info.device_id,
	};
}


DotProductResult LinuxVnpuDevice::vnpu_run_dot(std::span<const std::int8_t> input_a, std::span<const std::int8_t> input_b, std::chrono::milliseconds timeout)
{
	struct vnpu_dot_request dot_request = {};
	
	contain_input(input_a, dot_request.input_a);
	contain_input(input_b, dot_request.input_b);
	dot_request.timeout_ms = static_cast<__u32>(timeout.count());


	if(::ioctl(fd_, VNPU_IOCTL_RUN_DOT, &dot_request) < 0){
		throw::std::system_error(errno, std::generic_category(), "error IOCTL RUN DOT");
	}

	return DotProductResult {
		.result = dot_request.result,
		.device_error = dot_request.device_error,
		.status = static_cast<DotProductResult::driver_status>(dot_request.driver_status),
	};
}

void LinuxVnpuDevice::vnpu_reset()
{
	
	if(::ioctl(fd_, VNPU_IOCTL_RESET) < 0){
		throw::std::system_error(errno, std::generic_category(), "error IOCTL RESET");
	}
}

void LinuxVnpuDevice::vnpu_set_fault(FaultType Ftype)
{
	struct vnpu_fault_request fault;

	fault.fault_mask = static_cast<std::uint32_t>(Ftype);
	fault.abi_version = 1;

	if(::ioctl(fd_, VNPU_IOCTL_SET_FAULT, &fault) < 0){
		throw::std::system_error(errno, std::generic_category(), "error IOCTL SET FAULT");
	}

}

DeviceStats LinuxVnpuDevice::vnpu_get_stats() const
{
	struct vnpu_status status;

	if(::ioctl(fd_, VNPU_IOCTL_GET_STAT, &status) < 0){
		throw::std::system_error(errno, std::generic_category(), "error IOCTL GET STAT");
	}


	return DeviceStats{
		.submitted = status.submitted,
		.completed = status.completed,
		.timed_out = status.timed_out,
		.device_error = status.device_error,
		.resets = status.resets,
	};
}

void LinuxVnpuDevice::contain_input(std::span<const std::int8_t> input, std::uint32_t (&packed)[4]){
	 
	std::array<std::int32_t, 4> container;

	for (std::size_t out = 0; out < 4; ++out) {
		const std::size_t in = out * 4;
		container[out] =
			(static_cast<std::int32_t>(static_cast<std::uint8_t>(input[in + 0])) << 0) |
			(static_cast<std::int32_t>(static_cast<std::uint8_t>(input[in + 1])) << 8) |
			(static_cast<std::int32_t>(static_cast<std::uint8_t>(input[in + 2])) << 16) |
			(static_cast<std::int32_t>(static_cast<std::uint8_t>(input[in + 3])) << 24);

		packed[out] = static_cast<std::uint32_t>(container[out]);
		}
}		

