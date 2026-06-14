#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Doctor : public Unit { 
public:
    Doctor(std::string name, int health, std::vector<std::string> skills) 
        : Unit(name, health, skills) {}
};
