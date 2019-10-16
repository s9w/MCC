#pragma once

#include <ctime>

#include <vector>



struct State {
	double gb_hours = 0;
	std::vector<std::pair<std::time_t, double>> events = {};
};

State read_state_from_disk();
void write_state_to_disk(const State& state);
