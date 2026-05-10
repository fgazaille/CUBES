# Makefile for CUBES AI Learning Simulation (Raylib Edition)
#
# Usage:
#   make            - Build the simulation
#   make clean      - Remove build artifacts
#   make run        - Build and run the simulation
#   make debug      - Build with debug symbols

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra \
           -Iinclude -Iinclude/core -Iinclude/rendering -Iinclude/menu -Ithird_party

LDFLAGS = -lraylib

# Source files
CORE_SRC = src/core/neural_network.cpp \
           src/core/ai_agent.cpp \
           src/core/environment.cpp

RENDERING_SRC = src/rendering/renderer.cpp

MENU_SRC = src/menu/menu.cpp

MAIN_SRC = src/main.cpp

SRC = $(CORE_SRC) $(RENDERING_SRC) $(MENU_SRC) $(MAIN_SRC)
OBJ = $(SRC:.cpp=.o)
TARGET = simulation

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS += -g -DDEBUG
debug: clean $(TARGET)

.PHONY: all clean run debug
