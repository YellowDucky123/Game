#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Archer : public Unit {
public:
    Archer(int health) 
        : Unit(health) {}

	std::string unitType() override {
		return "Archer";
	}

	int skillCost(const std::string& skill) const override {
		if (skill == "shoot") return 4;
		return -1;
	}

	void doSkill(std::string skill, std::vector<Unit*> targets) override {
		if (skill == "shoot") {
			for (Unit* target : targets) {
				if (target != nullptr) {
					target->updateHealth(7);
				}
			}
		}
	}
};
