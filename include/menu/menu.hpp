/**
 * @file menu.hpp
 * @brief Home menu system for CUBES simulation.
 */

#pragma once

#include "config.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <fstream>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <functional>

// Forward declarations
class Environment;

/**
 * @brief Training configuration.
 */
struct TrainingConfig {
    int episodes = 1000;     ///< Number of training episodes
    int threads = 4;        ///< Processing power (1-16)
    bool auto_save = true;    ///< Auto-save when done
    bool load_brain = false;  ///< Load ./build/brain.json before training
};

/**
 * @brief Simulation configuration from menu.
 */
struct SimulationConfig {
    bool load_brain = false;  ///< Load ./build/brain.json before starting
    int fps_cap = 60;         ///< Frame rate cap (0 = unlimited)
};

// Training status shared between main and menu
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

// Callback for when training should start
using TrainingStartCallback = std::function<void(const TrainingConfig&, TrainingStatus*)>;

// Global stop flag for training (defined in main.cpp)
extern std::atomic<bool> g_training_stop_flag;

/**
 * @brief Menu states/screens.
 */
enum class MenuState {
    HOME,           ///< Main menu screen
    TRAINING_SCREEN,///< Training options screen (settings)
    TRAINING,       ///< Start training and exit menu
    TRAINING_ACTIVE,///< Training in progress with progress bar
    SETTINGS,       ///< Settings screen
    ABOUT,          ///< About/help screen
    START_SIM,      ///< Start the simulation
    EXIT            ///< Exit the program
};

/**
 * @brief Button widget for the menu.
 */
struct MenuButton {
    SDL_Rect rect;
    std::string text;
    bool hovered;
};

/**
 * @brief Home menu system for CUBES simulation.
 */
class Menu {
private:
    SDL_Renderer* renderer_;
    TTF_Font* title_font_;
    TTF_Font* button_font_;
    TTF_Font* text_font_;
    
    MenuState current_state_;
    std::vector<MenuButton> home_buttons_;
    TrainingConfig training_config_;
    Uint32 start_time_;
    
    // Training progress tracking (pointer to external status)
    TrainingStatus* training_status_ptr_;
    SDL_Rect stop_button_rect_;
    
    // Text input state
    enum class EditField { NONE, EPISODES, THREADS, AGENTS, FOOD_COUNT, GRID_SIZE, FOOD_THRESHOLD };
    
    void init_home_buttons();
    void render_home();
    void render_training();
    void render_training_active();
    void render_about();
    
    void handle_home_click(int x, int y);
    void handle_training_click(int x, int y);
    void apply_edits();
    
    void render_button(const MenuButton& button);
    void render_text(const std::string& text, int x, int y, 
                    TTF_Font* font, SDL_Color color);
    int center_x(int element_width, int total_width = 800);
    int center_text_x(const std::string& text, TTF_Font* font, int total_width = 800);
    void activate_home_button(int index);
    
    // Settings screen methods
    void render_settings();
    void handle_settings_click(int x, int y);
    void render_field(const SDL_Rect& field, EditField field_type, const std::string& display);
    
    // Menu navigation state
    int selected_home_button_index_ = 0;
    int mouse_x_ = 0;
    int mouse_y_ = 0;
    
    EditField editing_field_ = EditField::NONE;
    std::string input_buffer_;
    
    // Training start callback
    TrainingStartCallback training_start_callback_;
    
public:
    Menu(SDL_Renderer* renderer, TTF_Font* title_font, 
         TTF_Font* button_font, TTF_Font* text_font, 
         TrainingStatus* training_status = nullptr);
    
    ~Menu();
    
    MenuState run();
    
    TrainingConfig get_training_config() const { return training_config_; }
    SimulationConfig get_simulation_config() const { 
         SimulationConfig config;
         config.load_brain = std::ifstream("./build/brain.json").good();
         return config;
     }
    
    // Training progress methods
    void set_training_active(bool active, int total_episodes = 0, int threads = 0);
    bool is_training_active() const;
    
    // Set callback for when training should start
    void set_training_callback(TrainingStartCallback cb) { training_start_callback_ = cb; }
};
