#include "State.h"

#include <fstream>
#include <iomanip> // std::setw

#include "json.hpp"

static std::string filename = "mcc_state.json";

State read_state_from_disk() {
	std::ifstream i(filename);
	nlohmann::json j;
	i >> j;
	State s;
	j["gb_hours"].get_to(s.gb_hours);
	j["events"].get_to(s.events);
	return s;
}

void write_state_to_disk(const State& state){
	nlohmann::json j;
	j["gb_hours"] = state.gb_hours;
	j["events"] = state.events;
	std::ofstream o(filename);
	o << std::setw(4) << j << std::endl;
}
