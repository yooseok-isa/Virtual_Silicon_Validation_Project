#pragma once

#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <stdexcept>
#include <exception>

#define ERR_MISMATCH 0
#define ERR_VEC_LEN 1
#define ERR_TIMEOUT_CNT 2


struct DeviceInfo {
	std::uint32_t abi_version;
	std::uint32_t device_id;
	std::uint32_t revision;
	std::uint32_t capabilities;
};

struct DotProductResult {
	enum driver_status {
		DRIVER_OK = 0,
		DRIVER_TIMEOUT = 1,
		DRIVER_DEVICE_ERROR = 2,
		DRIVER_INV_REV,
		DRIVER_INV_LEN,
		DRIVER_INV_TIME,
		DRIVER_INV_MULTIBIT,
		DRIVER_INV_BIT,
	};

	std::int32_t result;
	std::int32_t device_error;
	driver_status status;
};

enum class FaultType : std::uint32_t {
	none = 0,
	irq_drop = 1u << 0,
	stuck_busy = 1u << 1,
	corrupt_result = 1u << 2,
	force_error = 1u << 3,
};

struct DeviceStats {
	std::uint64_t submitted;
	std::uint64_t completed;
	std::uint64_t timed_out;
	std::uint64_t device_error;
	std::uint64_t resets;
};

enum class VnpuErrorType{
	validation_error,
	system_error,
	device_error,
	driver_error,
	internal_error,
};

class VnpuError : public std::runtime_error{
	public:
		VnpuError(
				VnpuErrorType type,
				std::string message,
				int errnum = 0,
				std::uint32_t device_error = 0)
		: std::runtime_error(message),
		type_(type),
		errnum_(errnum),
		device_error_(device_error){}

	VnpuErrorType type() const {
		return type_;
	}

	int errnum() const {
		return errnum_;
	}

	std::uint32_t device_error() const {
		return device_error_;
	}

	private:
		VnpuErrorType type_;
		int errnum_;
		std::uint32_t device_error_;
};


class IVnpuDevice {
public:
	virtual DeviceInfo vnpu_get_info() const = 0;
	virtual DotProductResult vnpu_run_dot(
		std::span<const std::int8_t> input_a,
		std::span<const std::int8_t> input_b,
		std::chrono::milliseconds timeout) = 0;
	virtual void vnpu_reset() = 0;
	virtual void vnpu_set_fault(FaultType type) = 0;
	virtual DeviceStats vnpu_get_stats() const = 0;

	virtual ~IVnpuDevice() = default;
};

class LinuxVnpuDevice : public IVnpuDevice {
public:
	explicit LinuxVnpuDevice(std::string path = "/dev/vnpu0");
	~LinuxVnpuDevice() override;

	LinuxVnpuDevice(const LinuxVnpuDevice&) = delete;
	LinuxVnpuDevice& operator=(const LinuxVnpuDevice&) = delete;

	DeviceInfo vnpu_get_info() const override;
	DotProductResult vnpu_run_dot(
		std::span<const std::int8_t> input_a,
		std::span<const std::int8_t> input_b,
		std::chrono::milliseconds timeout) override;
	void vnpu_reset() override;
	void vnpu_set_fault(FaultType type) override;
	DeviceStats vnpu_get_stats() const override;

private:
	int fd_ = -1;

	static void pack_int8_input(
		std::span<const std::int8_t> input,
		std::uint32_t (&packed)[4]);
};

class MockVnpuDevice : public IVnpuDevice {
public:
	explicit MockVnpuDevice(std::string path = "/dev/vnpu0");
	~MockVnpuDevice() override = default;

	MockVnpuDevice(const LinuxVnpuDevice&) = delete;
	MockVnpuDevice& operator=(const LinuxVnpuDevice&) = delete;

	DeviceInfo vnpu_get_info() const override;
	DotProductResult vnpu_run_dot(
		std::span<const std::int8_t> input_a,
		std::span<const std::int8_t> input_b,
		std::chrono::milliseconds timeout) override;
	void vnpu_reset() override;
	void vnpu_set_fault(FaultType type) override;
	DeviceStats vnpu_get_stats() const override;

private:
	int fd_ = -1;

	static void pack_int8_input(
		std::span<const std::int8_t> input,
		std::uint32_t (&packed)[4]);

	static int32_t dot_product(
			std::span<const std::int8_t> input_a, 
			std::span<const std::int8_t> input_b);

};

inline const char* to_string(VnpuErrorType type) {
	switch (type) {
	case VnpuErrorType::validation_error:
	  return "validation_error";
	case VnpuErrorType::system_error:
	  return "system_error";
	case VnpuErrorType::device_error:
	  return "device_error";
	case VnpuErrorType::driver_error:
	  return "driver_error";
	case VnpuErrorType::internal_error:
	  return "internal_error";
	}

	return "internal_error";
}

