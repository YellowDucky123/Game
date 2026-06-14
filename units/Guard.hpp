#pragma once

#include "Unit.hpp"
#include <vector>
#include <string>

class Guard : public Unit {
private:
	static const int SHIELD_AMOUNT = 25;

	// The Guard's 2 shield charges. Each entry holds the unit currently
	// carrying that 25 HP charge, or nullptr if the charge is free to
	// deploy. Both charges can point at the same unit, combining to 50 HP.
	std::vector<Unit*> shields = std::vector<Unit*>(2, nullptr);

	public:
		Guard(int health) : Unit(health) {}

		std::string unitType() override {
			return "Guard";
		}

		int skillCost(const std::string& skill) const override {
			if (skill == "shield") return 1;
			if (skill == "attack") return 1;
			return -1;
		}

		// How many of the Guard's 2 charges are still actively protecting a
		// unit (not depleted by damage, revoked, or lost with a dead unit).
		int deployedShields() const {
			int count = 0;
			for (Unit* u : shields) {
				if (u != nullptr && u->isAlive() && u->getShield() > 0) count++;
			}
			return count;
		}

		int freeShieldSlots() const {
			return 2 - deployedShields();
		}

		// How many of the Guard's charges currently sit on "target"
		// (0, 1, or 2 if both were combined onto it for 50 HP).
		int shieldSlotsHeldBy(Unit* target) const {
			int count = 0;
			for (Unit* u : shields) {
				if (u == target) count++;
			}
			return count;
		}

		// Strips whichever charge(s) the Guard has on "target", discarding
		// its remaining shield HP and freeing those slots for redeployment.
		void revokeShield(Unit* target) {
			for (Unit*& slot : shields) {
				if (slot == target) slot = nullptr;
			}
			target->removeShield();
		}

		// Deploys one free charge onto target: 25 HP, or +25 (to 50) if the
		// Guard's other charge is already on the same unit.
		void deployShield(Unit* target) {
			for (Unit*& slot : shields) {
				if (slot == nullptr || !slot->isAlive() || slot->getShield() <= 0) {
					slot = target;
					target->addShield(SHIELD_AMOUNT);
					return;
				}
			}
		}

		void doSkill(std::string skill, std::vector<Unit*> targets) override {
			if (targets.empty() || targets[0] == nullptr) return;
			if (skill == "shield") {
				if (freeShieldSlots() == 0) {
					// Both charges deployed: targets[0] is revoked to make
					// room for any remaining (redeployment) target(s).
					revokeShield(targets[0]);
					for (size_t i = 1; i < targets.size(); ++i) {
						if (targets[i] != nullptr) deployShield(targets[i]);
					}
				} else {
					for (Unit* target : targets) {
						if (target != nullptr) deployShield(target);
					}
				}
			} else if (skill == "attack") {
				targets[0]->updateHealth(3);
			}
		}
};
