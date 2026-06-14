#pragma once
#include <vector>
#include <string> 

class Unit {
    std::string name;
    int health;
    bool alive = true;
    std::vector<std::string> skills;
		
public:
    Unit(std::string _name, int _health, std::vector<std::string> _skills) 
        : name(_name), health(_health), skills(_skills) {}

    void updateHealth(int amount) {
        health -= amount;
        if (health <= 0) {
            alive = false;
        }
    }
}; 
