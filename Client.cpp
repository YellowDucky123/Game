#include <SFML/Graphics.hpp>
#include "NetworkManager.hpp"
#include "GameEngine.hpp" // Include our brand new shared engine!

int main() {
    sf::RenderWindow window(sf::VideoMode(1000, 700), "LAN Card Game - Client");
    window.setFramerateLimit(30);

    NetworkManager net;
    net.connectToServer("127.0.0.1", 12345);

    // Client waits for the connection to register, then goes second: it is
    // Blue and starts in OpponentTurn while the Red host (Server.cpp) acts first.
    runGameLoop("Player 2", window, net, GameState::WaitingForPlayer, sf::Color::Blue, sf::Color::Red);

    return 0;
}
