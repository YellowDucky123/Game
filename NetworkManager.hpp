#pragma once
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

// A single network message exchanged between the two clients.
// Action: the sender performed a skill; targets are (side, index) pairs
//         relative to the sender (side 0 = sender's own team, 1 = sender's enemy).
// EndTurn: the sender chose to end their turn voluntarily.
struct NetworkMessage {
    enum class Type { Action, EndTurn };

    Type type = Type::EndTurn;
    std::string skill;
    std::string card;
    std::vector<std::pair<int, int>> targets;
};

class NetworkManager {
private:
    sf::TcpListener listener;
    sf::TcpSocket client;
    bool isConnected;
    bool isServer;

public:
    NetworkManager() : isConnected(false), isServer(false) {}

    // Initialize as Server/Host
    bool startServer(unsigned short port) {
        isServer = true;
        if (listener.listen(port) != sf::Socket::Done) {
            std::cerr << "Failed to bind to port " << port << std::endl;
            return false;
        }
        listener.setBlocking(false); // Non-blocking listener
        std::cout << "Server listening on port " << port << "..." << std::endl;
        return true;
    }

    // Initialize as Client
    bool connectToServer(const sf::IpAddress& ip, unsigned short port) {
        isServer = false;
        std::cout << "Connecting to " << ip << "..." << std::endl;
        if (client.connect(ip, port) == sf::Socket::Done) {
            client.setBlocking(false); // Non-blocking communication
            isConnected = true;
            std::cout << "Connected successfully!" << std::endl;
            return true;
        }
        return false;
    }

    // Keep checking for a connection (Only used by Server)
    void updateConnection() {
        if (isServer && !isConnected) {
            if (listener.accept(client) == sf::Socket::Done) {
                client.setBlocking(false);
                isConnected = true;
                std::cout << "Player 2 joined the match!" << std::endl;
            }
        }
    }

    // Send a skill action: which skill, which of the sender's cards used it,
    // and the (side, index) targets it was used on (relative to the sender).
    void sendSkillAction(const std::string& skill, const std::string& card, const std::vector<std::pair<int, int>>& targets) {
        if (!isConnected) return;
        sf::Packet packet;
        packet << static_cast<sf::Int32>(NetworkMessage::Type::Action);
        packet << skill << card;
        packet << static_cast<sf::Uint32>(targets.size());
        for (const auto& target : targets) {
            packet << static_cast<sf::Int32>(target.first) << static_cast<sf::Int32>(target.second);
        }
        client.send(packet);
    }

    // Send a notification that the sender voluntarily ended their turn.
    void sendEndTurn() {
        if (!isConnected) return;
        sf::Packet packet;
        packet << static_cast<sf::Int32>(NetworkMessage::Type::EndTurn);
        client.send(packet);
    }

    // Check if the opponent sent a message (Returns true if data came in)
    bool receiveMessage(NetworkMessage& out) {
        if (!isConnected) return false;
        sf::Packet packet;
        if (client.receive(packet) != sf::Socket::Done) return false;

        sf::Int32 typeTag;
        if (!(packet >> typeTag)) return false;

        if (typeTag == static_cast<sf::Int32>(NetworkMessage::Type::Action)) {
            out.type = NetworkMessage::Type::Action;
            sf::Uint32 count;
            if (!(packet >> out.skill >> out.card >> count)) return false;
            out.targets.clear();
            for (sf::Uint32 i = 0; i < count; ++i) {
                sf::Int32 side, index;
                if (!(packet >> side >> index)) return false;
                out.targets.emplace_back(side, index);
            }
        } else {
            out.type = NetworkMessage::Type::EndTurn;
            out.skill.clear();
            out.card.clear();
            out.targets.clear();
        }
        return true;
    }

    bool hasPeer() const { return isConnected; }
};
