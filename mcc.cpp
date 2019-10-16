#include <algorithm> // std::for_each
#include <chrono>
#include <execution> // std::execution::par
#include <iostream>
#include <iomanip> // std::setprecision
#include <memory>
#include <optional>
#include <string> // std::stoi
#include <windows.h>

#pragma warning(push) // supressing warnings in external code
#pragma warning( disable : 26444 )
#pragma warning( disable : 26451 )
#pragma warning( disable : 26812 )
#include "CLI11.hpp"
#pragma warning(pop)

#include "State.h"

#define CSI "\x1b["

template <class T>
constexpr double bytes_to_gb(const T byte_count) {
	return byte_count / 1024.0 / 1024.0 / 1024.0;
}


double get_free_physical_gb() {
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return bytes_to_gb(status.ullAvailPhys);
}


int get_free_memory_gbs(const int gb_reserve) {
	const double free_memory_in_gb = get_free_physical_gb();
	const int gb_to_fill = static_cast<int>(std::floor(free_memory_in_gb)) - gb_reserve;
	return gb_to_fill;
}


template <class T>
[[nodiscard]]
T get_sum(const std::vector<T>& vec) {
	T sum = 0;
	for (const T e : vec)
		sum += e;
	return sum;
}

constexpr size_t n_four_megabyte = 1024 * 1024;
constexpr size_t n_1gb = 256 * n_four_megabyte;

std::time_t get_current_local_tm() {
	time_t rawtime;
	time(&rawtime);
	return rawtime;
}

struct OneGB {
	OneGB() : m_data(std::make_unique<std::vector<int>>(n_1gb, 0)) {}
	~OneGB() {}
	OneGB(OneGB&& ref) noexcept {
		m_data = ref.m_data;
		ref.m_data = nullptr;
	}
	std::optional<std::time_t> check() const {
		if (get_sum(*m_data) != 0) {
			std::cout << "call hawking" << std::endl;
			return get_current_local_tm();
		}
		return std::nullopt;
	}
	void fix() {
		std::fill(std::begin(*m_data), std::end(*m_data), 0);
	}
	std::shared_ptr<std::vector<int>> m_data;
};


[[nodiscard]]
std::optional<std::time_t> check_memory(const std::vector<OneGB>& memory) {
	std::optional<std::time_t> event_catcher;
	std::for_each(
		std::execution::seq,
		std::cbegin(memory), 
		std::cend(memory), 
		[&](const OneGB& one_gb) {
			const auto event = one_gb.check();
			if (event.has_value())
				event_catcher = event.value();
		}
	);
	return event_catcher;
}


void fix_memory(std::vector<OneGB>& memory) {
	std::for_each(
		std::execution::seq,
		std::begin(memory),
		std::end(memory),
		[&](OneGB& one_gb) {
			one_gb.fix();
		}
	);
}


void resize_allocated_memory(std::vector<OneGB>& memory, const int gb_reserve) {
	const int free_memory = get_free_memory_gbs(gb_reserve);
	if (free_memory > 0) {
		memory.reserve(get_free_memory_gbs(gb_reserve));
		
		for (int i = 0; i < free_memory; ++i) {
			printf(CSI "K"); // clear the line
			std::cout << "\r";
			std::cout << "allocating memory... (" << i+1 << "/" << free_memory << "GB)" << std::flush;
			memory.emplace_back();
		}
	}
	if (free_memory < 0)
		memory.resize(memory.size() + free_memory);
}


void corrupt_memory(std::vector<OneGB>& memory) {
	if (rand() >100)
		return;
	(*(memory[0].m_data))[0] = 1;
}


void print_state(const State& state, const int memory_size) {
	printf(CSI "K"); // clear the line
	std::cout << "\r";
	std::cout << std::setprecision(2) << std::fixed << state.gb_hours;
	std::cout << " GB-hours (currently " << memory_size << "GB allocated)" << std::flush;
}


/// <summary>Self-explanatory, but doesn't seem to work on powershell</summary>
void set_console_cursor_visibility(bool visibility){
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO     cursorInfo;
	GetConsoleCursorInfo(out, &cursorInfo);
	cursorInfo.bVisible = visibility; // set the cursor visibility
	cursorInfo.dwSize = 100;
	SetConsoleCursorInfo(out, &cursorInfo);
}


int main(int argc, char* argv[]) {
	system(" "); // Start VT100 support
	set_console_cursor_visibility(false);

	TCHAR computer_name[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(computer_name) / sizeof(computer_name[0]);
	GetComputerName(computer_name, &size);

	if (argc == 1)
		std::cout << "run MCC.exe --help for options" << std::endl;
	CLI::App mcc_args{ "MCC scans for cosmic rays" };

	int gb_reserve = 3;
	mcc_args.add_option("-r,--reserve", gb_reserve, "Number if GB to keep in reserve. Default is 3");

	CLI11_PARSE(mcc_args, argc, argv);

	State state = read_state_from_disk(computer_name);
	std::vector<OneGB> memory;
	auto t0 = std::chrono::high_resolution_clock::now();

	while (true) {
		resize_allocated_memory(memory, gb_reserve);
		const std::optional<std::time_t> event = check_memory(memory);
		if (event.has_value()) {
			state.events.emplace_back(event.value(), state.gb_hours);
			fix_memory(memory);
		}

		print_state(state, memory.size());

		auto t1 = std::chrono::high_resolution_clock::now();
		double secs = (std::chrono::duration_cast <std::chrono::milliseconds> (t1 - t0).count()) * 0.001;
		state.gb_hours += secs / 3600.0 * memory.size();
		t0 = t1;

		write_state_to_disk(state, computer_name);

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
