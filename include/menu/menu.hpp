#pragma once

#include "config.hpp"
#include "BS_thread_pool.hpp"
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
    int total_food;
};

struct TrainingConfig {
    int episodes = 10000;
    bool auto_save = true;
    int parallel_envs = 4;
};

struct ParallelEnvState {
    std::unique_ptr<Environment> env;
    std::atomic<int> episodes_done{0};
    std::atomic<int> local_best{0};
    std::atomic<int> local_total{0};
};

struct TrainingStatus {
    std::atomic<bool> active{false};
    std::atomic<int> episodes_done{0};
    std::atomic<int> total_episodes{0};
    std::atomic<int> best_food{0};
    std::atomic<int> total_food_all_time{0};
    std::thread thread;
    std::vector<std::unique_ptr<ParallelEnvState>> envs;
    BS::light_thread_pool env_pool;
    bool auto_save = false;
    int parallel_count = 1;
    std::vector<TrainingFoodPoint> food_history;
    std::mutex history_mutex;
    std::atomic<int> last_saved_best_{0};
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
    bool editing_parallel_ = false;
    char par_buf_[32]{};

    // Settings editing state
    bool settings_inited_ = false;
    bool editing_grid_ = false;
    bool editing_agents_ = false;
    bool editing_food_count_ = false;
    bool editing_food_energy_ = false;
    bool editing_food_thresh_ = false;
    bool editing_max_energy_ = false;
    bool editing_energy_decay_ = false;
    bool editing_lr_ = false;
    bool editing_discount_ = false;
    bool editing_explore_init_ = false;
    bool editing_explore_decay_ = false;
    bool editing_explore_min_ = false;
    bool editing_buf_size_ = false;
    bool editing_batch_size_ = false;
    bool editing_target_freq_ = false;
    bool editing_max_threads_ = false;
    char grid_buf_[32]{};
    char agents_buf_[32]{};
    char food_count_buf_[32]{};
    char food_energy_buf_[32]{};
    char food_thresh_buf_[32]{};
    char max_energy_buf_[32]{};
    char energy_decay_buf_[32]{};
    char lr_buf_[32]{};
    char discount_buf_[32]{};
    char explore_init_buf_[32]{};
    char explore_decay_buf_[32]{};
    char explore_min_buf_[32]{};
    char buf_size_buf_[32]{};
    char batch_size_buf_[32]{};
    char target_freq_buf_[32]{};
    char max_threads_buf_[32]{};

    void do_home();
    void do_training_config();
    void do_training_active();
    void do_settings();
    void do_about();
};
