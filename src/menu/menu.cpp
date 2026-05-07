/**
 * @file menu.cpp
 * @brief Implementation of the home menu system.
 */

#include "menu.hpp"
#include <sstream>
#include <cmath>
#include <fstream>
#include <unordered_map>

// ============================================================================
// Text Cache for Menu Rendering
// ============================================================================

namespace {
    struct MenuTextCacheKey {
        std::string text;
        SDL_Color color;
        
        bool operator==(const MenuTextCacheKey& other) const {
            return text == other.text && 
                   color.r == other.color.r && color.g == other.color.g &&
                   color.b == other.color.b && color.a == other.color.a;
        }
    };
    
    struct MenuTextCacheKeyHash {
        std::size_t operator()(const MenuTextCacheKey& k) const {
            return std::hash<std::string>{}(k.text) ^ 
                   (k.color.r << 24 | k.color.g << 16 | k.color.b << 8 | k.color.a);
        }
    };
    
    struct CachedMenuText {
        SDL_Texture* texture;
        int width;
        int height;
        
        CachedMenuText() : texture(nullptr), width(0), height(0) {}
        CachedMenuText(SDL_Texture* tex, int w, int h) : texture(tex), width(w), height(h) {}
    };
    
    std::unordered_map<MenuTextCacheKey, CachedMenuText, MenuTextCacheKeyHash> menu_text_cache;
    TTF_Font* last_menu_font = nullptr;
}

void clear_menu_text_cache() {
    for (auto& pair : menu_text_cache) {
        if (pair.second.texture) {
            SDL_DestroyTexture(pair.second.texture);
        }
    }
    menu_text_cache.clear();
    last_menu_font = nullptr;
}

void render_cached_menu_text(SDL_Renderer* renderer, TTF_Font* font, 
                             const std::string& text, int x, int y, SDL_Color color) {
    if (!font || !renderer) return;
    
    if (font != last_menu_font) {
        clear_menu_text_cache();
        last_menu_font = font;
    }
    
    MenuTextCacheKey key{text, color};
    auto it = menu_text_cache.find(key);
    
    if (it == menu_text_cache.end()) {
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (!surface) return;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_FreeSurface(surface);
            return;
        }
        
        CachedMenuText cached{texture, surface->w, surface->h};
        SDL_FreeSurface(surface);
        
        it = menu_text_cache.emplace(key, cached).first;
    }
    
    const CachedMenuText& cached = it->second;
    SDL_Rect rect = {x, y, cached.width, cached.height};
    SDL_RenderCopy(renderer, cached.texture, NULL, &rect);
}

// ============================================================================
// Menu Construction/Destruction
// ============================================================================

Menu::Menu(SDL_Renderer* renderer,
             TTF_Font* title_font, TTF_Font* button_font, TTF_Font* text_font)
    : renderer_(renderer), title_font_(title_font), 
      button_font_(button_font), text_font_(text_font),
      current_state_(MenuState::HOME) {
    start_time_ = SDL_GetTicks();
    init_home_buttons();
}

Menu::~Menu() {
    clear_menu_text_cache();
}

// ============================================================================
// Initialization
// ============================================================================

void Menu::init_home_buttons() {
    home_buttons_.clear();
    home_buttons_.push_back({{300, 180, 200, 45}, "Start Simulation", false});
    home_buttons_.push_back({{300, 235, 200, 45}, "Training", false});
    home_buttons_.push_back({{300, 290, 200, 45}, "About", false});
    home_buttons_.push_back({{300, 345, 200, 45}, "Exit", false});
}

// ============================================================================
// Rendering Helpers
// ============================================================================

int Menu::center_x(int element_width, int total_width) {
    return (total_width - element_width) / 2;
}

int Menu::center_text_x(const std::string& text, TTF_Font* font, int total_width) {
    if (!font) return 0;
    int w, h;
    TTF_SizeText(font, text.c_str(), &w, &h);
    return center_x(w, total_width);
}

void Menu::render_text(const std::string& text, int x, int y, 
                       TTF_Font* font, SDL_Color color) {
    if (!font || !renderer_) return;
    // Use cached text rendering
    render_cached_menu_text(renderer_, font, text, x, y, color);
}

void Menu::render_button(const MenuButton& button) {
    SDL_Color color = button.hovered ? SDL_Color{100,100,255,255} : SDL_Color{220,220,240,255};
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &button.rect);
    SDL_SetRenderDrawColor(renderer_, 60,60,80,255);
    SDL_RenderDrawRect(renderer_, &button.rect);
    
    SDL_Color text_color = {0,0,0,255};
    SDL_Surface* surface = TTF_RenderText_Blended(button_font_, button.text.c_str(), text_color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        if (texture) {
            int text_x = button.rect.x + (button.rect.w - surface->w)/2;
            int text_y = button.rect.y + (button.rect.h - surface->h)/2;
            SDL_Rect rect = {text_x, text_y, surface->w, surface->h};
            SDL_RenderCopy(renderer_, texture, NULL, &rect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

// ============================================================================
// Render Screens
// ============================================================================

void Menu::render_home() {
    Uint32 elapsed = SDL_GetTicks() - start_time_;
    double pulse = std::sin(elapsed * 0.002) * 0.15 + 0.85;
    mouse_x_ = 0; mouse_y_ = 0;
    SDL_GetMouseState(&mouse_x_, &mouse_y_);
    
    // Subtle animated gradient background
    for (int y = 0; y < 600; y += 10) {
        Uint8 shade = static_cast<Uint8>(20 + 8.0 * std::sin((y + elapsed * 0.15) * 0.02));
        SDL_SetRenderDrawColor(renderer_, shade, shade, static_cast<Uint8>(35 + 5 * pulse), 255);
        SDL_Rect row = {0, y, 800, 10};
        SDL_RenderFillRect(renderer_, &row);
    }
    
    // Check if brain.json exists and show status
    bool brain_exists = std::ifstream("brain.json").good();
    
    int title_x = center_text_x("CUBES", title_font_);
    render_text("CUBES", title_x, 50, title_font_, {190,190,255,255});
    int subtitle_x = center_text_x("AI Learning Simulation", text_font_);
    render_text("AI Learning Simulation", subtitle_x, 120, text_font_, {210,210,240,255});
    int hint_x = center_text_x("Use mouse or ENTER to choose. ESC returns.", text_font_);
    render_text("Use mouse or ENTER to choose. ESC returns.", hint_x, 150, text_font_, {170,170,210,255});
    
    int hovered_index = -1;
    for (size_t i = 0; i < home_buttons_.size(); ++i) {
        auto& button = home_buttons_[i];
        SDL_Point p = {mouse_x_, mouse_y_};
        button.hovered = SDL_PointInRect(&p, &button.rect);
        if (button.hovered) hovered_index = static_cast<int>(i);
        render_button(button);
    }
    
    if (hovered_index >= 0) {
        selected_home_button_index_ = hovered_index;
    }

    // Highlight current keyboard-selected button when not hovered
    if (hovered_index < 0 && selected_home_button_index_ >= 0 && selected_home_button_index_ < static_cast<int>(home_buttons_.size())) {
        auto& button = home_buttons_[selected_home_button_index_];
        SDL_SetRenderDrawColor(renderer_, 160, 160, 255, 128);
        SDL_RenderFillRect(renderer_, &button.rect);
        render_button(button);
    }
    
    // Show brain.json status
    if (brain_exists) {
        render_text("Brain file found - will auto-load", 550, 570, text_font_, {140,180,140,255});
    } else {
        render_text("No brain file - agents will use random brains", 550, 570, text_font_, {180,140,140,255});
    }
    
    SDL_RenderPresent(renderer_);
}

// ============================================================================
// Training Screen
// ============================================================================

void Menu::render_about() {
    SDL_SetRenderDrawColor(renderer_, 14, 18, 30, 255);
    SDL_RenderClear(renderer_);
    
    int title_x = center_text_x("About CUBES", title_font_);
    render_text("About CUBES", title_x, 50, title_font_, {190, 190, 255, 255});
    
    int y = 120;
    render_text("CUBES - AI Learning Simulation", 100, y, text_font_, {220, 220, 240, 255});
    y += 40;
    render_text("Version 2.0 - Refactored", 100, y, text_font_, {200, 200, 220, 255});
    y += 40;
    render_text("Uses Deep Q-Network (DQN) reinforcement learning", 100, y, text_font_, {200, 200, 220, 255});
    y += 30;
    render_text("Agents learn to collect food and survive", 100, y, text_font_, {200, 200, 220, 255});
    y += 30;
    render_text("Genetic algorithms evolve better agents each generation", 100, y, text_font_, {200, 200, 220, 255});
    y += 50;
    render_text("Controls:", 100, y, text_font_, {220, 220, 240, 255});
    y += 30;
    render_text("ESC - Exit | R - Reset | Space - Pause", 100, y, text_font_, {180, 180, 200, 255});
    y += 25;
    render_text("W - Toggle Warp | +/- - Warp Speed", 100, y, text_font_, {180, 180, 200, 255});
    y += 25;
    render_text("D - Debug Mode | S - Save Brain | L - Load Brain", 100, y, text_font_, {180, 180, 200, 255});
    y += 25;
    render_text("F5 - Save State | F9 - Load State", 100, y, text_font_, {180, 180, 200, 255});
    y += 50;
    
    int back_x = center_x(100);
    SDL_Rect back_rect = {back_x, 500, 100, 40};
    SDL_SetRenderDrawColor(renderer_, 60, 60, 80, 255);
    SDL_RenderFillRect(renderer_, &back_rect);
    SDL_SetRenderDrawColor(renderer_, 100, 100, 170, 255);
    SDL_RenderDrawRect(renderer_, &back_rect);
    render_text("Back", back_x + 25, 510, text_font_, {220, 220, 240, 255});
    
    SDL_RenderPresent(renderer_);
}

void Menu::render_training() {
    SDL_SetRenderDrawColor(renderer_, 14, 18, 30, 255);
    SDL_RenderClear(renderer_);
    
    int title_x = center_text_x("Training Configuration", title_font_);
    render_text("Training Configuration", title_x, 20, title_font_, {190, 190, 255, 255});
    int subtitle_x = center_text_x("Quickly tune training parameters before launching.", text_font_);
    render_text("Quickly tune training parameters before launching.", subtitle_x, 90, text_font_, {200, 200, 240, 255});
    
    SDL_SetRenderDrawColor(renderer_, 30, 34, 55, 255);
    SDL_Rect panel = {140, 130, 520, 340};
    SDL_RenderFillRect(renderer_, &panel);
    SDL_SetRenderDrawColor(renderer_, 100, 100, 170, 255);
    SDL_RenderDrawRect(renderer_, &panel);
    
    render_text("Episodes:", 180, 170, text_font_, {220, 220, 240, 255});
    SDL_Rect ep_field = {320, 165, 160, 36};
    if (editing_field_ == EditField::EPISODES) {
        SDL_SetRenderDrawColor(renderer_, 100, 100, 200, 255);
    } else {
        SDL_SetRenderDrawColor(renderer_, 60, 60, 80, 255);
    }
    SDL_RenderFillRect(renderer_, &ep_field);
    SDL_SetRenderDrawColor(renderer_, 140, 140, 200, 255);
    SDL_RenderDrawRect(renderer_, &ep_field);
    std::string ep_display = (editing_field_ == EditField::EPISODES) ? input_buffer_ + "_" : std::to_string(training_config_.episodes);
    render_text(ep_display, 330, 170, text_font_, {255, 255, 255, 255});
    
    render_text("Threads:", 180, 230, text_font_, {220, 220, 240, 255});
    SDL_Rect th_field = {320, 225, 160, 36};
    if (editing_field_ == EditField::THREADS) {
        SDL_SetRenderDrawColor(renderer_, 100, 100, 200, 255);
    } else {
        SDL_SetRenderDrawColor(renderer_, 60, 60, 80, 255);
    }
    SDL_RenderFillRect(renderer_, &th_field);
    SDL_SetRenderDrawColor(renderer_, 140, 140, 200, 255);
    SDL_RenderDrawRect(renderer_, &th_field);
    std::string th_display = (editing_field_ == EditField::THREADS) ? input_buffer_ + "_" : std::to_string(training_config_.threads);
    render_text(th_display, 330, 230, text_font_, {255, 255, 255, 255});
    
    std::string auto_save_str = training_config_.auto_save ? "ON" : "OFF";
    render_text("Auto-save:", 180, 290, text_font_, {220, 220, 240, 255});
    MenuButton toggle_btn = {{320, 285, 100, 34}, auto_save_str, false};
    
    std::string load_brain_str = training_config_.load_brain ? "ON" : "OFF";
    render_text("Load Brain:", 180, 340, text_font_, {220, 220, 240, 255});
    MenuButton load_brain_btn = {{320, 335, 100, 34}, load_brain_str, false};
    
    int btn_y = 410;
    int btn_w = 80;
    int btn_spacing = 20;
    int total_btns_width = 4 * btn_w + 3 * btn_spacing;
    int btns_start_x = center_x(total_btns_width);
    
    MenuButton ep1k_btn = {{btns_start_x, btn_y, btn_w, 34}, "1K", false};
    MenuButton ep10k_btn = {{btns_start_x + btn_w + btn_spacing, btn_y, btn_w, 34}, "10K", false};
    MenuButton ep50k_btn = {{btns_start_x + 2*(btn_w + btn_spacing), btn_y, btn_w, 34}, "50K", false};
    MenuButton ep100k_btn = {{btns_start_x + 3*(btn_w + btn_spacing), btn_y, btn_w, 34}, "100K", false};
    
    int action_btn_y = 480;
    int action_btn_w = 160;
    int action_btn_spacing = 20;
    int total_action_width = 2 * action_btn_w + action_btn_spacing;
    int action_start_x = center_x(total_action_width);
    
    MenuButton start_btn = {{action_start_x, action_btn_y, action_btn_w, 46}, "Start Training", false};
    MenuButton back_btn = {{action_start_x + action_btn_w + action_btn_spacing, action_btn_y, action_btn_w, 46}, "Back", false};
    
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    SDL_Point p = {mx, my};
    toggle_btn.hovered = SDL_PointInRect(&p, &toggle_btn.rect);
    load_brain_btn.hovered = SDL_PointInRect(&p, &load_brain_btn.rect);
    ep1k_btn.hovered = SDL_PointInRect(&p, &ep1k_btn.rect);
    ep10k_btn.hovered = SDL_PointInRect(&p, &ep10k_btn.rect);
    ep50k_btn.hovered = SDL_PointInRect(&p, &ep50k_btn.rect);
    ep100k_btn.hovered = SDL_PointInRect(&p, &ep100k_btn.rect);
    start_btn.hovered = SDL_PointInRect(&p, &start_btn.rect);
    back_btn.hovered = SDL_PointInRect(&p, &back_btn.rect);
    
    render_button(toggle_btn);
    render_button(load_brain_btn);
    render_button(ep1k_btn);
    render_button(ep10k_btn);
    render_button(ep50k_btn);
    render_button(ep100k_btn);
    render_button(start_btn);
    render_button(back_btn);
    
    render_text("Tip: Press ENTER to start training.", subtitle_x, 540, text_font_, {180, 180, 220, 255});
    SDL_RenderPresent(renderer_);
}

void Menu::handle_training_click(int x, int y) {
    if (x >= 320 && x <= 480 && y >= 165 && y <= 201) {
        editing_field_ = EditField::EPISODES;
        input_buffer_ = std::to_string(training_config_.episodes);
        SDL_StartTextInput();
        return;
    }
    if (x >= 320 && x <= 480 && y >= 225 && y <= 261) {
        editing_field_ = EditField::THREADS;
        input_buffer_ = std::to_string(training_config_.threads);
        SDL_StartTextInput();
        return;
    }
    if (editing_field_ != EditField::NONE) {
        apply_edits();
        editing_field_ = EditField::NONE;
        SDL_StopTextInput();
    }
    if (x >= 320 && x <= 420 && y >= 285 && y <= 319) {
        training_config_.auto_save = !training_config_.auto_save;
    }
    if (x >= 320 && x <= 420 && y >= 335 && y <= 369) {
        training_config_.load_brain = !training_config_.load_brain;
    }
    
    int btn_y = 410;
    int btn_w = 80;
    int btn_spacing = 20;
    int total_btns_width = 4 * btn_w + 3 * btn_spacing;
    int btns_start_x = center_x(total_btns_width);
    
    if (x >= btns_start_x && x <= btns_start_x + btn_w && y >= btn_y && y <= btn_y + 34) {
        training_config_.episodes = 1000;
        return;
    }
    if (x >= btns_start_x + btn_w + btn_spacing && x <= btns_start_x + 2*btn_w + btn_spacing && y >= btn_y && y <= btn_y + 34) {
        training_config_.episodes = 10000;
        return;
    }
    if (x >= btns_start_x + 2*(btn_w + btn_spacing) && x <= btns_start_x + 3*btn_w + 2*btn_spacing && y >= btn_y && y <= btn_y + 34) {
        training_config_.episodes = 50000;
        return;
    }
    if (x >= btns_start_x + 3*(btn_w + btn_spacing) && x <= btns_start_x + 4*btn_w + 3*btn_spacing && y >= btn_y && y <= btn_y + 34) {
        training_config_.episodes = 100000;
        return;
    }
    
    int action_btn_y = 470;
    int action_btn_w = 120;
    int action_btn_spacing = 20;
    int total_action_width = 2 * action_btn_w + action_btn_spacing;
    int action_start_x = center_x(total_action_width);
    
    if (x >= action_start_x && x <= action_start_x + action_btn_w && y >= action_btn_y && y <= action_btn_y + 46) {
        current_state_ = MenuState::TRAINING;
        return;
    }
    if (x >= action_start_x + action_btn_w + action_btn_spacing && x <= action_start_x + 2*action_btn_w + action_btn_spacing && y >= action_btn_y && y <= action_btn_y + 46) {
        current_state_ = MenuState::HOME;
    }
}

// ============================================================================
// Event Handling
// ============================================================================

void Menu::activate_home_button(int index) {
    if (index < 0 || index >= static_cast<int>(home_buttons_.size())) return;
    const std::string& text = home_buttons_[index].text;
    if (text == "Start Simulation") {
        current_state_ = MenuState::START_SIM;
    } else if (text == "Training") {
        current_state_ = MenuState::TRAINING_SCREEN;
    } else if (text == "About") {
        current_state_ = MenuState::ABOUT;
    } else if (text == "Exit") {
        current_state_ = MenuState::EXIT;
    }
}

void Menu::handle_home_click(int x, int y) {
    for (size_t i = 0; i < home_buttons_.size(); ++i) {
        if (x >= home_buttons_[i].rect.x && 
            x <= home_buttons_[i].rect.x + home_buttons_[i].rect.w &&
            y >= home_buttons_[i].rect.y && 
            y <= home_buttons_[i].rect.y + home_buttons_[i].rect.h) {
            selected_home_button_index_ = static_cast<int>(i);
            activate_home_button(selected_home_button_index_);
            return;
        }
    }
}

void Menu::handle_settings_click(int x, int y) {
    int back_x = center_x(100);
    SDL_Rect back_rect = {back_x, 500, 100, 40};
    if (x >= back_rect.x && x <= back_rect.x + back_rect.w &&
        y >= back_rect.y && y <= back_rect.y + back_rect.h) {
        current_state_ = MenuState::HOME;
    }
}

void Menu::apply_edits() {
    if (editing_field_ == EditField::EPISODES) {
        try {
            int val = std::stoi(input_buffer_);
            training_config_.episodes = std::max(100, std::min(500000, val));
        } catch (...) { /* invalid input, keep current */ }
    } else if (editing_field_ == EditField::THREADS) {
        try {
            int val = std::stoi(input_buffer_);
            training_config_.threads = std::max(1, std::min(16, val));
        } catch (...) { /* invalid input, keep current */ }
    }
}

// ============================================================================
// Main Menu Loop
// ============================================================================

MenuState Menu::run() {
    SDL_Event event;
    bool running = true;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                current_state_ = MenuState::EXIT;
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int x = event.button.x;
                    int y = event.button.y;
                    
                    if (current_state_ == MenuState::HOME) {
                        handle_home_click(x, y);
                    } else if (current_state_ == MenuState::TRAINING_SCREEN) {
                        handle_training_click(x, y);
                    } else if (current_state_ == MenuState::ABOUT) {
                        // Check if back button clicked
                        int back_x = center_x(100);
                        SDL_Rect back_rect = {back_x, 500, 100, 40};
                        if (x >= back_rect.x && x <= back_rect.x + back_rect.w &&
                            y >= back_rect.y && y <= back_rect.y + back_rect.h) {
                            current_state_ = MenuState::HOME;
                        }
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (current_state_ == MenuState::HOME) {
                        current_state_ = MenuState::EXIT;
                        running = false;
                    } else if (editing_field_ != EditField::NONE) {
                        editing_field_ = EditField::NONE;
                        SDL_StopTextInput();
                    } else {
                        current_state_ = MenuState::HOME;
                    }
                } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    if (editing_field_ != EditField::NONE) {
                        apply_edits();
                        editing_field_ = EditField::NONE;
                        SDL_StopTextInput();
                    } else if (current_state_ == MenuState::HOME) {
                        activate_home_button(selected_home_button_index_);
                    } else if (current_state_ == MenuState::TRAINING_SCREEN) {
                        current_state_ = MenuState::TRAINING;
                    }
                } else if (event.key.keysym.sym == SDLK_UP) {
                    if (current_state_ == MenuState::HOME) {
                        selected_home_button_index_ = (selected_home_button_index_ + static_cast<int>(home_buttons_.size()) - 1) % static_cast<int>(home_buttons_.size());
                    }
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    if (current_state_ == MenuState::HOME) {
                        selected_home_button_index_ = (selected_home_button_index_ + 1) % static_cast<int>(home_buttons_.size());
                    }
                } else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    if (editing_field_ != EditField::NONE && !input_buffer_.empty()) {
                        input_buffer_.pop_back();
                    }
                }
            } else if (event.type == SDL_TEXTINPUT) {
                if (editing_field_ != EditField::NONE) {
                    input_buffer_ += event.text.text;
                }
            }
        }
        
        if (current_state_ == MenuState::HOME) {
            render_home();
        } else if (current_state_ == MenuState::TRAINING_SCREEN) {
            render_training();
        } else if (current_state_ == MenuState::ABOUT) {
            render_about();
        }
        
        if (current_state_ == MenuState::START_SIM || 
            current_state_ == MenuState::TRAINING ||
            current_state_ == MenuState::EXIT) {
            running = false;
        }
        
        SDL_Delay(16);
    }
    
    return current_state_;
}
