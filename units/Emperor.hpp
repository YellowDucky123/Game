#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Emperor : public Unit {
	bool shieldInherited = false;

public:
    Emperor(int health)
        : Unit(health) {}

	std::string unitType() override {
		return "Emperor";
	}

	// True once the Emperor has inherited the fallen Guard's shield.
	bool hasInheritedShield() const {
		return shieldInherited;
	}

	int skillCost(const std::string& skill) const override {
		if (skill == "attack") return 1;
		if (skill == "drain") return 0;
		if (skill == "inheritShield") return 3;
		return -1;
	}

	void doSkill(std::string skill, std::vector<Unit*> targets) override {
		if (skill == "attack" && !targets.empty() && targets[0] != nullptr) {
			targets[0]->updateHealth(9);
			reduceHealth(7);
		} else if (skill == "drain") {
			for (Unit* ally : targets) {
				if (ally != nullptr) {
					ally->reduceHealth(2);
				}
			}
			heal(6);
		} else if (skill == "inheritShield") {
			shieldInherited = true;
			applyShield(25);
		}
	}
};
