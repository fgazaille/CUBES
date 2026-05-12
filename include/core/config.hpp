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
constexpr int AGENT_COUNT = 1;
constexpr int MAX_ENERGY = 200;
constexpr int ENERGY_DECAY = 1;
constexpr int FOOD_ENERGY = 80;

// ============================================================================
// Reinforcement Learning Parameters
// ============================================================================

constexpr double LEARNING_RATE = 0.001;   // Lower base rate for Adam optimizer
constexpr double DISCOUNT_FACTOR = 0.99;
constexpr double INITIAL_EXPLORE_RATE = 1.0;
constexpr double EXPLORE_DECAY = 0.997;
constexpr double MIN_EXPLORE_RATE = 0.01;
constexpr size_t EXPERIENCE_BUFFER_SIZE = 100000;
constexpr int BATCH_SIZE = 64;

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
// Utility Functions
// ============================================================================

inline int SCREEN_WIDTH = 1600;
inline int SCREEN_HEIGHT = 900;

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
