#include <csstdint>
#include <span>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "include/vnpu-hal"
#include "linux-driver/vnpu-uapi.h"

class LinuxVnpuDevice : public IVnpuDevice {
	public:
		explicit LinuxVnpuDevice(std::string path = "/dev/vnpu0")
		{
			fd_ = ::open(path.c_str(), O_DRWR | O_CLOEXEC);
			if(fd_ < 0){
				throw std::system_error(errno, std::generic_category(), "open error /dev/vnpu0");
			}
		}

		~LinuxVnpuDevice() override
		{
			if(fd_ >= 0)
				::close(fd_);
		}

		DeviceInfo vnpu_get_info() const override
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

		DotProductResult vnpu_run_dot(std::span<const std::int8_t> input_a, std::span<const std::int8_t> input_b, std::chrono::mileseconds timeout) const override
		{
			struct vnpu_dot_request dot_request = {};
			
			contain_input(input_a, dot_request.input_a);
			contain_input(input_b, dot_request.input_b);
			dot_request.timeout_ms = static_cast<__u32>(timeout);


			if(::ioctl(fd_, VNPU_IOCTL_RUN_DOT, &dot_request) < 0){
				throw::std::system_error(errno, std::generic_category(), "error IOCTL RUN DOT");
			}

			return DotProductResult {
				.result = dot_request.result,
				.device_error = dot_request.device_error,
				.status = dot_request.driver_status,
			};
		}
		
		void vnpu_reset() override {
			
			if(::ioctl(fd_, VNPU_IOCTL_RESET) < 0){
				throw::std::system_error(errno, std::generic_category(), "error IOCTL RESET");
			}
		}

		void vnpu_set_fault(FaultType Ftype) override{
			struct vnpu_fault_request fault;

			fault.fault_mask = Ftype;
			fault.abi_version = 1;

			if(::ioctl(fd_, VNPU_IOCTL_SET_FAULT, &fault) < 0){
				throw::std::system_error(errno, std::generic_category(), "error IOCTL SET FAULT");
			}
		
		}

		DeviceStats vnpu_get_stats() const override {
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
			}
		}

	private:
		void contain_input(std::span<const std::int8_t> input, __u32 (&input)[4]){
			 
			std::array<std::int32_t, 4> container;

			for (std::size_t out = 0; out < 4; ++out) {
				const std::size_t in = out * 4;
				container[out] =
					(static_cast<std::int32_t>(static_cast<std::uint8_t>(input[in + 0])) << 0) |
					(static_cast<std::int32_t>(static_cast<std::uint8_t>(input[in + 1])) << 8) |
					(static_cast<std::int32_t>(static_cast<std::uint8_t>(input[in + 2])) << 16) |
					(static_cast<std::int32_t>(static_cast<std::uint8_t>(input[in + 3])) << 24);

				input[i] = static_cast<__u32>(container);
				}
		}		

}
