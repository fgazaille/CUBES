/**
 * @file config.hpp
 * @brief Central configuration for the AI Learning Simulation.
 */

#pragma once

#include <cstddef>
#include <algorithm>
#include <string>
#include <fstream>

namespace Config {

// ============================================================================
// Simulation Parameters
// ============================================================================

constexpr int GRID_SIZE = 15;
constexpr int FOOD_COUNT = 50;
constexpr int AGENT_COUNT = 5;
constexpr int MAX_ENERGY = 200;
constexpr int ENERGY_DECAY = 1;
constexpr int FOOD_ENERGY = 80;

// ============================================================================
// Reinforcement Learning Parameters
// ============================================================================

constexpr double LEARNING_RATE = 0.05;
constexpr double DISCOUNT_FACTOR = 0.99;
constexpr double INITIAL_EXPLORE_RATE = 1.0;
constexpr double EXPLORE_DECAY = 0.995;
constexpr double MIN_EXPLORE_RATE = 0.05;
constexpr size_t EXPERIENCE_BUFFER_SIZE = 100000;
constexpr int BATCH_SIZE = 32;
constexpr int SLEEP_MS = 1;
constexpr int NUM_THREADS = 16;

// ============================================================================
// Neural Network Architecture
// ============================================================================

constexpr int INPUT_SIZE = 12;
constexpr int HIDDEN_LAYER1_SIZE = 64;
constexpr int HIDDEN_LAYER2_SIZE = 32;
constexpr int OUTPUT_SIZE = 4;
constexpr int TARGET_UPDATE_FREQUENCY = 1000;

// ============================================================================
// Time Warp Parameters
// ============================================================================

constexpr bool DEFAULT_TIME_WARP_MODE = false;
constexpr double DEFAULT_TIME_WARP_FACTOR = 1.0;

// ============================================================================
// Display Parameters (dynamic — updated on window resize)
// ============================================================================

inline int SCREEN_WIDTH = 1600;
inline int SCREEN_HEIGHT = 900;
inline int SIDEBAR_WIDTH = SCREEN_WIDTH / 4;
inline int GRID_WIDTH = SCREEN_WIDTH - SIDEBAR_WIDTH;
inline int GRID_HEIGHT = SCREEN_HEIGHT;
inline int CELL_SIZE = (GRID_WIDTH < GRID_HEIGHT ? GRID_WIDTH : GRID_HEIGHT) / GRID_SIZE;

inline void recalculate_layout() {
    SIDEBAR_WIDTH = SCREEN_WIDTH / 4;
    GRID_WIDTH = SCREEN_WIDTH - SIDEBAR_WIDTH;
    GRID_HEIGHT = SCREEN_HEIGHT;
    CELL_SIZE = std::max(1, (std::min(GRID_WIDTH, GRID_HEIGHT)) / GRID_SIZE);
}

inline std::string asset_prefix() {
    for (const char* p : {"assets/", "../assets/", "./"}) {
        if (std::ifstream(std::string(p) + "Futura.ttf").good()) return p;
    }
    return "assets/";
}

inline std::string brain_file_path() {
    return asset_prefix() + "brain.json";
}

} // namespace Config

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
