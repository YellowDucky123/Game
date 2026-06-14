#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Farmer : public Unit {
public:
    Farmer(int health)
        : Unit(health) {}

	std::string unitType() override {
		return "Farmer";
	}

	int skillCost(const std::string& skill) const override {
		if (skill == "harvest") return 0;
		if (skill == "heal") return 1;
		return -1;
	}

	bool endsTurn(const std::string& skill) const override {
		return skill == "harvest" || skill == "heal";
	}

	void doSkill(std::string skill, std::vector<Unit*> targets) override {
		if (skill == "harvest" && !targets.empty() && targets[0] != nullptr) {
			targets[0]->reduceHealth(2);
		} else if (skill == "heal") {
			heal(2);
			for (Unit* ally : targets) {
				if (ally != nullptr) {
					ally->heal(2);
				}
			}
		}
	}
};
