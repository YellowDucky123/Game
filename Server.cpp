#include <SFML/Graphics.hpp>
#include "NetworkManager.hpp"
#include "GameEngine.hpp" // Include our brand new shared engine!

int main() {
    sf::RenderWindow window(sf::VideoMode(1000, 700), "LAN Card Game - Server (Host)");
    window.setFramerateLimit(30);

    NetworkManager net;
    net.startServer(12345);

    // Run the shared loop! Server starts at WaitingForPlayer, is Red, enemy is Blue
    runGameLoop("Player 1", window, net, GameState::WaitingForPlayer, sf::Color::Red, sf::Color::Blue);

    return 0;
}
