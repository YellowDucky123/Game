# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -std=c++17

# SFML Libraries to link against (Order matters!)
LIBS = -lsfml-graphics -lsfml-window -lsfml-network -lsfml-system

# Target executables
all: server client

HEADERS = NetworkManager.hpp GameEngine.hpp Player.hpp \
          units/Unit.hpp units/Guard.hpp units/Archer.hpp units/Farmer.hpp \
          units/Emperor.hpp units/Servant.hpp units/Doctor.hpp

# Compile the server
server: Server.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) Server.cpp -o server $(LIBS)

# Compile the client
client: Client.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) Client.cpp -o client $(LIBS)

# Clean up the compiled files
clean:
	rm -f server client
