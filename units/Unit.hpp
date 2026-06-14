#pragma once
#include <vector>
#include <string>
#include <set>
#include <algorithm>

class Unit {
protected:
    std::string name;
    int health;
    int maxHealth;
    bool alive = true;
    int shieldHP = 0;
    Unit* protector = nullptr;
    std::set<std::string> usedSkills;

public:
    Unit(int _health) : health(_health), maxHealth(_health) {}
    virtual ~Unit() = default;

    // Damage from an attack: redirected to a protector (if any) and absorbed
    // by shield HP before reducing health.
    void updateHealth(int amount) {
        if (amount > 0 && protector != nullptr && protector->isAlive()) {
            protector->updateHealth(amount);
            return;
        }
        if (amount > 0 && shieldHP > 0) {
            int absorbed = std::min(shieldHP, amount);
            shieldHP -= absorbed;
            amount -= absorbed;
        }
        health -= amount;
        if (health <= 0) {
            health = 0;
            alive = false;
        }
    }

    // Self-inflicted health change that bypasses shields and protectors.
    void reduceHealth(int amount) {
        health -= amount;
        if (health <= 0) {
            health = 0;
            alive = false;
        }
    }

    // Healing only ever restores health, never shield HP, and cannot exceed maxHealth.
    void heal(int amount) {
        health = std::min(maxHealth, health + amount);
    }

    // A shield never regenerates: once active it only decreases from damage,
    // and a new shield can only be applied once it's depleted/removed.
    void applyShield(int amount) {
        if (shieldHP <= 0) {
            shieldHP = amount;
        }
    }

    // Adds shield HP, stacking with any shield already present (used by the
    // Guard to combine both of its charges onto one unit for 50 HP).
    void addShield(int amount) {
        shieldHP += amount;
    }

    // Strips an active shield, discarding any remaining shield HP.
    void removeShield() {
        shieldHP = 0;
    }

    void attachProtector(Unit& guardian) {
        if (&guardian != this) {
            protector = &guardian;
        }
    }

    int getHealth() const {
        return health;
    }

    int getMaxHealth() const {
        return maxHealth;
    }

    int getShield() const {
        return shieldHP;
    }

    bool isAlive() const {
        return alive;
    }

    bool isSkillUsed(const std::string& skill) const {
        return usedSkills.count(skill) > 0;
    }

    void markSkillUsed(const std::string& skill) {
        usedSkills.insert(skill);
    }

    void resetSkills() {
        usedSkills.clear();
    }

	virtual std::string unitType() = 0;

	// Returns the TT cost of a skill, or -1 if this unit cannot perform it.
	virtual int skillCost(const std::string& skill) const { return -1; }

	// Whether performing this skill ends the player's turn immediately.
	virtual bool endsTurn(const std::string& skill) const { return false; }

	virtual void doSkill(std::string skill, std::vector<Unit*> targets) {}
};
