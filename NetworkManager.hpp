#pragma once
#include <SFML/Network.hpp>
#include <iostream>

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

	// Send an action: What unit was used, and how much damage/healing it did
	void sendAction(int unitIndex, int actionAmount) {
		if (!isConnected) return;
		sf::Packet packet;
		packet << unitIndex << actionAmount;
		client.send(packet);
	}

	// Check if the opponent made a move (Returns true if data came in)
	bool receiveAction(int& outUnitIndex, int& outActionAmount) {
		if (!isConnected) return false;
		sf::Packet packet;
		if (client.receive(packet) == sf::Socket::Done) {
			if (packet >> outUnitIndex >> outActionAmount) {
				return true;
			}
		}
		return false;
	}

    bool hasPeer() const { return isConnected; }
};
