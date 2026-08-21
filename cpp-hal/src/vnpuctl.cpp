#include <charconv>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <vnpu/vnpu-hal.hpp>

struct DotInput {
	std::vector<std::int8_t> input_a;
	std::vector<std::int8_t> input_b;
	std::chrono::milliseconds timeout;
};

static void print_usage()
{
	std::cerr
		<< "Usage:\n"
		<< "  vnpuctl info --json\n"
		<< "  vnpuctl run-dot --input input.json --json\n"
		<< "  vnpuctl run-dot --input-a 1,2,3,4,5,6,7,8 --input-b 1,1,1,1,1,1,1,1 --timeout-ms 100 --json\n"
		<< "  vnpuctl inject-fault irq-drop|stuck-busy|corrupt-result|force-error\n"
		<< "  vnpuctl clear-faults\n"
		<< "  vnpuctl reset\n"
		<< "  vnpuctl stats --json\n";
}

static std::string get_option(int argc, char** argv, std::string_view option)
{
	for (int i = 1; i + 1 < argc; ++i) {
		if (std::string_view(argv[i]) == option) {
			return argv[i + 1];
		}
	}

	throw std::runtime_error(std::string("missing option: ") + std::string(option));
}

static std::string get_option_or(int argc, char** argv, std::string_view option, std::string fallback)
{
	for (int i = 1; i + 1 < argc; ++i) {
		if (std::string_view(argv[i]) == option) {
			return argv[i + 1];
		}
	}

	return fallback;
}

static bool has_flag(int argc, char** argv, std::string_view flag)
{
	for (int i = 1; i < argc; i++) {
		if (std::string_view(argv[i]) == flag) {
			return true;
		}
	}
	return false;
}

static int find_command_index(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i) {
		const std::string_view arg(argv[i]);
		if (arg == "--device") {
			++i;
			continue;
		}
		if (!arg.starts_with("--")) {
			return i;
		}
	}

	return -1;
}

static std::string_view trim(std::string_view text)
{
	while (!text.empty() && (text.front() == ' ' || text.front() == '\n' || text.front() == '\t')) {
		text.remove_prefix(1);
	}
	while (!text.empty() && (text.back() == ' ' || text.back() == '\n' || text.back() == '\t')) {
		text.remove_suffix(1);
	}

	return text;
}

static std::vector<std::int8_t> parse_int8_list(std::string_view text)
{
	std::vector<std::int8_t> out;
	while (!text.empty()) {
		const std::size_t comma = text.find(',');
		const std::string_view token = trim(text.substr(0, comma));
		int value = 0;
		const char* begin = token.data();
		const char* end = token.data() + token.size();
		const auto result = std::from_chars(begin, end, value);

		if (result.ec != std::errc() || result.ptr != end) {
			throw std::runtime_error("invalid int8 list");
		}
		if (value < -128 || value > 127) {
			throw std::runtime_error("int8 list value out of range");
		}

		out.push_back(static_cast<std::int8_t>(value));
		if (comma == std::string_view::npos) {
			break;
		}
		text.remove_prefix(comma + 1);
	}

	if (out.size() != 8) {
		throw std::runtime_error("Revision A requires exactly 8 elements");
	}
	return out;
}

static std::vector<std::int8_t> parse_json_int8_array(const std::string& body, std::string_view field)
{
	const std::string key = "\"" + std::string(field) + "\"";
	const std::size_t key_pos = body.find(key);
	if (key_pos == std::string::npos) {
		throw std::runtime_error("missing field: " + std::string(field));
	}

	const std::size_t begin = body.find('[', key_pos);
	const std::size_t end = body.find(']', begin);
	if (begin == std::string::npos || end == std::string::npos || end <= begin) {
		throw std::runtime_error("invalid array field: " + std::string(field));
	}

	return parse_int8_list(std::string_view(body).substr(begin + 1, end - begin - 1));
}

static int parse_json_int(const std::string& body, std::string_view field)
{
	const std::string key = "\"" + std::string(field) + "\"";
	const std::size_t key_pos = body.find(key);
	if (key_pos == std::string::npos) {
		throw std::runtime_error("missing field: " + std::string(field));
	}

	const std::size_t colon = body.find(':', key_pos);
	if (colon == std::string::npos) {
		throw std::runtime_error("invalid integer field: " + std::string(field));
	}

	const std::size_t begin = body.find_first_not_of(" \n\t", colon + 1);
	std::size_t end = body.find_first_of(",}\n\t ", begin);
	if (begin == std::string::npos) {
		throw std::runtime_error("invalid integer field: " + std::string(field));
	}
	if (end == std::string::npos) {
		end = body.size();
	}

	int value = 0;
	const char* first = body.data() + begin;
	const char* last = body.data() + end;
	const auto result = std::from_chars(first, last, value);
	if (result.ec != std::errc() || result.ptr != last) {
		throw std::runtime_error("invalid integer field: " + std::string(field));
	}

	return value;
}

static DotInput parse_run_dot_input_json(const std::string& path)
{
	std::ifstream file(path);
	if (!file) {
		throw std::runtime_error("failed to open: " + path);
	}

	const std::string body(
		(std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	DotInput input;
	input.input_a = parse_json_int8_array(body, "input_a");
	input.input_b = parse_json_int8_array(body, "input_b");

	if (input.input_a.size() != input.input_b.size()) {
		throw std::runtime_error("input_a and input_b length mismatch");
	}

	const int timeout_ms = parse_json_int(body, "timeout_ms");
	if (timeout_ms <= 0) {
		throw std::runtime_error("timeout_ms must be positive");
	}

	input.timeout = std::chrono::milliseconds(timeout_ms);
	return input;
}

static const char* driver_status_to_string(DotProductResult::driver_status status)
{
	switch (status) {
	case DotProductResult::DRIVER_OK:
		return "ok";
	case DotProductResult::DRIVER_TIMEOUT:
		return "timeout";
	case DotProductResult::DRIVER_DEVICE_ERROR:
		return "device_error";
	}

	return "unknown";
}

static FaultType parse_fault_type(std::string_view value)
{
	if (value == "irq-drop") {
		return FaultType::irq_drop;
	}
	if (value == "stuck-busy") {
		return FaultType::stuck_busy;
	}
	if (value == "corrupt-result") {
		return FaultType::corrupt_result;
	}
	if (value == "force-error") {
		return FaultType::force_error;
	}

	throw std::runtime_error("unknown fault type");
}

int main(int argc, char** argv)
{
	try {
		const int command_index = find_command_index(argc, argv);
		if (command_index < 0) {
			print_usage();
			return 1;
		}

		const std::string_view command = argv[command_index];
		const bool json = has_flag(argc, argv, "--json");
		LinuxVnpuDevice dev(get_option_or(argc, argv, "--device", "/dev/vnpu0"));

		if (command == "info") {
			const DeviceInfo info = dev.vnpu_get_info();
			if (json) {
				std::cout
					<< "{\n"
					<< "\t\"abi_version\":" << info.abi_version << ",\n"
					<< "\t\"device_id\":" << info.device_id << ",\n"
					<< "\t\"revision\":" << info.revision << ",\n"
					<< "\t\"capabilities\":" << info.capabilities << "\n"
					<< "}\n";
			} else {
				std::cout
					<< "abi_version=" << info.abi_version << "\n"
					<< "device_id=0x" << std::hex << info.device_id << std::dec << "\n"
					<< "revision=" << info.revision << "\n"
					<< "capabilities=" << info.capabilities << "\n";
			}
			return 0;
		}

		if (command == "run-dot") {
			DotInput input;
			if (has_flag(argc, argv, "--input")) {
				input = parse_run_dot_input_json(get_option(argc, argv, "--input"));
			} else {
				input.input_a = parse_int8_list(get_option(argc, argv, "--input-a"));
				input.input_b = parse_int8_list(get_option(argc, argv, "--input-b"));
				const int timeout_ms = std::stoi(get_option(argc, argv, "--timeout-ms"));
				if (timeout_ms <= 0) {
					throw std::runtime_error("timeout_ms must be positive");
				}
				input.timeout = std::chrono::milliseconds(timeout_ms);
			}

			const DotProductResult dot = dev.vnpu_run_dot(input.input_a, input.input_b, input.timeout);
			if(dot.device_error != 0){
				std::cout << "Run Dot device error!\n";
			}

			if (json) {
				std::cout
					<< "{\n"
					<< "\"result\":" << dot.result << ",\n"
					<< "\"driver_status\":\"" << driver_status_to_string(dot.status) << "\",\n"
					<< "\"device_error\":" << dot.device_error << "\n"
					<< "}\n";
			} else {
				std::cout
					<< "result=" << dot.result << "\n"
					<< "driver_status=" << driver_status_to_string(dot.status) << "\n"
					<< "device_error=" << dot.device_error << "\n";
			}
			return 0;
		}

		if (command == "inject-fault") {
			if (command_index + 1 >= argc) {
				throw std::runtime_error("missing fault type");
			}
			dev.vnpu_set_fault(parse_fault_type(argv[command_index + 1]));
			std::cout << "fault injected\n";
			return 0;
		}

		if (command == "clear-faults") {
			dev.vnpu_set_fault(FaultType::none);
			std::cout << "faults cleared\n";
			return 0;
		}

		if (command == "reset") {
			dev.vnpu_reset();
			std::cout << "device reset\n";
			return 0;
		}

		if (command == "stats") {
			const DeviceStats stats = dev.vnpu_get_stats();
			if (json) {
				std::cout
					<< "{\n"
					<< "\"submitted\":" << stats.submitted << ",\n"
					<< "\"completed\":" << stats.completed << ",\n"
					<< "\"timed_out\":" << stats.timed_out << ",\n"
					<< "\"device_error\":" << stats.device_error << ",\n"
					<< "\"resets\":" << stats.resets << "\n"
					<< "}\n";
			} else {
				std::cout
					<< "submitted=" << stats.submitted << "\n"
					<< "completed=" << stats.completed << "\n"
					<< "timed_out=" << stats.timed_out << "\n"
					<< "device_error=" << stats.device_error << "\n"
					<< "resets=" << stats.resets << "\n";
			}
			return 0;
		}

		print_usage();
		return 1;
	} catch (const std::exception& error) {
		if (has_flag(argc, argv, "--json")) {
			std::cout
				<< "{"
				<< "\"error\":\"" << error.what() << "\""
				<< "}\n";
		} else {
			std::cerr << "error: " << error.what() << "\n";
		}

		return 1;
	}
}
