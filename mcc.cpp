#include <algorithm> // std::for_each
#include <chrono>
#include <execution> // std::execution::par
#include <fstream>
#include <iostream>
#include <iomanip> // std::setprecision
#include <memory>
#include <optional>
#include <string> // std::stoi
#include <vector>
#include <windows.h>

#include "json.hpp"


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
	std::shared_ptr<std::vector<int>> m_data;
};


std::optional<std::time_t> check_memory(const std::vector<OneGB>& memory) {
	std::for_each(
		std::execution::seq,
		std::cbegin(memory), 
		std::cend(memory), 
		[](const OneGB& one_gb) {
			const auto event = one_gb.check();
			if (event.has_value())
				return event.value();
		}
	);
	return std::nullopt;
}


void resize_allocated_memory(std::vector<OneGB>& memory, const int gb_reserve) {
	const int free_memory = get_free_memory_gbs(gb_reserve);
	if (free_memory > 0) {
		memory.reserve(get_free_memory_gbs(gb_reserve));
		for (int i = 0; i < free_memory; ++i)
			memory.emplace_back();
	}
	if (free_memory < 0)
		memory.resize(memory.size() + free_memory);
}


std::optional<int> get_memory_reserve_from_args(int argc, char* argv[]) {
	if (argc < 2)
		return std::nullopt;
	try {
		return std::stoi(argv[1]);
	}
	catch (const std::invalid_argument& /*e*/) {
		return std::nullopt;
	}
}


void corrupt_memory(std::vector<OneGB>& memory) {
	if (rand() >100)
		return;
	(*(memory[3].m_data))[5] = 1;
}


int main(int argc, char* argv[]) {
	constexpr int default_memory_gb_reserve = 2;
	const int gb_reserve = get_memory_reserve_from_args(argc, argv).value_or(default_memory_gb_reserve);

	std::vector<OneGB> memory;
	double gb_hours = 0.0;
	auto t0 = std::chrono::high_resolution_clock::now();
	std::vector<time_t> events;

	while (true) {
		resize_allocated_memory(memory, gb_reserve);
		const std::optional<std::time_t> event = check_memory(memory);
		if (event.has_value())
			events.emplace_back(event.value());

		std::cout << std::setprecision(2) << std::fixed << gb_hours << " GB-hours (currently " << memory.size() << "GB allocated)" << "\r" << std::flush;

		auto t1 = std::chrono::high_resolution_clock::now();
		double secs = (std::chrono::duration_cast <std::chrono::milliseconds> (t1 - t0).count()) * 0.001;
		gb_hours += secs / 3600.0 * memory.size();
		t0 = t1;

		nlohmann::json j;
		j["gb_hours"] = gb_hours;
		j["events"] = events;
		std::ofstream o("mcc_state.json");
		o << std::setw(4) << j << std::endl;

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
