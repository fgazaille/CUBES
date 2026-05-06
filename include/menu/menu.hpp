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

/**
 * @brief Menu states/screens.
 */
enum class MenuState {
    HOME,           ///< Main menu screen
    TRAINING_SCREEN,///< Training options screen (settings)
    TRAINING,       ///< Start training and exit menu
    SETTINGS,       ///< Configuration editor
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
     * @brief Training configuration.
     */
    struct TrainingConfig {
        int episodes = 1000;     ///< Number of training episodes
        int threads = 4;        ///< Processing power (1-16)
        bool auto_save = true;    ///< Auto-save when done
        bool load_brain = false;  ///< Load brain.json before training
    };

    /**
     * @brief Simulation configuration from menu.
     */
    struct SimulationConfig {
        bool load_brain = false;  ///< Load brain.json before starting
        int fps_cap = 60;         ///< Frame rate cap (0 = unlimited)
    };

/**
 * @brief Simulation configuration from menu.
 */

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
    
    void init_home_buttons();
    void render_home();
    void render_training();
    void render_settings();
    void render_about();
    
    void handle_home_click(int x, int y);
    void handle_training_click(int x, int y);
    void handle_settings_click(int x, int y);
    void apply_edits();
    
    void render_button(const MenuButton& button);
    void render_text(const std::string& text, int x, int y, 
                    TTF_Font* font, SDL_Color color);
    int center_x(int element_width, int total_width = 800);
    int center_text_x(const std::string& text, TTF_Font* font, int total_width = 800);
    void activate_home_button(int index);
    
    // Menu navigation state
    int selected_home_button_index_ = 0;
    int mouse_x_ = 0;
    int mouse_y_ = 0;
    
    // Text input state
    enum class EditField { NONE, EPISODES, THREADS };
    EditField editing_field_ = EditField::NONE;
    std::string input_buffer_;
    
public:
    Menu(SDL_Renderer* renderer, TTF_Font* title_font, 
         TTF_Font* button_font, TTF_Font* text_font);
    
    ~Menu();
    
    MenuState run();
    
    TrainingConfig get_training_config() const { return training_config_; }
    SimulationConfig get_simulation_config() const { 
        SimulationConfig config;
        config.load_brain = std::ifstream("brain.json").good();
        return config;
    }
};
