# CUBES - AI Learning Simulation

A C++ reinforcement learning simulation where AI agents learn to navigate a grid world, collect food, and manage energy using Deep Q-Networks (DQN).

## Features

- **Neural Network AI**: Agents use feedforward neural networks (64/32 hidden neurons) with ReLU activation
- **Reinforcement Learning**: DQN with experience replay (100K buffer), target networks, and adaptive learning rates
- **Monotonic Improvement**: Agents never die (energy ≥ 1), preventing oscillation between generations
- **Best Brain Preservation**: Automatically saves `assets/brain.json` when improvement detected
- **Real-time Visualization**: Raylib-based renderer with resizable window, grid, agents, and statistics
- **Multi-threaded Training**: Parallel processing with progress tracking
- **Resizable Window**: Dynamic layout adapts to any window size
- **Modern UI**: Dark theme with smooth animations and raygui-based menus

## Building

### Prerequisites

**macOS:**
```bash
brew install raylib
```

**Linux:**
```bash
sudo apt install libraylib-dev cmake g++
```

### Compilation

```bash
make -j$(nproc)
```

or with CMake:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
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
- **Auto-save**: Automatically saves best brain to `assets/brain.json`
- **Load Brain**: Start from existing `assets/brain.json` if available

Training progress is saved to `training_progress.txt`.

Press **Ctrl+C** to stop training early.

### Controls

**Simulation:**
- **ESC**: Quit
- **SPACE**: Pause/Resume (or step in debug mode)
- **R**: Reset simulation
- **D**: Toggle debug mode (click agents to inspect)
- **S**: Save brain of first alive agent to `assets/brain.json`
- **L**: Load brain for all agents from `assets/brain.json`
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
│   ├── rendering/      # Raylib renderer
│   └── menu/           # Menu system (raygui)
├── src/
│   ├── core/           # Implementation of core modules
│   ├── rendering/      # Rendering implementation
│   └── menu/           # Menu implementation
├── third_party/        # Third-party headers (json.hpp, BS_thread_pool.hpp, raygui.h)
├── assets/             # Textures (Player.png, Food.png) and fonts
├── tests/              # Unit tests (test_neural_network.cpp)
├── docs/               # Documentation and known issues
├── training_progress.txt # Training progress log
└── training_summary.txt  # Training summary
```

## Version 2.0 Changes

- **Migrated from SDL2 to Raylib**: Cleaner API, simpler build, modern rendering
- **Resizable window**: Dynamic layout adapts to any size
- **Modern UI**: Raygui-based menus with dark theme
- **Improved visuals**: Rounded corners, smooth animations, cleaner agent rendering
- **Fixed brain save path**: Now uses `assets/brain.json` consistently

## License

MIT License - See LICENSE file for details.
