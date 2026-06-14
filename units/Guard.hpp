#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Guard : public Unit {
	public:
		Guard(std::string name, int health, std::vector<std::string> skills) : Unit(name, health, skills) {}

};
