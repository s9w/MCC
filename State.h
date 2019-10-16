#pragma once

#include <ctime>
#include <string>
#include <vector>


struct State {
	double gb_hours = 0;
	std::vector<std::pair<std::time_t, double>> events = {};
};

[[nodiscard]] State read_state_from_disk(const std::string& computer_name);
void write_state_to_disk(const State& state, const std::string& computer_name);
