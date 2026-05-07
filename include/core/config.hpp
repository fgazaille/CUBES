/**
 * @file config.hpp
 * @author CUBES Project
 * @brief Central configuration file for the AI Learning Simulation.
 * 
 * This file contains all configurable parameters for the simulation including
 * grid dimensions, neural network architecture, learning parameters, and
 * display settings. All constants use constexpr for compile-time optimization.
 */

#pragma once

#include <cstddef>

namespace Config {

// ============================================================================
// Simulation Parameters
// ============================================================================

/** @brief Size of the grid (GRID_SIZE x GRID_SIZE cells) */
constexpr int GRID_SIZE = 15;  // Smaller = easier learning

/** @brief Number of food items initially spawned */
constexpr int FOOD_COUNT = 50;  // More food = less oscillation

/** @brief Number of AI agents in the simulation */
constexpr int AGENT_COUNT = 5;

/** @brief Maximum energy an agent can have */
constexpr int MAX_ENERGY = 200;

/** @brief Energy cost per movement action */
constexpr int ENERGY_DECAY = 1;

/** @brief Energy restored when consuming food */
constexpr int FOOD_ENERGY = 80;

// ============================================================================
// Reinforcement Learning Parameters
// ============================================================================

/** @brief Learning rate for neural network training (typical range: 0.001-0.01) */
constexpr double LEARNING_RATE = 0.05;  // Higher = faster learning

/** @brief Discount factor for future rewards (close to 1 = long-term focus) */
constexpr double DISCOUNT_FACTOR = 0.99;

/** @brief Initial exploration rate (1.0 = 100% random actions) */
constexpr double INITIAL_EXPLORE_RATE = 1.0;

/** @brief Decay rate for exploration (multiplied each step) */
constexpr double EXPLORE_DECAY = 0.995;  // Faster decay: After 1K steps ≈ 0.006, then clamps to 0.1

/** @brief Minimum exploration rate (prevents getting stuck in local optima) */
constexpr double MIN_EXPLORE_RATE = 0.05;  // Lower = more exploitation of learned behaviors

/** @brief Maximum size of the experience replay buffer */
constexpr size_t EXPERIENCE_BUFFER_SIZE = 100000;  // Larger = more stable learning

/** @brief Mini-batch size for neural network training */
constexpr int BATCH_SIZE = 32;

/** @brief Delay between simulation steps (milliseconds) */
constexpr int SLEEP_MS = 1;

/** @brief Number of threads for parallel agent processing */
constexpr int NUM_THREADS = 16;

// ============================================================================
// Neural Network Architecture
// ============================================================================

/**
 * @brief Input layer size
 * 
 * State representation includes:
 * - Agent position (x, y) normalized
 * - Energy level normalized
 * - Distance to closest food
 * - Direction to closest food (4 values: left, right, up, down)
 * - Boundary awareness (4 values: distance from each edge)
 * Total: 2 + 1 + 1 + 4 + 4 = 12
 */
constexpr int INPUT_SIZE = 12;

/** @brief First hidden layer neuron count */
constexpr int HIDDEN_LAYER1_SIZE = 64;  // Larger = more stable learning

/** @brief Second hidden layer neuron count */
constexpr int HIDDEN_LAYER2_SIZE = 32;  // Larger = less forgetting

/** @brief Output layer size (one Q-value per possible action) */
constexpr int OUTPUT_SIZE = 4;

/** @brief Steps between target network updates (for DQN stability) */
constexpr int TARGET_UPDATE_FREQUENCY = 1000;  // Less frequent = more stable

// ============================================================================
// Display Parameters
// ============================================================================

/** @brief Main window width in pixels */
constexpr int SCREEN_WIDTH = 1600;

/** @brief Main window height in pixels */
constexpr int SCREEN_HEIGHT = 900;

/** @brief Width of the sidebar for statistics display */
constexpr int SIDEBAR_WIDTH = 350;

/** @brief Width available for the grid (SCREEN_WIDTH - SIDEBAR_WIDTH) */
constexpr int GRID_WIDTH = SCREEN_WIDTH - SIDEBAR_WIDTH;

/** @brief Height available for the grid (full SCREEN_HEIGHT) */
constexpr int GRID_HEIGHT = SCREEN_HEIGHT;

/**
 * @brief Size of each grid cell in pixels
 * @note Calculated to fit the grid within the available space
 */
constexpr int CELL_SIZE = (GRID_WIDTH < GRID_HEIGHT ? GRID_WIDTH : GRID_HEIGHT) / GRID_SIZE;

// ============================================================================
// Time Warp Parameters
// ============================================================================

/** @brief Whether time warp is enabled by default */
constexpr bool DEFAULT_TIME_WARP_MODE = false;

/** @brief Default time warp speed multiplier (1.0 = normal speed) */
constexpr double DEFAULT_TIME_WARP_FACTOR = 1.0;

} // namespace Config

// Import into global namespace for convenience
using namespace Config;

// ============================================================================
// Runtime Configuration (mutable settings)
// ============================================================================

/**
 * @brief Runtime configuration that can be changed via settings menu.
 * 
 * These values override the constexpr defaults at runtime.
 */
struct RuntimeConfig {
    int grid_size = GRID_SIZE;
    int food_count = FOOD_COUNT;
    int agent_count = AGENT_COUNT;
    int food_energy = FOOD_ENERGY;
    int food_reset_threshold = 10;  ///< Respawn food when count drops below this
    
    static RuntimeConfig& instance() {
        static RuntimeConfig config;
        return config;
    }
};
