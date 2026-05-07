# CUBES - AI Learning Simulation

A C++ reinforcement learning simulation where AI agents learn to navigate a grid world, collect food, and manage energy using Deep Q-Networks (DQN).

## Features

- **Neural Network AI**: Agents use feedforward neural networks (64/32 hidden neurons) with ReLU activation
- **Reinforcement Learning**: DQN with experience replay (100K buffer), target networks, and adaptive learning rates
- **Monotonic Improvement**: Agents never die (energy ≥ 1), preventing oscillation between generations
- **Best Brain Preservation**: Automatically saves `brain_best.json` when improvement detected
- **Real-time Visualization**: SDL2-based renderer with grid, agents, and statistics
- **Multi-threaded Training**: Parallel processing with progress tracking to `training_progress.txt`
- **Smart Resource Management**: RAII smart pointers and cached textures for performance

## Building

### Prerequisites

**macOS:**
```bash
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer cmake
```

**Linux:**
```bash
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev cmake g++
```

### Compilation

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
```

### Installation

```bash
sudo make install  # Installs to /usr/local/bin/simulation
```

## Usage

### Running the Simulation

```bash
./simulation
```

### Training Mode (Multi-threaded)

From the menu, select "Training" to start parallel training:
- **Episodes**: Number of training cycles (1000-100000)
- **Threads**: Parallel training threads (1-16)
- **Auto-save**: Automatically saves best brain to `brain.json`
- **Load Brain**: Start from existing `brain.json` if available

Training progress is saved to `training_progress.txt` and summary to `training_summary.txt`.

Press **Ctrl+C** to stop training early.

### Controls

**Simulation:**
- **ESC**: Quit
- **SPACE**: Pause/Resume (or step in debug mode)
- **R**: Reset simulation
- **D**: Toggle debug mode (click agents to inspect)
- **S**: Save brain of first alive agent to `brain.json`
- **L**: Load brain for all agents from `brain.json`
- **F5**: Save full simulation state
- **F9**: Load full simulation state
- **+/-**: Adjust time warp speed
- **0**: Reset time warp to 1x
- **W**: Toggle time warp mode

## Configuration

Edit `include/core/config.hpp` to adjust:

### Simulation Parameters
- `GRID_SIZE`: Grid dimensions (default: 15x15)
- `FOOD_COUNT`: Food items spawned (default: 50)
- `AGENT_COUNT`: Number of AI agents (default: 5)
- `MAX_ENERGY`: Maximum agent energy (default: 100)

### Neural Network Architecture
- `INPUT_SIZE`: 12 (position, energy, food info, boundaries)
- `HIDDEN_LAYER1_SIZE`: 64 neurons (default)
- `HIDDEN_LAYER2_SIZE`: 32 neurons (default)
- `OUTPUT_SIZE`: 4 (LEFT, RIGHT, UP, DOWN)

### Learning Parameters
- `LEARNING_RATE`: 0.01 (adaptive: lower when exploring)
- `DISCOUNT_FACTOR`: 0.99 (future reward importance)
- `INITIAL_EXPLORE_RATE`: 1.0 (start fully random)
- `EXPORE_DECAY`: 0.9995 (slow decay)
- `MIN_EXPLORE_RATE`: 0.1 (always explore a bit)
- `EXPERIENCE_BUFFER_SIZE`: 100000 (replay memory)

## Project Structure

```
CUBES/
├── include/
│   ├── core/           # Neural network, AI agent, environment
│   ├── rendering/      # SDL utilities, renderer
│   └── menu/           # Menu system
├── src/
│   ├── core/           # Implementation of core modules
│   ├── rendering/      # Rendering implementation
│   └── menu/           # Menu implementation
├── third_party/        # Third-party headers (json.hpp, BS_thread_pool.hpp)
├── assets/             # Textures (Player.png, Food.png) and fonts
├── tests/              # Unit tests (test_neural_network.cpp)
├── build/              # Build directory (gitignored)
├── brain.json          # Saved brain (auto-generated)
├── brain_best.json     # Best brain (auto-generated)
├── training_progress.txt # Training progress log
└── training_summary.txt  # Training summary
```

## Documentation

To generate Doxygen documentation:
```bash
doxygen Doxyfile
```

## Recent Improvements

- **Fixed oscillation**: Agents never die, brains preserved across resets
- **Increased network size**: 64/32 hidden neurons for better learning
- **Adaptive learning rate**: Higher when exploiting, lower when exploring
- **Stronger rewards**: +20 for food, -20 for "death" (now prevented)
- **Text caching**: SDL text rendering optimization
- **Training progress**: Real-time progress tracking and summary
- **Signal handling**: Ctrl+C stops training gracefully

## License

MIT License - See LICENSE file for details.
