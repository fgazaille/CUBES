# Makefile for CUBES AI Learning Simulation
#
# Usage:
#   make            - Build the simulation
#   make clean      - Remove build artifacts
#   make run        - Build and run the simulation
#   make debug      - Build with debug symbols
#
# Requirements:
#   - g++ with C++17 support
#   - SDL2, SDL2_image, SDL2_ttf, SDL2_mixer installed
#   - Headers in /usr/local/include/SDL2
#   - Libraries in /usr/local/lib
#
# Adjust CXXFLAGS and LDFLAGS if your SDL2 is in a different location.

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 \
             -Wall \
             -Wextra \
             -I/usr/local/include/SDL2 \
             -Iinclude \
             -Iinclude/core \
             -Iinclude/rendering \
             -Iinclude/menu \
             -Ithird_party

LDFLAGS = -L/usr/local/lib \
          -lSDL2 \
          -lSDL2_image \
          -lSDL2_ttf \
          -lSDL2_mixer

# Source files
CORE_SRC = src/core/neural_network.cpp \
           src/core/ai_agent.cpp \
           src/core/environment.cpp

RENDERING_SRC = src/rendering/sdl_utils.cpp \
                  src/rendering/renderer.cpp

MENU_SRC = src/menu/menu.cpp

MAIN_SRC = src/main.cpp

SRC = $(CORE_SRC) $(RENDERING_SRC) $(MENU_SRC) $(MAIN_SRC)
OBJ = $(SRC:.cpp=.o)
TARGET = simulation

# Default target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Compile object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Build and run
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Phony targets
.PHONY: all clean run debug
