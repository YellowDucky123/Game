#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Emperor : public Unit {
public:
    Emperor(std::string name, int health, std::vector<std::string> skills) 
        : Unit(name, health, skills) {}
};
