#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include "NetworkManager.hpp"

enum class GameState { 
    WaitingForPlayer, 
    SelectingUnit, 
    SelectingSkill, 
    SelectingTarget, 
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

struct GameUnit {
    int health = 100;
    int maxHealth = 100;
};

inline void playerNameDisplay(std::string name, sf::Text& statusText) {
    static sf::Font font;
    static bool loaded = font.loadFromFile("/mnt/c/Windows/Fonts/arial.ttf");
    if (loaded) {
        statusText.setFont(font); 
        statusText.setCharacterSize(24); 
        statusText.setFillColor(sf::Color::White);
        statusText.setPosition(250.f, 15.f); 
    }
}

inline void runGameLoop(std::string playerName, sf::RenderWindow& window, NetworkManager& net, GameState initialState, sf::Color playerColor, sf::Color enemyColor) {
    
    GameState currentState = initialState;

    // 6v6 Team Vectors
    std::vector<GameUnit> myUnits(6);
    std::vector<GameUnit> enemyUnits(6);

    sf::Font uiFont;
    uiFont.loadFromFile("/mnt/c/Windows/Fonts/arial.ttf");

    static std::vector<sf::Texture> myTextures(6);
    static std::vector<sf::Texture> enemyTextures(6);

    // -----------------------------------------------------------------
    // Graphical Element Layout (6 Across - Single Wide Row Placement)
    // -----------------------------------------------------------------
    std::vector<sf::RectangleShape> myCards(6);
    std::vector<sf::RectangleShape> mySkillPanels(6);
    std::vector<sf::RectangleShape> mySkillAButtons(6);
    std::vector<sf::RectangleShape> mySkillBButtons(6);

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

        // Sub-panel under frame
        mySkillPanels[i].setSize(sf::Vector2f(90.f, 35.f));
        mySkillPanels[i].setFillColor(sf::Color(60, 50, 40)); 
        mySkillPanels[i].setPosition(xPos, yPos + 110.f);

        // Skill A Mini-box (Left)
        mySkillAButtons[i].setSize(sf::Vector2f(25.f, 22.f));
        mySkillAButtons[i].setFillColor(sf::Color(34, 139, 34)); 
        mySkillAButtons[i].setPosition(xPos + 4.f, yPos + 116.f);

        // Skill B Mini-box (Middle)
        mySkillBButtons[i].setSize(sf::Vector2f(25.f, 22.f));
        mySkillBButtons[i].setFillColor(sf::Color(70, 130, 180)); 
        mySkillBButtons[i].setPosition(xPos + 33.f, yPos + 116.f);
    }

    // Opponent Layout - 6 Across along the top
    std::vector<sf::RectangleShape> enemyCards(6);
    std::vector<sf::RectangleShape> enemySkillPanels(6);

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

        enemySkillPanels[i].setSize(sf::Vector2f(90.f, 30.f));
        enemySkillPanels[i].setFillColor(sf::Color(60, 50, 40));
        enemySkillPanels[i].setPosition(xPos, yPos + 110.f);
    }

    sf::Text statusText;
    playerNameDisplay(playerName, statusText);

    int selectedUnitIdx = -1;
    int selectedSkillIdx = -1;
    int selectedTargetIdx = -1;

    // -----------------------------------------------------------------
    // Main Window Event Processing
    // -----------------------------------------------------------------
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            // Sync listener
            if (currentState == GameState::WaitingForPlayer) {
                if (net.hasPeer()) {
                    if (playerColor == sf::Color::Red) currentState = GameState::SelectingUnit; 
                    else currentState = GameState::OpponentTurn; 
                } else {
                    net.updateConnection(); 
                }
                continue; 
            }

            // Clicking Actions
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));

                // Step 1: Select Your Unit Card
                if (currentState == GameState::SelectingUnit) {
                    for (size_t i = 0; i < myCards.size(); ++i) {
                        if (myUnits[i].health > 0 && myCards[i].getGlobalBounds().contains(mousePosF)) {
                            selectedUnitIdx = i;
                            currentState = GameState::SelectingSkill;
                            break;
                        }
                    }
                }
                // Step 2: Select Skill box attached DIRECTLY beneath the selected unit
                else if (currentState == GameState::SelectingSkill) {
                    if (mySkillAButtons[selectedUnitIdx].getGlobalBounds().contains(mousePosF)) {
                        selectedSkillIdx = 0;
                        currentState = GameState::SelectingTarget;
                    }
                    else if (mySkillBButtons[selectedUnitIdx].getGlobalBounds().contains(mousePosF)) {
                        selectedSkillIdx = 1;
                        currentState = GameState::SelectingTarget;
                    }
                    else {
                        // Let player cancel/switch selection by clicking another unit card
                        for (size_t i = 0; i < myCards.size(); ++i) {
                            if (myUnits[i].health > 0 && myCards[i].getGlobalBounds().contains(mousePosF)) {
                                selectedUnitIdx = i; 
                                break;
                            }
                        }
                    }
                }
                // Step 3: Select Enemy Target Card
                else if (currentState == GameState::SelectingTarget) {
                    for (size_t i = 0; i < enemyCards.size(); ++i) {
                        if (enemyUnits[i].health > 0 && enemyCards[i].getGlobalBounds().contains(mousePosF)) {
                            selectedTargetIdx = i;

                            int damageData = (selectedSkillIdx == 0) ? 30 : 15;
                            
                            enemyUnits[selectedTargetIdx].health -= damageData;
                            if (enemyUnits[selectedTargetIdx].health < 0) enemyUnits[selectedTargetIdx].health = 0;

                            net.sendAction(selectedTargetIdx, damageData);

                            selectedUnitIdx = -1;
                            selectedSkillIdx = -1;
                            selectedTargetIdx = -1;
                            currentState = GameState::OpponentTurn;
                            break;
                        }
                    }
                }
            }
        }

        // Network Sync
        if (currentState == GameState::OpponentTurn && net.hasPeer()) {
            int incomingTargetID, incomingDamageVal;
            if (net.receiveAction(incomingTargetID, incomingDamageVal)) {
                if (incomingTargetID >= 0 && incomingTargetID < 6) {
                    myUnits[incomingTargetID].health -= incomingDamageVal;
                    if (myUnits[incomingTargetID].health < 0) myUnits[incomingTargetID].health = 0;
                }
                selectedUnitIdx = -1;
                selectedSkillIdx = -1;
                selectedTargetIdx = -1; 
                currentState = GameState::SelectingUnit; 
            }
        }

        if (currentState == GameState::WaitingForPlayer) statusText.setString(playerName + " - Connecting...");
        else if (currentState == GameState::OpponentTurn) statusText.setString(playerName + " - Opponent acting...");
        else if (currentState == GameState::SelectingUnit) statusText.setString(playerName + " - Select a Unit (6 Across)");
        else if (currentState == GameState::SelectingSkill) statusText.setString(playerName + " - Pick Skill S1/S2 underneath the card");
        else statusText.setString(playerName + " - Click an Enemy to Attack");

        // -----------------------------------------------------------------
        // Drawing Pipeline
        // -----------------------------------------------------------------
        window.clear(sf::Color(35, 35, 35)); 

        // Draw Player Team Rows
        for (size_t i = 0; i < myCards.size(); ++i) {
            if (myUnits[i].health <= 0) continue; 

            if (currentState == GameState::SelectingUnit) {
                myCards[i].setOutlineThickness(1.f);
                myCards[i].setOutlineColor(sf::Color::Green);
            } else if ((int)i == selectedUnitIdx) {
                myCards[i].setOutlineThickness(3.f);
                myCards[i].setOutlineColor(sf::Color::Yellow);
            } else {
                myCards[i].setOutlineThickness(0.f);
            }

            window.draw(myCards[i]);
            window.draw(mySkillPanels[i]);
            window.draw(mySkillAButtons[i]);
            window.draw(mySkillBButtons[i]);

            sf::Text btnText;
            btnText.setFont(uiFont);
            btnText.setCharacterSize(11);
            btnText.setFillColor(sf::Color::White);

            btnText.setString("S1");
            btnText.setPosition(mySkillAButtons[i].getPosition().x + 6.f, mySkillAButtons[i].getPosition().y + 3.f);
            window.draw(btnText);

            btnText.setString("S2");
            btnText.setPosition(mySkillBButtons[i].getPosition().x + 6.f, mySkillBButtons[i].getPosition().y + 3.f);
            window.draw(btnText);

            // Health counter string mapping inside sub-panel right slot
            sf::Text hpText;
            hpText.setFont(uiFont);
            hpText.setString(std::to_string(myUnits[i].health));
            hpText.setCharacterSize(12);
            hpText.setFillColor(sf::Color(255, 60, 60)); 
            hpText.setPosition(mySkillPanels[i].getPosition().x + 64.f, mySkillPanels[i].getPosition().y + 10.f);
            window.draw(hpText);
        }

        // Draw Enemy Team Rows
        for (size_t i = 0; i < enemyCards.size(); ++i) {
            if (enemyUnits[i].health <= 0) continue;

            if (currentState == GameState::SelectingTarget) {
                enemyCards[i].setOutlineThickness(2.f);
                enemyCards[i].setOutlineColor(sf::Color::Red); 
            } else {
                enemyCards[i].setOutlineThickness(0.f);
            }

            window.draw(enemyCards[i]);
            window.draw(enemySkillPanels[i]);

            sf::Text hpText;
            hpText.setFont(uiFont);
            hpText.setString("HP: " + std::to_string(enemyUnits[i].health));
            hpText.setCharacterSize(12);
            hpText.setFillColor(sf::Color(255, 60, 60));
            hpText.setPosition(enemySkillPanels[i].getPosition().x + 25.f, enemySkillPanels[i].getPosition().y + 7.f);
            window.draw(hpText);
        }

        window.draw(statusText);
        window.display();
    }
}
