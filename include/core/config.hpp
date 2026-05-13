/**
 * @file config.hpp
 * @brief Central configuration for the AI Learning Simulation.
 */

#pragma once

#include <cstddef>
#include <algorithm>
#include <string>
#include <fstream>
#include <thread>

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
constexpr int HIDDEN_LAYER1_SIZE = 32;
constexpr int HIDDEN_LAYER2_SIZE = 32;
constexpr int OUTPUT_SIZE = 4;
constexpr int TARGET_UPDATE_FREQUENCY = 1000;

// ============================================================================
// Time Warp Parameters
// ============================================================================

constexpr bool DEFAULT_TIME_WARP_MODE = true;
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
    int food_reset_threshold = 10;
    int max_energy = MAX_ENERGY;
    int energy_decay = ENERGY_DECAY;

    double learning_rate = LEARNING_RATE;
    double discount_factor = DISCOUNT_FACTOR;
    double initial_explore_rate = INITIAL_EXPLORE_RATE;
    double explore_decay = EXPLORE_DECAY;
    double min_explore_rate = MIN_EXPLORE_RATE;

    int experience_buffer_size = static_cast<int>(EXPERIENCE_BUFFER_SIZE);
    int batch_size = BATCH_SIZE;
    int target_update_frequency = TARGET_UPDATE_FREQUENCY;

    int max_threads = 0;  // 0 = auto (use all available cores)

    int get_thread_count() const {
        if (max_threads > 0) return max_threads;
        int hc = static_cast<int>(std::thread::hardware_concurrency());
        return hc > 0 ? hc : 1;
    }

    static RuntimeConfig& instance() {
        static RuntimeConfig config;
        return config;
    }
};
