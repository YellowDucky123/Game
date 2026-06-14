#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Servant : public Unit {
public:
    Servant(int health)
        : Unit(health) {}

	std::string unitType() override {
		return "Servant";
	}

	int skillCost(const std::string& skill) const override {
		if (skill == "attach") return 1;
		return -1;
	}

	void doSkill(std::string skill, std::vector<Unit*> targets) override {
		if (skill == "attach" && !targets.empty() && targets[0] != nullptr) {
			targets[0]->attachProtector(*this);
		}
	}
};
