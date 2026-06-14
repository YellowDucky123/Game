# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -std=c++17

# SFML Libraries to link against (Order matters!)
LIBS = -lsfml-graphics -lsfml-window -lsfml-network -lsfml-system

# Target executables
all: server client

# Compile the server
server: Server.cpp NetworkManager.hpp
	$(CXX) $(CXXFLAGS) Server.cpp -o server $(LIBS)

# Compile the client
client: Client.cpp NetworkManager.hpp
	$(CXX) $(CXXFLAGS) Client.cpp -o client $(LIBS)

# Clean up the compiled files
clean:
	rm -f server client
