#pragma once

#include "config.hpp"
#include "raylib.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "raygui.h"
#pragma GCC diagnostic pop
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <functional>
#include <fstream>

class Environment;

struct TrainingConfig {
    int episodes = 1000;
    int threads  = 4;
    bool auto_save   = true;
    bool load_brain  = false;
};

struct SimulationConfig {
    bool load_brain  = false;
    int fps_cap      = 60;
};

struct TrainingStatus {
    std::atomic<bool> active{false};
    std::atomic<bool> stop_flag{false};
    std::atomic<int> episodes_done{0};
    std::atomic<int> total_episodes{0};
    std::atomic<int> threads_done{0};
    std::atomic<int> total_threads{0};
    std::vector<int> food_counts;
    std::atomic<int> global_best_food{0};
    std::mutex best_brain_mutex;
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<Environment>> envs;
    bool auto_save = false;
};

using TrainingStartCallback = std::function<void(const TrainingConfig&, TrainingStatus*)>;

extern std::atomic<bool> g_training_stop_flag;

enum class MenuState {
    HOME,
    TRAINING_SCREEN,
    TRAINING,
    TRAINING_ACTIVE,
    SETTINGS,
    ABOUT,
    START_SIM,
    EXIT
};

class Menu {
public:
    Menu(TrainingStatus* training_status = nullptr);
    ~Menu();

    MenuState run();

    TrainingConfig get_training_config() const { return training_config_; }
    SimulationConfig get_simulation_config() const {
        SimulationConfig config;
        config.load_brain = std::ifstream(brain_file_path()).good();
        return config;
    }

    void set_training_active(bool active, int total_episodes = 0, int threads = 0);
    bool is_training_active() const;
    void set_training_callback(TrainingStartCallback cb) { training_start_callback_ = cb; }

private:
    MenuState current_state_ = MenuState::HOME;
    TrainingConfig training_config_;
    TrainingStatus* training_status_ptr_ = nullptr;
    TrainingStartCallback training_start_callback_;

    // Editing state
    bool editing_episodes_ = false;
    bool editing_threads_  = false;
    bool editing_grid_ = false;
    bool editing_agents_ = false;
    bool editing_food_count_ = false;
    bool editing_food_thresh_ = false;
    char ep_buf_[32]{};
    char th_buf_[32]{};
    char grid_buf_[32]{};
    char agents_buf_[32]{};
    char food_count_buf_[32]{};
    char food_thresh_buf_[32]{};

    void do_home();
    void do_training_config();
    void do_training_active();
    void do_settings();
    void do_about();
};
