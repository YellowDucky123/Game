#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Servant : public Unit {
public:
    Servant(std::string name, int health, std::vector<std::string> skills) 
        : Unit(name, health, skills) {}
};
