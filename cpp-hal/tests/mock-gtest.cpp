#include "gtest/gtest.h"
#include <array>
#include <chrono>
#include <cstdint>

#include <gtest/gtest.h>

#include <vnpu/vnpu-hal.hpp>

using namespace std::chrono_literals;

namespace { // code anonymous

class MockVnpuDeviceTest : public ::testing::Test{
	protected:
		MockVnpuDevice dev;
		std::array<const std::int8_t, 8> input_a = {1,2,3,4,5,6,7,8};
		std::array<const std::int8_t, 8> input_b = {8,7,6,5,4,3,2,1};
		std::chrono::milliseconds time_out{100};
		
		uint32_t dot_run_validation(
				std::array<const std::int8_t, 8> a, 
				std::array<const std::int8_t, 8> b)
		{
			uint32_t result = 0;
			for(size_t i=0; i < a.size(); i++){
				result += a[i]*b[i];
			}
			return result;
		}
};

TEST_F(MockVnpuDeviceTest, ReturnDeviceInfo){
	const DeviceInfo info = dev.vnpu_get_info();

	EXPECT_EQ(info.abi_version, 1u);
	EXPECT_EQ(info.device_id, 0x564E5055u);
	EXPECT_EQ(info.revision, 0u);
	EXPECT_EQ(info.capabilities, 0u);
}

TEST_F(MockVnpuDeviceTest, RunDotProduct){

	const DotProductResult result = dev.vnpu_run_dot(input_a, input_b, time_out);

	EXPECT_EQ(result.result, dot_run_validation(input_a, input_b));
	EXPECT_EQ(result.device_error, 0u);
	EXPECT_EQ(result.status, DotProductResult::DRIVER_OK);
}

TEST_F(MockVnpuDeviceTest, DeviceResets){
	DeviceStats before_stats;
	DeviceStats after_stats;
	
	// one reset
	dev.vnpu_reset();
	after_stats = dev.vnpu_get_stats();

	EXPECT_EQ(after_stats.resets, 1u);

	// triple reset
	dev.vnpu_reset();
	dev.vnpu_reset();
	dev.vnpu_reset();
	after_stats = dev.vnpu_get_stats();

	EXPECT_EQ(after_stats.resets, 4u);
}

TEST_F(MockVnpuDeviceTest, SetFaultmaskIRQ){
	DeviceStats a_stats;
	
	dev.vnpu_set_fault(FaultType::irq_drop);
	EXPECT_THROW(dev.vnpu_run_dot(input_a, input_b, time_out), VnpuError);
	a_stats = dev.vnpu_get_stats();
	
	EXPECT_EQ(a_stats.timed_out, 1);
	EXPECT_EQ(a_stats.completed, 0u);
	EXPECT_EQ(a_stats.device_error, 0u);
	EXPECT_EQ(a_stats.resets, 0u);
}

TEST_F(MockVnpuDeviceTest, SetFaultmaskBUSY){
	DeviceStats a_stats;

	dev.vnpu_set_fault(FaultType::stuck_busy);
	EXPECT_THROW(dev.vnpu_run_dot(input_a, input_b, time_out), VnpuError);
	a_stats = dev.vnpu_get_stats();

	EXPECT_EQ(a_stats.submitted, 1);
	EXPECT_EQ(a_stats.timed_out, 1);
	EXPECT_EQ(a_stats.completed, 0u);
	EXPECT_EQ(a_stats.device_error, 0u);
	EXPECT_EQ(a_stats.resets, 0u);

	// dev.vnpu_set_fault(FaultType::none);
	// dev.vnpu_reset();
}

TEST_F(MockVnpuDeviceTest, SetFaultmaskFORCE){
	DeviceStats a_stats;

	dev.vnpu_set_fault(FaultType::force_error);
	dev.vnpu_run_dot(input_a, input_b, time_out);
	a_stats = dev.vnpu_get_stats();

	EXPECT_EQ(a_stats.submitted, 1);
	EXPECT_EQ(a_stats.device_error, 1);
	EXPECT_EQ(a_stats.completed, 0u);
	EXPECT_EQ(a_stats.timed_out, 0u);
	EXPECT_EQ(a_stats.resets, 0u);
	
	// dev.vnpu_set_fault(FaultType::none);
	// dev.vnpu_reset();
}

TEST_F(MockVnpuDeviceTest, SetFaultmaskCORRUPT){
	DeviceStats a_stats;
	DotProductResult result;
	
	dev.vnpu_set_fault(FaultType::corrupt_result);
	result = dev.vnpu_run_dot(input_a, input_b, time_out);
	a_stats = dev.vnpu_get_stats();
	
	EXPECT_NE(result.result, dot_run_validation(input_a, input_b));
	EXPECT_EQ(a_stats.submitted, 1);
	EXPECT_EQ(a_stats.completed, 1);
	EXPECT_EQ(a_stats.device_error, 0u);
	EXPECT_EQ(a_stats.timed_out, 0u);
	EXPECT_EQ(a_stats.resets, 0u);
	
	// dev.vnpu_set_fault(FaultType::none);
	// dev.vnpu_reset();
}

}; //namespace end.



