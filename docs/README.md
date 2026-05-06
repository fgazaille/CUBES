# CUBES - AI Learning Simulation (Refactored)

A C++ simulation where AI agents learn to collect food using Deep Q-Networks (DQN) and genetic algorithms, with real-time visualization.

## Features

- **Deep Q-Network (DQN)**: Agents use a 3-layer neural network for Q-value approximation
- **Experience Replay**: Stabilizes training by breaking correlations in consecutive experiences
- **Target Network**: Separate network for stable Q-value targets
- **Genetic Evolution**: When agents die, top performers create the next generation through selection, crossover, and mutation
- **Real-time Visualization**: SDL2-based rendering with:
  - Grid world with food and agents
  - Statistics sidebar
  - Neural network visualizer
  - Time warp for faster training
- **Multi-threading**: Thread pool for parallel agent processing

## Project Structure

```
clean/
├── CMakeLists.txt              # CMake build configuration
├── Makefile                   # Make build configuration
├── include/                   # Header files
│   ├── core/                 # Core simulation logic
│   │   ├── config.hpp        # All configuration constants (documented)
│   │   ├── experience.hpp    # Experience replay struct
│   │   ├── neural_network.hpp # Neural network (documented)
│   │   ├── ai_agent.hpp     # AI agent class (documented)
│   │   └── environment.hpp  # Simulation environment (documented)
│   ├── rendering/            # SDL rendering logic
│   │   ├── sdl_utils.hpp    # SDL init/cleanup (documented)
│   │   ├── renderer.hpp     # Main rendering (documented)
│   │   └── brain_visualizer.hpp # Brain viz (documented)
│   ├── SDL2/                # SDL2 headers (third-party)
│   ├── json.hpp             # nlohmann JSON library (third-party)
│   └── BS_thread_pool.hpp   # Thread pool library (third-party)
├── src/                      # Implementation files
│   ├── core/
│   │   ├── neural_network.cpp
│   │   ├── ai_agent.cpp
│   │   └── environment.cpp
│   ├── rendering/
│   │   ├── sdl_utils.cpp
│   │   ├── renderer.cpp
│   │   └── brain_visualizer.cpp
│   ├── experiments/         # Old experimental versions (v0.1-v0.5)
│   └── main.cpp             # Entry point (documented)
├── assets/                   # Game assets
│   ├── Player.png           # Agent sprite (32x32 recommended)
│   ├── Food.png             # Food sprite (32x32 recommended)
│   └── Futura.ttf          # Font file
├── data/                     # Save data
│   └── brain.json          # Example saved brain
└── docs/
    └── README.md            # This file
```

## Building

### Using CMake (Recommended)

```bash
mkdir build && cd build
cmake ..
make
```

### Using Make

```bash
make
```

### Using Debug Build

```bash
make debug    # Adds -g flag for debugging
```

## Running

```bash
./simulation
# or with Make:
make run
```

## Controls

| Key | Action |
|-----|--------|
| `ESC` | Exit the simulation |
| `R` | Reset simulation (start new generation) |
| `Space` | Pause/Resume simulation |
| `W` | Toggle time warp mode |
| `+` / `=` | Increase time warp speed |
| `-` | Decrease time warp speed |
| `0` | Reset warp speed to normal (1x) |
| `B` | Toggle brain visualizer window |
| `Mouse` | Click "Show Brain" button to open brain viz |

## Configuration

All configuration is in `include/core/config.hpp`. Key parameters:

```cpp
// Simulation
constexpr int GRID_SIZE = 20;         // Grid dimensions
constexpr int FOOD_COUNT = 30;         // Food items
constexpr int AGENT_COUNT = 5;         // Number of agents

// Neural Network
constexpr int INPUT_SIZE = 12;         // State vector size
constexpr int HIDDEN_LAYER1_SIZE = 32; // Neurons in first hidden layer
constexpr int HIDDEN_LAYER2_SIZE = 16; // Neurons in second hidden layer
constexpr int OUTPUT_SIZE = 4;         // One per action (LEFT, RIGHT, UP, DOWN)

// Learning
constexpr double LEARNING_RATE = 0.001;   // Neural network learning rate
constexpr double DISCOUNT_FACTOR = 0.99;   // Future reward importance
constexpr double EXPLORE_DECAY = 0.9997;  // Exploration decay per step
```

## How It Works

### 1. State Representation (12 values)
- Agent position (x, y) normalized
- Energy level normalized
- Distance to closest food
- Direction to closest food (4 values)
- Boundary awareness (4 values)

### 2. Action Selection (Epsilon-Greedy)
- With probability epsilon: explore (random action, biased toward food)
- With probability 1-epsilon: exploit (highest Q-value)

### 3. Reward Function
- Base penalty for moving: `-ENERGY_DECAY * 0.1`
- Large reward for eating food: `+food_value * 0.5`
- Proximity reward for moving toward food
- Penalty for hitting walls: `-0.2`
- Penalty for dying: `-1.0`

### 4. Learning (DQN)
1. Store experience (state, action, reward, next_state, done)
2. Sample mini-batch from replay buffer
3. Compute target Q-values using target network
4. Train policy network with MSE loss
5. Periodically update target network

### 5. Evolution (When All Agents Die)
1. Select top 50% as parents
2. Keep top performers unchanged (elitism)
3. Create offspring via crossover and mutation
4. Reset positions and energy

## Dependencies

- **SDL2** - Core multimedia library
- **SDL2_image** - Image loading (PNG support)
- **SDL2_ttf** - TrueType font rendering
- **SDL2_mixer** - Audio support
- **C++17** - Language standard (for `std::optional`, `std::variant`, etc.)
- **CMake 3.10+** (optional, for CMake build)

### Installing Dependencies (macOS)

```bash
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer
```

### Installing Dependencies (Ubuntu/Debian)

```bash
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

## Code Documentation

All code is documented with Doxygen-style comments. To generate documentation:

```bash
# Install Doxygen
brew install doxygen   # macOS
sudo apt-get install doxygen   # Ubuntu

# Generate docs
doxygen -g Doxyfile   # Generate config
doxygen Doxyfile       # Build documentation
```

## License

[Add your license here]

## Author

CUBES Project

## Version History

- **v2.0** (Refactored): Modular structure, comprehensive documentation, best practices
- **v1.0** (Original): Monolithic implementation with multiple experimental versions
