#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Archer : public Unit {
public:
    Archer(std::string name, int health, std::vector<std::string> skills) 
        : Unit(name, health, skills) {}
};
