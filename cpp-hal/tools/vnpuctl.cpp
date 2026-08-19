#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "include/vnpu-hal"


void print_usage(int num){

	if(num == 1){
		std::cerr
			<<"Usage:\n"
			<< "vnpuctl <command> <input file>"
	}
	else if(num == 2){
		std::cerr
			<<"Usage:\n"
			<< "vnpuctl info --json\n"
			<< "vnpuctl run-dot --input-a <file-or-list> --input-b <file-or-list> --timeout-ms 100 --json\n"
			<< "vnpuctl inject-fault irq-drop\n"
			<< "vnpuctl inject-fault stuck-busy\n"
			<< "vnpuctl inject-fault corrupt-result\n"
			<< "vnpuctl inject-fault force-error\n"
			<< "vnpuctl clear-faults\n"
			<< "vnpuctl reset\n"
			<< "vnpuctl stats --json\n"
	}
}



int main(int argc, char** argv){
	if(argc <2)
		return print_usage(1);

	const std:string_view command = argv[1];
	CliOptions options();


	const bool json = has_flag(argc, argv, "--json");
	LinuxVnpuDevice dev;
	
	if(command == "info"){

		struct DeviceInfo info;

		info = dev.vnpu_get_info();
		
	}

	if(command == "run-dot"){
		struct DotProductResult dot;

		dev.vnpu_run_dot(input_a, input_b, timeout);
	}

	if(command == "inject-fault"){
		uint32_t fault_mask;

		dev.vnpu_set_fault(fault_mask);
	}

	if(command == "clear-fault"){
		uint32_t fault_mask;

		dev.vnpu_set_fault(fault_mask);
	}

	if(command == "reset"){
		dev.vnpu_reset();
	}

	if(command == "stats"){
		struct DeviceStats stats;

		stats = dev.vnpu_get_stats();

	}
}
