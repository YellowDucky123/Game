#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cctype>
#include <algorithm>
#include "NetworkManager.hpp"
#include "Player.hpp"

enum class GameState {
    WaitingForPlayer,
    SelectingUnit,
    SelectingSkill,
    SelectingTarget,
    SelectingShieldTransfer,
    OpponentTurn
};

std::vector<std::string> order = {
	"Guard",
	"Archer",
	"Farmer",
	"Emperor",
	"Servant",
	"Doctor"
};

// Describes one skill button: how many targets it needs and whose team
// those targets come from (relative to the acting player).
struct SkillInfo {
    std::string name;
    int targetCount;  // 0, 1, or 2
    int targetSide;   // 0 = none, 1 = own team, 2 = enemy team
};

const std::unordered_map<std::string, std::vector<SkillInfo>> SKILL_TABLE = {
    {"Guard",   {{"shield", 2, 1}, {"attack", 1, 2}}},
    {"Archer",  {{"shoot", 2, 2}}},
    {"Farmer",  {{"harvest", 0, 0}, {"heal", 0, 0}}},
    {"Emperor", {{"attack", 1, 2}, {"drain", 0, 0}, {"inheritShield", 0, 0}}},
    {"Servant", {{"attach", 1, 1}}},
    {"Doctor",  {{"healOne", 1, 1}, {"healAll", 0, 0}}}
};

inline const SkillInfo* findSkillInfo(const std::string& unitType, const std::string& skill) {
    auto it = SKILL_TABLE.find(unitType);
    if (it == SKILL_TABLE.end()) return nullptr;
    for (const SkillInfo& info : it->second) {
        if (info.name == skill) return &info;
    }
    return nullptr;
}

// A skill button is clickable when the unit can actually afford/perform it
// this turn and hasn't already used it.
inline bool skillEnabled(Player& player, Unit& unit, const std::string& skill) {
    int cost = unit.skillCost(skill);
    if (cost < 0) return false;
    if (unit.isSkillUsed(skill)) return false;
    if (cost > player.getTT()) return false;
    if (skill == "inheritShield" && !player.canInheritShield()) return false;
    return true;
}

// Short label for a skill button, e.g. "Sh1" for shield costing 1 TT.
inline std::string skillLabel(const std::string& skill, int cost) {
    std::string abbrev = skill.substr(0, 2);
    if (!abbrev.empty()) abbrev[0] = static_cast<char>(std::toupper(abbrev[0]));
    return abbrev + std::to_string(cost);
}

inline sf::Color skillSlotColor(size_t slot) {
    switch (slot) {
        case 0: return sf::Color(34, 139, 34);
        case 1: return sf::Color(70, 130, 180);
        default: return sf::Color(148, 0, 211);
    }
}

// Health bar color ramps from green to yellow to red as HP drops.
inline sf::Color healthColor(float pct) {
    if (pct > 0.6f) return sf::Color(70, 200, 90);
    if (pct > 0.3f) return sf::Color(230, 200, 50);
    return sf::Color(220, 60, 60);
}

const std::string RULES_TEXT =
"GENERAL\n"
"  Each turn you gain +1 Turn Token (TT). TT carries over between turns.\n"
"  Your turn ends when TT reaches 0, or when you click \"End Turn\".\n"
"  Each unit's skill can only be used once per turn.\n"
"  Shields absorb damage before HP, but never regenerate or heal back up.\n"
"\n"
"GUARD\n"
"  Shield (1 TT) - Deploy your 2 shield charges (25 HP each). Pick one ally to\n"
"    shield, then either click \"Finish Shielding\" to stop there, or pick a\n"
"    second target: the same ally combines both charges into a 50 HP shield,\n"
"    a different ally gets a second 25 HP shield. If both charges are already\n"
"    deployed, pick a current holder to revoke first (its remaining shield HP\n"
"    is discarded) before redeploying.\n"
"  Attack (1 TT) - Deal 3 damage to an enemy unit.\n"
"\n"
"ARCHER\n"
"  Shoot (4 TT) - Deal 7 damage to 2 enemy units.\n"
"\n"
"SERVANT\n"
"  Attach (1 TT) - Redirect all damage aimed at the chosen ally to the Servant.\n"
"\n"
"DOCTOR\n"
"  Heal One (1 TT) - Heal one ally for 3 HP.\n"
"  Heal All (8 TT) - Heal every ally for 3 HP.\n"
"\n"
"FARMER\n"
"  Harvest (0 TT) - Drain 2 HP from your Emperor for +3 TT. Ends your turn.\n"
"  Heal (1 TT) - Heal the Farmer and Doctor by 2 HP each. Ends your turn.\n"
"\n"
"EMPEROR\n"
"  Attack (1 TT) - Deal 9 damage to an enemy, but take 7 recoil damage.\n"
"  Drain (0 TT) - Reduce all allies' HP by 2, heal self for 6.\n"
"  Inherit Shield (3 TT) - Once the Guard has died, gain a 25 HP shield for\n"
"    the Emperor (one-time only).\n"
"\n"
"Click anywhere to close.";

inline bool isMyTurnState(GameState s) {
    return s == GameState::SelectingUnit || s == GameState::SelectingSkill ||
           s == GameState::SelectingTarget || s == GameState::SelectingShieldTransfer;
}

inline void playerNameDisplay(std::string name, sf::Text& statusText) {
    static sf::Font font;
    static bool loaded = font.loadFromFile("/mnt/c/Windows/Fonts/arial.ttf");
    if (loaded) {
        statusText.setFont(font);
        statusText.setCharacterSize(24);
        statusText.setFillColor(sf::Color::White);
        statusText.setPosition(20.f, 250.f);
    }
}

inline void runGameLoop(std::string playerName, sf::RenderWindow& window, NetworkManager& net, GameState initialState, sf::Color playerColor, sf::Color enemyColor) {

    GameState currentState = initialState;

    // Each client tracks both teams using the real game logic. As long as
    // both clients apply the same sequence of executeSkill/startTurn calls,
    // myPlayer/enemyPlayer stay mirrored across the network.
    Player myPlayer;
    Player enemyPlayer;

    if (currentState == GameState::SelectingUnit) {
        myPlayer.startTurn();
    } else if (currentState == GameState::OpponentTurn) {
        enemyPlayer.startTurn();
    }

    sf::Font uiFont;
    uiFont.loadFromFile("/mnt/c/Windows/Fonts/arial.ttf");

    static std::vector<sf::Texture> myTextures(6);
    static std::vector<sf::Texture> enemyTextures(6);

    // -----------------------------------------------------------------
    // Graphical Element Layout (6 Across - Single Wide Row Placement)
    // -----------------------------------------------------------------
    std::vector<sf::RectangleShape> myCards(6);

    for (int i = 0; i < 6; ++i) {
        // Base Card - Formatted to sit in 1 flat horizontal line across
        myCards[i].setSize(sf::Vector2f(150.f, 180.f));

        // Horizontal Placement Row Math
        float xPos = 20.f + (i * 165.f); // Spreads perfectly from left to right across the screen
        float yPos = 480.f;              // Locked to the bottom region of the layout
        myCards[i].setPosition(xPos, yPos);

        std::string filename = "images/" + order[i] + ".png";
        if (myTextures[i].loadFromFile(filename)) {
            myCards[i].setTexture(&myTextures[i]);
            myCards[i].setFillColor(sf::Color::White);
        } else {
            myCards[i].setFillColor(playerColor);
        }
    }

    // Opponent Layout - 6 Across along the top
    std::vector<sf::RectangleShape> enemyCards(6);

    for (int i = 0; i < 6; ++i) {
        enemyCards[i].setSize(sf::Vector2f(150.f, 180.f));

        float xPos = 20.f + (i * 165.f);
        float yPos = 60.f; // Locked up near top boundary header
        enemyCards[i].setPosition(xPos, yPos);

        std::string filename = "images/" + order[i] + ".png";
        if (enemyTextures[i].loadFromFile(filename)) {
            enemyCards[i].setTexture(&enemyTextures[i]);
            enemyCards[i].setFillColor(sf::Color::White);
        } else {
            enemyCards[i].setFillColor(enemyColor);
        }
    }

    sf::Text statusText;
    playerNameDisplay(playerName, statusText);

    sf::RectangleShape endTurnButton;
    endTurnButton.setSize(sf::Vector2f(130.f, 45.f));
    endTurnButton.setPosition(850.f, 300.f);
    endTurnButton.setFillColor(sf::Color(170, 40, 40));

    sf::RectangleShape rulesButton;
    rulesButton.setSize(sf::Vector2f(130.f, 45.f));
    rulesButton.setPosition(850.f, 360.f);
    rulesButton.setFillColor(sf::Color(60, 90, 160));

    sf::RectangleShape finishShieldButton;
    finishShieldButton.setSize(sf::Vector2f(130.f, 45.f));
    finishShieldButton.setPosition(850.f, 240.f);
    finishShieldButton.setFillColor(sf::Color(60, 140, 90));

    bool showRules = false;

    int selectedUnitIdx = -1;
    std::string selectedSkill;
    int targetsNeeded = 0;
    int shieldSlotsForCast = 0;
    std::vector<Unit*> pendingTargetUnits;
    std::vector<std::pair<int, int>> pendingTargetCodes;

    // Helper: returns the on-screen rectangle for the skill button "slot"
    // (0-2) beneath the card at index "cardIdx".
    auto skillBoxRect = [](int cardIdx, int slot) {
        float xPos = 20.f + cardIdx * 165.f;
        float yPos = 480.f;
        return sf::FloatRect(xPos + 4.f + slot * 48.f, yPos + 137.f, 44.f, 40.f);
    };

    // Draws a labeled progress bar (used for HP and shield readouts):
    // a dark background track, a colored fill proportional to "pct", and a
    // text label overlaid on top.
    auto drawBar = [&](float x, float y, float w, float h, float pct, sf::Color fillColor, sf::Color bgColor, const std::string& label) {
        sf::RectangleShape bg(sf::Vector2f(w, h));
        bg.setPosition(x, y);
        bg.setFillColor(bgColor);
        window.draw(bg);

        pct = std::max(0.f, std::min(1.f, pct));
        if (pct > 0.f) {
            sf::RectangleShape fill(sf::Vector2f(w * pct, h));
            fill.setPosition(x, y);
            fill.setFillColor(fillColor);
            window.draw(fill);
        }

        sf::Text text;
        text.setFont(uiFont);
        text.setCharacterSize(11);
        text.setFillColor(sf::Color::White);
        text.setString(label);
        text.setPosition(x + 3.f, y - 2.f);
        window.draw(text);
    };

    // Resets the in-progress selection and returns to the unit-picking step.
    auto resetSelection = [&]() {
        selectedUnitIdx = -1;
        selectedSkill.clear();
        targetsNeeded = 0;
        shieldSlotsForCast = 0;
        pendingTargetUnits.clear();
        pendingTargetCodes.clear();
    };

    // Runs the chosen skill locally, ships it to the opponent, then advances
    // to whoever's turn it is next.
    auto executeAndAdvance = [&]() {
        bool ok = myPlayer.executeSkill(selectedSkill, order[selectedUnitIdx], pendingTargetUnits);
        if (ok) {
            net.sendSkillAction(selectedSkill, order[selectedUnitIdx], pendingTargetCodes);
            if (myPlayer.mustEndTurn()) {
                enemyPlayer.startTurn();
                currentState = GameState::OpponentTurn;
            } else {
                currentState = GameState::SelectingUnit;
            }
            selectedUnitIdx = -1;
        } else {
            currentState = GameState::SelectingSkill;
        }
        selectedSkill.clear();
        targetsNeeded = 0;
        shieldSlotsForCast = 0;
        pendingTargetUnits.clear();
        pendingTargetCodes.clear();
    };

    // Ends the local player's turn voluntarily (End Turn button).
    auto endTurnVoluntarily = [&]() {
        enemyPlayer.startTurn();
        net.sendEndTurn();
        currentState = GameState::OpponentTurn;
        resetSelection();
    };

    // -----------------------------------------------------------------
    // Main Window Event Processing
    // -----------------------------------------------------------------
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (currentState == GameState::WaitingForPlayer) continue;

            // Clicking Actions
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));

                if (showRules) {
                    // Any click while the rules are open just closes them.
                    showRules = false;
                }
                else if (rulesButton.getGlobalBounds().contains(mousePosF)) {
                    showRules = true;
                }
                else if (isMyTurnState(currentState) && endTurnButton.getGlobalBounds().contains(mousePosF)) {
                    endTurnVoluntarily();
                }
                else if (currentState == GameState::SelectingTarget && selectedSkill == "shield" &&
                         pendingTargetUnits.size() == 1 && finishShieldButton.getGlobalBounds().contains(mousePosF)) {
                    executeAndAdvance();
                }
                // Step 1: Select Your Unit Card
                else if (currentState == GameState::SelectingUnit) {
                    for (int i = 0; i < 6; ++i) {
                        if (myPlayer.getUnit(order[i]).isAlive() && myCards[i].getGlobalBounds().contains(mousePosF)) {
                            selectedUnitIdx = i;
                            currentState = GameState::SelectingSkill;
                            break;
                        }
                    }
                }
                // Step 2: Select one of the skill boxes beneath the selected unit
                else if (currentState == GameState::SelectingSkill) {
                    Unit& unit = myPlayer.getUnit(order[selectedUnitIdx]);
                    const std::vector<SkillInfo>& skills = SKILL_TABLE.at(order[selectedUnitIdx]);

                    bool handled = false;
                    for (size_t s = 0; s < skills.size() && s < 3; ++s) {
                        if (skillBoxRect(selectedUnitIdx, (int)s).contains(mousePosF)) {
                            handled = true;
                            if (skillEnabled(myPlayer, unit, skills[s].name)) {
                                selectedSkill = skills[s].name;
                                targetsNeeded = skills[s].targetCount;
                                pendingTargetUnits.clear();
                                pendingTargetCodes.clear();
                                if (selectedSkill == "shield") {
                                    if (myPlayer.guardFreeShieldSlots() == 0) {
                                        currentState = GameState::SelectingShieldTransfer;
                                    } else {
                                        shieldSlotsForCast = myPlayer.guardFreeShieldSlots();
                                        currentState = GameState::SelectingTarget;
                                    }
                                } else if (targetsNeeded == 0) {
                                    executeAndAdvance();
                                } else {
                                    currentState = GameState::SelectingTarget;
                                }
                            }
                            break;
                        }
                    }

                    if (!handled) {
                        if (myCards[selectedUnitIdx].getGlobalBounds().contains(mousePosF)) {
                            // Clicked the already-selected card again: deselect.
                            resetSelection();
                            currentState = GameState::SelectingUnit;
                        } else {
                            // Switch to a different unit.
                            for (int i = 0; i < 6; ++i) {
                                if (i != selectedUnitIdx && myPlayer.getUnit(order[i]).isAlive() && myCards[i].getGlobalBounds().contains(mousePosF)) {
                                    selectedUnitIdx = i;
                                    break;
                                }
                            }
                        }
                    }
                }
                // Step 3: Select target(s) for the chosen skill
                else if (currentState == GameState::SelectingTarget) {
                    const SkillInfo* info = findSkillInfo(order[selectedUnitIdx], selectedSkill);

                    if (selectedSkill == "shield") {
                        // Any living ally is a valid shield target: a fresh
                        // one gets a 25 HP charge, the same one picked twice
                        // combines both charges into a 50 HP shield.
                        for (int i = 0; i < 6; ++i) {
                            Unit& target = myPlayer.getUnit(order[i]);
                            if (target.isAlive() && myCards[i].getGlobalBounds().contains(mousePosF)) {
                                pendingTargetUnits.push_back(&target);
                                pendingTargetCodes.push_back({0, i});
                                if (shieldSlotsForCast <= 1 || (int)pendingTargetUnits.size() >= 2) {
                                    executeAndAdvance();
                                }
                                break;
                            }
                        }
                    } else if (info->targetSide == 2) {
                        // Enemy targets. Clicking our own acting card cancels.
                        if (myCards[selectedUnitIdx].getGlobalBounds().contains(mousePosF)) {
                            selectedSkill.clear();
                            targetsNeeded = 0;
                            pendingTargetUnits.clear();
                            pendingTargetCodes.clear();
                            currentState = GameState::SelectingSkill;
                        } else {
                            for (int i = 0; i < 6; ++i) {
                                Unit& target = enemyPlayer.getUnit(order[i]);
                                if (target.isAlive() && enemyCards[i].getGlobalBounds().contains(mousePosF)) {
                                    pendingTargetUnits.push_back(&target);
                                    pendingTargetCodes.push_back({1, i});
                                    if ((int)pendingTargetUnits.size() == targetsNeeded) {
                                        executeAndAdvance();
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        // Own-team targets (attach/healOne).
                        for (int i = 0; i < 6; ++i) {
                            Unit& target = myPlayer.getUnit(order[i]);
                            if (target.isAlive() && myCards[i].getGlobalBounds().contains(mousePosF)) {
                                pendingTargetUnits.push_back(&target);
                                pendingTargetCodes.push_back({0, i});
                                if ((int)pendingTargetUnits.size() == targetsNeeded) {
                                    executeAndAdvance();
                                }
                                break;
                            }
                        }
                    }
                }
                // Step 3b: Guard's both shield charges are deployed - pick a
                // current holder to revoke before redeploying.
                else if (currentState == GameState::SelectingShieldTransfer) {
                    for (int i = 0; i < 6; ++i) {
                        Unit& target = myPlayer.getUnit(order[i]);
                        if (target.getShield() > 0 && myCards[i].getGlobalBounds().contains(mousePosF)) {
                            pendingTargetUnits.push_back(&target);
                            pendingTargetCodes.push_back({0, i});
                            shieldSlotsForCast = myPlayer.guardShieldSlotsHeldBy(&target);
                            currentState = GameState::SelectingTarget;
                            break;
                        }
                    }
                }
            }
        }

        // Sync listener - checked every frame, not just on SFML events,
        // so the state advances even if no window events occur while waiting.
        if (currentState == GameState::WaitingForPlayer) {
            if (net.hasPeer()) {
                if (playerColor == sf::Color::Red) {
                    currentState = GameState::SelectingUnit;
                    myPlayer.startTurn();
                } else {
                    currentState = GameState::OpponentTurn;
                    enemyPlayer.startTurn();
                }
            } else {
                net.updateConnection();
            }
        }

        // Network Sync
        if (currentState == GameState::OpponentTurn && net.hasPeer()) {
            NetworkMessage msg;
            if (net.receiveMessage(msg)) {
                if (msg.type == NetworkMessage::Type::Action) {
                    std::vector<Unit*> targets;
                    for (const std::pair<int, int>& code : msg.targets) {
                        int side = code.first;
                        int idx = code.second;
                        // Flip the sender's perspective into ours: the
                        // sender's own team (side 0) is our enemyPlayer, and
                        // the sender's enemy (side 1, i.e. us) is our myPlayer.
                        Player& team = (side == 1) ? myPlayer : enemyPlayer;
                        targets.push_back(&team.getUnit(order[idx]));
                    }
                    enemyPlayer.executeSkill(msg.skill, msg.card, targets);
                    if (enemyPlayer.mustEndTurn()) {
                        myPlayer.startTurn();
                        currentState = GameState::SelectingUnit;
                    }
                } else {
                    myPlayer.startTurn();
                    currentState = GameState::SelectingUnit;
                }
            }
        }

        if (currentState == GameState::WaitingForPlayer) statusText.setString(playerName + " - Connecting...");
        else if (currentState == GameState::OpponentTurn) statusText.setString(playerName + " - Opponent acting...");
        else if (currentState == GameState::SelectingUnit) statusText.setString(playerName + " - Select a unit");
        else if (currentState == GameState::SelectingSkill) statusText.setString(playerName + " - Pick a skill");
        else if (currentState == GameState::SelectingTarget && selectedSkill == "shield" && pendingTargetUnits.size() == 1)
            statusText.setString(playerName + " - Place a 2nd shield charge, or click Finish Shielding");
        else if (currentState == GameState::SelectingTarget) statusText.setString(playerName + " - Select target for " + selectedSkill);
        else statusText.setString(playerName + " - Both shield charges deployed: pick a unit to revoke one from");

        // -----------------------------------------------------------------
        // Drawing Pipeline
        // -----------------------------------------------------------------
        window.clear(sf::Color(18, 18, 24));

        sf::RectangleShape enemyBand(sf::Vector2f(1000.f, 370.f));
        enemyBand.setPosition(0.f, 0.f);
        enemyBand.setFillColor(sf::Color(42, 28, 32));
        window.draw(enemyBand);

        sf::RectangleShape myBand(sf::Vector2f(1000.f, 330.f));
        myBand.setPosition(0.f, 370.f);
        myBand.setFillColor(sf::Color(26, 38, 30));
        window.draw(myBand);

        // Draw Player Team Rows
        for (int i = 0; i < 6; ++i) {
            Unit& unit = myPlayer.getUnit(order[i]);
            if (!unit.isAlive()) continue;

            if (i == selectedUnitIdx) {
                myCards[i].setOutlineThickness(3.f);
                myCards[i].setOutlineColor(sf::Color::Yellow);
            } else if (currentState == GameState::SelectingUnit) {
                myCards[i].setOutlineThickness(1.f);
                myCards[i].setOutlineColor(sf::Color::Green);
            } else if (currentState == GameState::SelectingTarget) {
                const SkillInfo* info = findSkillInfo(order[selectedUnitIdx], selectedSkill);
                bool valid = info && info->targetSide == 1 && unit.isAlive();
                myCards[i].setOutlineThickness(valid ? 2.f : 0.f);
                myCards[i].setOutlineColor(sf::Color::Cyan);
            } else if (currentState == GameState::SelectingShieldTransfer) {
                bool valid = unit.getShield() > 0;
                myCards[i].setOutlineThickness(valid ? 2.f : 0.f);
                myCards[i].setOutlineColor(sf::Color::Cyan);
            } else {
                myCards[i].setOutlineThickness(0.f);
            }

            window.draw(myCards[i]);

            float xPos = myCards[i].getPosition().x;
            float yPos = myCards[i].getPosition().y;

            // Name plate
            sf::RectangleShape nameBar(sf::Vector2f(150.f, 20.f));
            nameBar.setPosition(xPos, yPos);
            nameBar.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(nameBar);

            sf::Text nameText;
            nameText.setFont(uiFont);
            nameText.setCharacterSize(14);
            nameText.setFillColor(sf::Color::White);
            nameText.setString(order[i]);
            nameText.setPosition(xPos + 4.f, yPos + 1.f);
            window.draw(nameText);

            // HP bar
            float hpPct = (float)unit.getHealth() / (float)unit.getMaxHealth();
            std::string hpLabel = std::to_string(unit.getHealth()) + "/" + std::to_string(unit.getMaxHealth());
            drawBar(xPos, yPos + 108.f, 150.f, 14.f, hpPct, healthColor(hpPct), sf::Color(40, 40, 40), hpLabel);

            // Shield bar (only shown while a shield is active)
            if (unit.getShield() > 0) {
                float shPct = unit.getShield() / 50.f;
                drawBar(xPos, yPos + 124.f, 150.f, 12.f, shPct, sf::Color(80, 160, 230), sf::Color(30, 40, 55), "Shield " + std::to_string(unit.getShield()));
            }

            // Skill buttons, shown only for the currently selected unit.
            if (i == selectedUnitIdx &&
                (currentState == GameState::SelectingSkill ||
                 currentState == GameState::SelectingTarget ||
                 currentState == GameState::SelectingShieldTransfer)) {

                sf::RectangleShape skillBg(sf::Vector2f(150.f, 43.f));
                skillBg.setPosition(xPos, yPos + 136.f);
                skillBg.setFillColor(sf::Color(15, 15, 20, 235));
                window.draw(skillBg);

                const std::vector<SkillInfo>& skills = SKILL_TABLE.at(order[i]);
                for (size_t s = 0; s < skills.size() && s < 3; ++s) {
                    sf::FloatRect rect = skillBoxRect(i, (int)s);
                    sf::RectangleShape box(sf::Vector2f(rect.width, rect.height));
                    box.setPosition(rect.left, rect.top);
                    box.setOutlineThickness(1.f);
                    box.setOutlineColor(sf::Color(20, 20, 20));

                    if (currentState == GameState::SelectingSkill) {
                        bool enabled = skillEnabled(myPlayer, unit, skills[s].name);
                        box.setFillColor(enabled ? skillSlotColor(s) : sf::Color(80, 80, 80));
                    } else {
                        bool chosen = skills[s].name == selectedSkill;
                        box.setFillColor(chosen ? skillSlotColor(s) : sf::Color(50, 50, 50));
                    }
                    window.draw(box);

                    sf::Text label;
                    label.setFont(uiFont);
                    label.setCharacterSize(13);
                    label.setFillColor(sf::Color::White);
                    label.setString(skillLabel(skills[s].name, unit.skillCost(skills[s].name)));
                    label.setPosition(rect.left + 6.f, rect.top + 11.f);
                    window.draw(label);
                }
            }
        }

        // Draw Enemy Team Rows
        for (int i = 0; i < 6; ++i) {
            Unit& unit = enemyPlayer.getUnit(order[i]);
            if (!unit.isAlive()) continue;

            if (currentState == GameState::SelectingTarget) {
                const SkillInfo* info = findSkillInfo(order[selectedUnitIdx], selectedSkill);
                bool valid = info && info->targetSide == 2;
                enemyCards[i].setOutlineThickness(valid ? 2.f : 0.f);
                enemyCards[i].setOutlineColor(sf::Color::Red);
            } else {
                enemyCards[i].setOutlineThickness(0.f);
            }

            window.draw(enemyCards[i]);

            float xPos = enemyCards[i].getPosition().x;
            float yPos = enemyCards[i].getPosition().y;

            // Name plate
            sf::RectangleShape nameBar(sf::Vector2f(150.f, 20.f));
            nameBar.setPosition(xPos, yPos);
            nameBar.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(nameBar);

            sf::Text nameText;
            nameText.setFont(uiFont);
            nameText.setCharacterSize(14);
            nameText.setFillColor(sf::Color::White);
            nameText.setString(order[i]);
            nameText.setPosition(xPos + 4.f, yPos + 1.f);
            window.draw(nameText);

            // HP bar
            float hpPct = (float)unit.getHealth() / (float)unit.getMaxHealth();
            std::string hpLabel = std::to_string(unit.getHealth()) + "/" + std::to_string(unit.getMaxHealth());
            drawBar(xPos, yPos + 108.f, 150.f, 14.f, hpPct, healthColor(hpPct), sf::Color(40, 40, 40), hpLabel);

            // Shield bar (only shown while a shield is active)
            if (unit.getShield() > 0) {
                float shPct = unit.getShield() / 50.f;
                drawBar(xPos, yPos + 124.f, 150.f, 12.f, shPct, sf::Color(80, 160, 230), sf::Color(30, 40, 55), "Shield " + std::to_string(unit.getShield()));
            }
        }

        window.draw(statusText);

        sf::Text ttText;
        ttText.setFont(uiFont);
        ttText.setCharacterSize(20);
        ttText.setFillColor(sf::Color::White);
        ttText.setPosition(20.f, 285.f);
        ttText.setString("TT: " + std::to_string(myPlayer.getTT()));
        window.draw(ttText);

        sf::Vector2f mouseNow(sf::Mouse::getPosition(window));

        if (isMyTurnState(currentState)) {
            bool hover = endTurnButton.getGlobalBounds().contains(mouseNow);
            endTurnButton.setFillColor(hover ? sf::Color(210, 70, 70) : sf::Color(170, 40, 40));
            endTurnButton.setOutlineThickness(2.f);
            endTurnButton.setOutlineColor(sf::Color(255, 180, 180));
            window.draw(endTurnButton);

            sf::Text endText;
            endText.setFont(uiFont);
            endText.setCharacterSize(16);
            endText.setFillColor(sf::Color::White);
            endText.setString("End Turn");
            endText.setPosition(endTurnButton.getPosition().x + 18.f, endTurnButton.getPosition().y + 12.f);
            window.draw(endText);
        }

        // Finish Shielding button: lets the Guard stop after deploying just
        // one of its 2 shield charges.
        if (currentState == GameState::SelectingTarget && selectedSkill == "shield" && pendingTargetUnits.size() == 1) {
            bool hover = finishShieldButton.getGlobalBounds().contains(mouseNow);
            finishShieldButton.setFillColor(hover ? sf::Color(80, 180, 115) : sf::Color(60, 140, 90));
            finishShieldButton.setOutlineThickness(2.f);
            finishShieldButton.setOutlineColor(sf::Color(190, 255, 215));
            window.draw(finishShieldButton);

            sf::Text finishText;
            finishText.setFont(uiFont);
            finishText.setCharacterSize(15);
            finishText.setFillColor(sf::Color::White);
            finishText.setString("Finish");
            finishText.setPosition(finishShieldButton.getPosition().x + 38.f, finishShieldButton.getPosition().y + 6.f);
            window.draw(finishText);

            sf::Text finishText2;
            finishText2.setFont(uiFont);
            finishText2.setCharacterSize(15);
            finishText2.setFillColor(sf::Color::White);
            finishText2.setString("Shielding");
            finishText2.setPosition(finishShieldButton.getPosition().x + 22.f, finishShieldButton.getPosition().y + 23.f);
            window.draw(finishText2);
        }

        // Rules button (always available so either player can check it).
        {
            bool hover = rulesButton.getGlobalBounds().contains(mouseNow);
            rulesButton.setFillColor(hover ? sf::Color(90, 120, 200) : sf::Color(60, 90, 160));
            rulesButton.setOutlineThickness(2.f);
            rulesButton.setOutlineColor(sf::Color(180, 200, 255));
            window.draw(rulesButton);

            sf::Text rulesButtonText;
            rulesButtonText.setFont(uiFont);
            rulesButtonText.setCharacterSize(16);
            rulesButtonText.setFillColor(sf::Color::White);
            rulesButtonText.setString("Rules");
            rulesButtonText.setPosition(rulesButton.getPosition().x + 35.f, rulesButton.getPosition().y + 12.f);
            window.draw(rulesButtonText);
        }

        // Rules overlay, drawn last so it covers everything else.
        if (showRules) {
            sf::RectangleShape overlay;
            overlay.setSize(sf::Vector2f(900.f, 600.f));
            overlay.setPosition(50.f, 50.f);
            overlay.setFillColor(sf::Color(20, 20, 28, 240));
            overlay.setOutlineThickness(2.f);
            overlay.setOutlineColor(sf::Color(180, 200, 255));
            window.draw(overlay);

            sf::Text titleText;
            titleText.setFont(uiFont);
            titleText.setCharacterSize(28);
            titleText.setStyle(sf::Text::Bold);
            titleText.setFillColor(sf::Color(180, 200, 255));
            titleText.setString("Rules");
            titleText.setPosition(70.f, 60.f);
            window.draw(titleText);

            sf::Text rulesText;
            rulesText.setFont(uiFont);
            rulesText.setCharacterSize(16);
            rulesText.setFillColor(sf::Color::White);
            rulesText.setString(RULES_TEXT);
            rulesText.setPosition(70.f, 105.f);
            window.draw(rulesText);
        }

        window.display();
    }
}
