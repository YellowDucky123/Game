#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Farmer : public Unit {
public:
    Farmer(std::string name, int health, std::vector<std::string> skills) 
        : Unit(name, health, skills) {}
};
