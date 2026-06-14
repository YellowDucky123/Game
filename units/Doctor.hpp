#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Doctor : public Unit {
public:
    Doctor(int health)
        : Unit(health) {}

	std::string unitType() override {
		return "Doctor";
	}

	int skillCost(const std::string& skill) const override {
		if (skill == "healOne") return 1;
		if (skill == "healAll") return 8;
		return -1;
	}

	void doSkill(std::string skill, std::vector<Unit*> targets) override {
		if (skill == "healOne" && !targets.empty() && targets[0] != nullptr) {
			targets[0]->heal(3);
		} else if (skill == "healAll") {
			for (Unit* ally : targets) {
				if (ally != nullptr) {
					ally->heal(3);
				}
			}
		}
	}
};
