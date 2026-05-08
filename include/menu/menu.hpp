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

struct TrainingFoodPoint {
    int episodes_done;
    int best_food;
};

struct TrainingConfig {
    int episodes = 10000;
    bool auto_save = true;
};

struct TrainingStatus {
    std::atomic<bool> active{false};
    std::atomic<int> episodes_done{0};
    std::atomic<int> total_episodes{0};
    std::atomic<int> best_food{0};
    std::thread thread;
    std::unique_ptr<Environment> env;
    bool auto_save = false;
    std::vector<TrainingFoodPoint> food_history;
    std::mutex history_mutex;
};

using TrainingStartCallback = std::function<void(const TrainingConfig&, TrainingStatus*)>;

extern std::atomic<bool> g_training_stop_flag;

enum class MenuState {
    HOME,
    TRAINING_CONFIG,
    TRAINING_ACTIVE,
    SETTINGS,
    ABOUT,
    START_SIM,
    EXIT
};

class Menu {
public:
    Menu(TrainingStatus* training_status = nullptr);
    ~Menu() = default;

    MenuState run();

    TrainingConfig get_training_config() const { return training_config_; }

    void set_training_callback(TrainingStartCallback cb) { training_start_callback_ = cb; }

private:
    MenuState current_state_ = MenuState::HOME;
    TrainingConfig training_config_;
    TrainingStatus* training_status_ptr_ = nullptr;
    TrainingStartCallback training_start_callback_;

    bool editing_episodes_ = false;
    char ep_buf_[32]{};

    // Settings editing state
    bool editing_grid_ = false;
    bool editing_agents_ = false;
    bool editing_food_count_ = false;
    bool editing_food_thresh_ = false;
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
