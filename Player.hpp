#pragma once

#include "units/Guard.hpp"
#include "units/Emperor.hpp"
#include "units/Servant.hpp"
#include "units/Doctor.hpp"
#include "units/Archer.hpp"
#include "units/Farmer.hpp"
#include <string>
#include <vector>

class Player {
	int TT = 0;
	bool forceEndTurn = false;
	Emperor emperor{60};
	Guard guard{25};
	Servant servant{17};
	Doctor doctor{34};
	Farmer farmer{28};
	Archer archer{15};

	std::vector<Unit*> allUnits() {
		return {&emperor, &guard, &servant, &doctor, &farmer, &archer};
	}

public:
	Unit& getUnit(const std::string& card) {
		if (card == "Emperor") return emperor;
		if (card == "Guard") return guard;
		if (card == "Servant") return servant;
		if (card == "Doctor") return doctor;
		if (card == "Farmer") return farmer;
		return archer;
	}

	// How many of the Guard's 2 shield charges are currently unused and
	// available to deploy via the "shield" skill.
	int guardFreeShieldSlots() {
		return guard.freeShieldSlots();
	}

	// How many of the Guard's charges currently sit on "target" (0, 1, or 2).
	int guardShieldSlotsHeldBy(Unit* target) {
		return guard.shieldSlotsHeldBy(target);
	}

	// Whether the Emperor is currently eligible to inherit the Guard's shield.
	bool canInheritShield() {
		return !guard.isAlive() && !emperor.hasInheritedShield();
	}

	int getTT() const {
		return TT;
	}

	// Called at the start of this player's turn: gains 1 TT (TT carries
	// over between turns) and refreshes every unit's once-per-turn skills.
	void startTurn() {
		TT += 1;
		forceEndTurn = false;
		for (Unit* unit : {(Unit*)&emperor, (Unit*)&guard, (Unit*)&servant, (Unit*)&doctor, (Unit*)&farmer, (Unit*)&archer}) {
			unit->resetSkills();
		}
	}

	// A turn must end once TT is exhausted, a turn-ending skill was used,
	// or the player may also end early.
	bool mustEndTurn() const {
		return TT == 0 || forceEndTurn;
	}

	bool executeSkill(std::string skill, std::string myCard, std::vector<Unit*> targets) {
		Unit& unit = getUnit(myCard);

		if (unit.isSkillUsed(skill)) return false;

		int cost = unit.skillCost(skill);
		if (cost < 0 || cost > TT) return false;

		if (skill == "inheritShield" && (guard.isAlive() || emperor.hasInheritedShield())) return false;

		// The Guard has 2 shield charges (25 HP each) to deploy with this
		// skill - onto up to 2 allies, or combined onto one for 50 HP. If
		// both charges are already deployed, the first target must be one
		// of the current holders, freeing its charge(s) (remaining shield
		// HP simply discarded) to make room for any redeployment target.
		if (skill == "shield") {
			if (targets.empty() || targets.size() > 2) return false;
			for (Unit* t : targets) {
				if (t == nullptr) return false;
			}

			if (guard.freeShieldSlots() == 0) {
				if (targets[0]->getShield() <= 0) return false;
				int freed = guard.shieldSlotsHeldBy(targets[0]);
				if ((int)targets.size() - 1 > freed) return false;
			} else if ((int)targets.size() > guard.freeShieldSlots()) {
				return false;
			}
		}

		if (skill == "drain") {
			targets = {(Unit*)&guard, (Unit*)&servant, (Unit*)&doctor, (Unit*)&farmer, (Unit*)&archer};
		}

		// Farmer's "harvest" drains the player's own Emperor for +3TT.
		if (skill == "harvest") {
			targets = {(Unit*)&emperor};
		}

		// Farmer's "heal" restores itself and the Doctor.
		if (skill == "heal" && myCard == "Farmer") {
			targets = {(Unit*)&doctor};
		}

		// Doctor's "healAll" restores every unit on the team.
		if (skill == "healAll") {
			targets = {(Unit*)&emperor, (Unit*)&guard, (Unit*)&servant, (Unit*)&doctor, (Unit*)&farmer, (Unit*)&archer};
		}

		unit.doSkill(skill, targets);
		unit.markSkillUsed(skill);
		TT -= cost;

		if (skill == "harvest") {
			TT += 3;
		}

		if (unit.endsTurn(skill)) {
			forceEndTurn = true;
		}

		return true;
	}
};
