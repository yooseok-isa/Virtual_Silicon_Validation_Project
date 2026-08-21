#include <cerrno>
#include <chrono>
#include <cstdint>
#include <fcntl.h>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <sys/ioctl.h>
#include <unistd.h>

#include <vnpu/vnpu-hal.hpp>

#include "vnpu_uapi.h"

LinuxVnpuDevice::LinuxVnpuDevice(std::string path)
{
	fd_ = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
	if (fd_ < 0) {
		throw std::system_error(errno, std::generic_category(), "open /dev/vnpu0");
	}
}

LinuxVnpuDevice::~LinuxVnpuDevice()
{
	if (fd_ >= 0) {
		::close(fd_);
	}
}

DeviceInfo LinuxVnpuDevice::vnpu_get_info() const
{
	struct vnpu_info info = {};

	if (::ioctl(fd_, VNPU_IOCTL_GET_INFO, &info) < 0) {
		throw std::system_error(errno, std::generic_category(), "ioctl GET_INFO");
	}

	return DeviceInfo {
		.abi_version = info.abi_version,
		.device_id = info.device_id,
		.revision = 0,
		.capabilities = 0,
	};
}

DotProductResult LinuxVnpuDevice::vnpu_run_dot(
	std::span<const std::int8_t> input_a,
	std::span<const std::int8_t> input_b,
	std::chrono::milliseconds timeout)
{
	struct vnpu_dot_request dot_request = {};

	if (input_a.size() != input_b.size()) {
		throw std::invalid_argument("input_a and input_b length mismatch");
	}
	if (input_a.size() != 8) {
		throw std::invalid_argument("Revision A requires vector length 8");
	}
	if (timeout.count() <= 0) {
		throw std::invalid_argument("timeout must be positive");
	}

	pack_int8_input(input_a, dot_request.input_a);
	pack_int8_input(input_b, dot_request.input_b);
	dot_request.abi_version = 1;
	dot_request.vector_length = static_cast<__u32>(input_a.size());
	dot_request.timeout_ms = static_cast<__u32>(timeout.count());

	if (::ioctl(fd_, VNPU_IOCTL_RUN_DOT, &dot_request) < 0) {
		throw std::system_error(errno, std::generic_category(), "ioctl RUN_DOT");
	}

	return DotProductResult {
		.result = dot_request.result,
		.device_error = static_cast<std::int32_t>(dot_request.device_error),
		.status = static_cast<DotProductResult::driver_status>(dot_request.driver_status),
	};
}

void LinuxVnpuDevice::vnpu_reset()
{
	if (::ioctl(fd_, VNPU_IOCTL_RESET) < 0) {
		throw std::system_error(errno, std::generic_category(), "ioctl RESET");
	}
}

void LinuxVnpuDevice::vnpu_set_fault(FaultType type)
{
	struct vnpu_fault_request fault = {};

	fault.abi_version = 1;
	fault.fault_mask = static_cast<std::uint32_t>(type);

	if (::ioctl(fd_, VNPU_IOCTL_SET_FAULT, &fault) < 0) {
		throw std::system_error(errno, std::generic_category(), "ioctl SET_FAULT");
	}
}

DeviceStats LinuxVnpuDevice::vnpu_get_stats() const
{
	struct vnpu_status status = {};

	if (::ioctl(fd_, VNPU_IOCTL_GET_STAT, &status) < 0) {
		throw std::system_error(errno, std::generic_category(), "ioctl GET_STAT");
	}

	return DeviceStats {
		.submitted = status.submitted,
		.completed = status.completed,
		.timed_out = status.timed_out,
		.device_error = status.device_error,
		.resets = status.resets,
	};
}

void LinuxVnpuDevice::pack_int8_input(
	std::span<const std::int8_t> input,
	std::uint32_t (&packed)[4])
{
	for (std::uint32_t& word : packed) {
		word = 0;
	}

	const std::size_t word_count = (input.size() + 3) / 4;
	for (std::size_t out = 0; out < word_count; ++out) {
		const std::size_t in = out * 4;
		std::uint32_t word = 0;

		for (std::size_t byte = 0; byte < 4 && in + byte < input.size(); ++byte) {
			word |= static_cast<std::uint32_t>(
				static_cast<std::uint8_t>(input[in + byte])) << (byte * 8);
		}

		packed[out] = word;
	}
}
