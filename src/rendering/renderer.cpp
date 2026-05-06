/**
 * @file renderer.cpp
 * @brief Implementation of main simulation rendering.
 * 
 * Renders the grid, agents, food, and sidebar with statistics.
 */

#include "renderer.hpp"
#include <sstream>
#include <iomanip>
#include <unordered_map>

// ============================================================================
// Text Cache Implementation
// ============================================================================

namespace {
    struct TextCacheKey {
        std::string text;
        SDL_Color color;
        
        bool operator==(const TextCacheKey& other) const {
            return text == other.text && 
                   color.r == other.color.r && color.g == other.color.g &&
                   color.b == other.color.b && color.a == other.color.a;
        }
    };
    
    struct TextCacheKeyHash {
        std::size_t operator()(const TextCacheKey& k) const {
            return std::hash<std::string>{}(k.text) ^ 
                   (k.color.r << 24 | k.color.g << 16 | k.color.b << 8 | k.color.a);
        }
    };
    
    std::unordered_map<TextCacheKey, CachedText, TextCacheKeyHash> text_cache;
    TTF_Font* last_font = nullptr;
}

void clear_text_cache() {
    for (auto& pair : text_cache) {
        if (pair.second.texture) {
            SDL_DestroyTexture(pair.second.texture);
        }
    }
    text_cache.clear();
    last_font = nullptr;
}

void render_cached_text(SDL_Renderer* renderer, TTF_Font* font, 
                        const std::string& text, int x, int y, SDL_Color color) {
    if (!font || !renderer) return;
    
    // Invalidate cache if font changed
    if (font != last_font) {
        clear_text_cache();
        last_font = font;
    }
    
    TextCacheKey key{text, color};
    auto it = text_cache.find(key);
    
    if (it == text_cache.end()) {
        // Create new texture
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (!surface) return;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_FreeSurface(surface);
            return;
        }
        
        CachedText cached{texture, surface->w, surface->h};
        SDL_FreeSurface(surface);
        
        it = text_cache.emplace(key, cached).first;
    }
    
    // Render cached texture
    const CachedText& cached = it->second;
    SDL_Rect rect = {x, y, cached.width, cached.height};
    SDL_RenderCopy(renderer, cached.texture, NULL, &rect);
}

// ============================================================================
// Original Text Rendering (for dynamic text)
// ============================================================================

void render_text(SDL_Renderer* renderer, TTF_Font* font, 
                const std::string& text, int x, int y, SDL_Color color) {
    if (!font || !renderer) return;
    
    // Render text to surface
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) return;
    
    // Convert to texture for rendering
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    
    // Draw texture at specified position
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    
    // Cleanup
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// ============================================================================
// Progress Bar Rendering
// ============================================================================

void render_progress_bar(SDL_Renderer* renderer, int x, int y, int w, int h,
                        float percentage, const Color& fill, const Color& bg) {
    // Draw background
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_Rect bg_rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &bg_rect);
    
    // Draw fill (clamped to 0-1)
    int fill_w = static_cast<int>(w * std::max(0.0f, std::min(1.0f, percentage)));
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_Rect fill_rect = {x, y, fill_w, h};
    SDL_RenderFillRect(renderer, &fill_rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b, COLOR_TEXT.a);
    SDL_RenderDrawRect(renderer, &bg_rect);
}

// ============================================================================
// Main Environment Rendering
// ============================================================================

void render_environment(SDL_Renderer* renderer, const Environment& env,
                        SDL_Texture* agent_tex, SDL_Texture* food_tex,
                        TTF_Font* large_font, TTF_Font* regular_font,
                        bool time_warp_mode, double time_warp_factor,
                        bool debug_mode, int selected_agent) {
    
    // === Clear screen with background color ===
    SDL_SetRenderDrawColor(renderer, COLOR_BACKGROUND.r, COLOR_BACKGROUND.g, 
                          COLOR_BACKGROUND.b, COLOR_BACKGROUND.a);
    SDL_RenderClear(renderer);
    
    // === Draw grid background ===
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_Rect grid_rect = {0, 0, GRID_WIDTH, GRID_HEIGHT};
    SDL_RenderFillRect(renderer, &grid_rect);
    
    // === Draw grid lines ===
    SDL_SetRenderDrawColor(renderer, COLOR_GRID.r, COLOR_GRID.g, 
                          COLOR_GRID.b, COLOR_GRID.a);
    for (int i = 0; i <= GRID_SIZE; ++i) {
        SDL_RenderDrawLine(renderer, i * CELL_SIZE, 0, 
                          i * CELL_SIZE, GRID_HEIGHT);
        SDL_RenderDrawLine(renderer, 0, i * CELL_SIZE, 
                          GRID_WIDTH, i * CELL_SIZE);
    }
    
    // === Draw food items ===
    for (const auto& food : env.get_food_list()) {
        SDL_Rect food_rect = {
            food.pos.x * CELL_SIZE, 
            food.pos.y * CELL_SIZE, 
            CELL_SIZE, 
            CELL_SIZE
        };
        SDL_RenderCopy(renderer, food_tex, NULL, &food_rect);
    }
    
    // === Draw agents ===
    for (const auto& agent : env.get_agents()) {
        if (agent.is_alive()) {
            SDL_Rect agent_rect = {
                agent.pos.x * CELL_SIZE, 
                agent.pos.y * CELL_SIZE, 
                CELL_SIZE, 
                CELL_SIZE
            };
            // Apply agent's unique color to texture
            SDL_SetTextureColorMod(agent_tex, agent.color.r, 
                                  agent.color.g, agent.color.b);
            SDL_RenderCopy(renderer, agent_tex, NULL, &agent_rect);
        }
    }

    // Highlight selected agent in debug mode
    if (debug_mode && selected_agent >= 0 && static_cast<size_t>(selected_agent) < env.get_agents().size()) {
        const AI& agent = env.get_agents()[static_cast<size_t>(selected_agent)];
        SDL_Rect highlight_rect = {
            agent.pos.x * CELL_SIZE,
            agent.pos.y * CELL_SIZE,
            CELL_SIZE,
            CELL_SIZE
        };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
        for (int i = 0; i < 3; ++i) {
            SDL_Rect frame = {highlight_rect.x - i, highlight_rect.y - i, highlight_rect.w + 2*i, highlight_rect.h + 2*i};
            SDL_RenderDrawRect(renderer, &frame);
        }
    }
    
    // === Draw sidebar background ===
    SDL_SetRenderDrawColor(renderer, COLOR_SIDEBAR.r, COLOR_SIDEBAR.g, 
                          COLOR_SIDEBAR.b, COLOR_SIDEBAR.a);
    SDL_Rect sidebar = {GRID_WIDTH, 0, SIDEBAR_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &sidebar);
    
    // === Draw sidebar header ===
    SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
    SDL_Rect header = {GRID_WIDTH, 0, SIDEBAR_WIDTH, 60};
    SDL_RenderFillRect(renderer, &header);
    
    // === Header text ===
    render_text(renderer, large_font, "AI Simulation Stats", 
                GRID_WIDTH + 10, 15, 
                {COLOR_HEADER.r, COLOR_HEADER.g, COLOR_HEADER.b, 255});
    
    // === Statistics text ===
    SDL_Color text_color = {COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b, 255};
    
    // Episode counter
    std::stringstream ep_info;
    ep_info << "Episode: " << env.get_episode();
    render_text(renderer, regular_font, ep_info.str(), 
                GRID_WIDTH + 10, 70, text_color);
    
    // Active agents
    std::stringstream agent_info;
    agent_info << "Active Agents: " << env.get_alive_count() << "/" << AGENT_COUNT;
    render_text(renderer, regular_font, agent_info.str(), 
                GRID_WIDTH + 10, 100, text_color);
    
    // Food count
    std::stringstream food_info;
    food_info << "Food: " << env.get_food_list().size() << "/" << FOOD_COUNT;
    render_text(renderer, regular_font, food_info.str(), 
                GRID_WIDTH + 10, 130, text_color);
    
    // Total food spawned
    std::stringstream total_food_info;
    total_food_info << "Total Food Spawned: " << env.get_total_food_spawned();
    render_text(renderer, regular_font, total_food_info.str(), 
                GRID_WIDTH + 10, 160, text_color);
    
    // Food eaten this episode
    std::stringstream current_food_info;
    current_food_info << "Food This Episode: " << env.get_current_episode_food();
    render_text(renderer, regular_font, current_food_info.str(), 
                GRID_WIDTH + 10, 190, text_color);
    
    // Average food per episode
    std::stringstream avg_food_info;
    avg_food_info << "Avg Food/Ep: " << std::fixed << std::setprecision(1) << env.get_avg_food_per_episode();
    render_text(renderer, regular_font, avg_food_info.str(), 
                GRID_WIDTH + 10, 220, text_color);
    
    // Average exploration rate
    std::stringstream explore_info;
    explore_info << "Avg Epsilon: " << std::fixed << std::setprecision(3) << env.get_avg_exploration_rate();
    render_text(renderer, regular_font, explore_info.str(), 
                GRID_WIDTH + 10, 250, text_color);
    
    // Time warp status
    std::stringstream warp_info;
    warp_info << "Time Warp: " << (time_warp_mode ? "ON" : "OFF") 
              << " (x" << time_warp_factor << ")";
    render_text(renderer, regular_font, warp_info.str(), 
                GRID_WIDTH + 10, 280, text_color);
    
    // === Agent Statistics Section ===
    render_cached_text(renderer, regular_font, "Agent Statistics", 
                GRID_WIDTH + 10, 310, 
                {COLOR_HEADER.r, COLOR_HEADER.g, COLOR_HEADER.b, 255});
    
    int agentY = 340;
    char agent_id = 'A';
    
    for (size_t i = 0; i < env.get_agents().size(); ++i, ++agent_id) {
        const AI& agent = env.get_agents()[i];
        
        // Agent background (dimmed if dead)
        SDL_SetRenderDrawColor(renderer, 60, 60, 80, 
                              agent.is_alive() ? 255 : 100);
        SDL_Rect agent_rect = {GRID_WIDTH + 5, agentY, 
                               SIDEBAR_WIDTH - 10, 75};
        SDL_RenderFillRect(renderer, &agent_rect);
        
        // Agent name with custom color
        std::stringstream name;
        name << "Agent " << agent_id;
        SDL_Color agent_color = agent.color;
        if (!agent.is_alive()) {
            agent_color.r *= 0.5;
            agent_color.g *= 0.5;
            agent_color.b *= 0.5;
        }
        render_text(renderer, regular_font, name.str(), 
                    GRID_WIDTH + 15, agentY + 5, agent_color);
        
        // Status (Active/Inactive)
        render_text(renderer, regular_font, 
                    agent.is_alive() ? "Active" : "Inactive", 
                    GRID_WIDTH + SIDEBAR_WIDTH - 80, agentY + 5, 
                    agent.is_alive() ? SDL_Color{0, 200, 0, 255} 
                                      : SDL_Color{200, 0, 0, 255});
        
        // Food eaten
        std::stringstream food_eaten;
        food_eaten << "Food: " << agent.total_food_eaten;
        render_text(renderer, regular_font, food_eaten.str(), 
                    GRID_WIDTH + 15, agentY + 30, text_color);
        
        // Energy bar
        render_text(renderer, regular_font, "Energy:", 
                    GRID_WIDTH + 15, agentY + 50, text_color);
        render_progress_bar(renderer, GRID_WIDTH + 80, agentY + 53, 
                            SIDEBAR_WIDTH - 100, 15, 
                            agent.get_energy_percentage(), 
                            COLOR_AGENT_ENERGY_BAR, COLOR_AGENT_ENERGY_BG);
        
        agentY += 85;
    }
    
    // === Controls Section ===
    render_cached_text(renderer, regular_font, "Controls", 
                GRID_WIDTH + 10, agentY + 10, 
                {COLOR_HEADER.r, COLOR_HEADER.g, COLOR_HEADER.b, 255});
    render_cached_text(renderer, regular_font, "ESC - Exit", 
                GRID_WIDTH + 15, agentY + 40, text_color);
    render_cached_text(renderer, regular_font, "R - Reset", 
                GRID_WIDTH + 15, agentY + 65, text_color);
    render_cached_text(renderer, regular_font, "Space - Pause/Resume", 
                GRID_WIDTH + 15, agentY + 90, text_color);
    render_cached_text(renderer, regular_font, "W - Toggle Warp", 
                GRID_WIDTH + 15, agentY + 115, text_color);
    render_cached_text(renderer, regular_font, "+/- - Warp Speed", 
                GRID_WIDTH + 15, agentY + 140, text_color);
    render_cached_text(renderer, regular_font, "0 - Reset Warp", 
                GRID_WIDTH + 15, agentY + 165, text_color);
    render_cached_text(renderer, regular_font, "D - Toggle Debug", 
                GRID_WIDTH + 15, agentY + 190, text_color);
    render_cached_text(renderer, regular_font, "Click Agent - Debug Info", 
                GRID_WIDTH + 15, agentY + 215, text_color);
    
    // === Debug Overlay ===
    if (debug_mode && selected_agent >= 0 && static_cast<size_t>(selected_agent) < env.get_agents().size()) {
        const AI& agent = env.get_agents()[static_cast<size_t>(selected_agent)];
        if (agent.is_alive()) {
            render_debug_overlay(renderer, agent, env.get_food_list(),
                                   GRID_WIDTH + 15, agentY + 240, regular_font);
        }
    }
}

// ============================================================================
// Debug Overlay
// ============================================================================

void render_debug_overlay(SDL_Renderer* renderer, const AI& agent, 
                          const std::vector<Food>& food_list,
                          int x, int y, TTF_Font* font) {
    if (!font || !renderer) return;
    
    SDL_Color text_color = {200, 200, 255, 255};
    SDL_Color header_color = {180, 180, 255, 255};
    int line_height = 20;
    int current_y = y;
    
    // Get debug info
    std::vector<double> state = agent.get_state_for_debug(food_list);
    std::vector<double> q_values = agent.get_last_q_values();
    
    // Header
    render_text(renderer, font, "=== DEBUG AGENT ===", x, current_y, header_color);
    current_y += line_height + 5;
    
    // Position
    std::stringstream pos_ss;
    pos_ss << "Pos: (" << agent.pos.x << ", " << agent.pos.y << ")";
    render_text(renderer, font, pos_ss.str(), x, current_y, text_color);
    current_y += line_height;
    
    // Energy
    std::stringstream energy_ss;
    energy_ss << "Energy: " << agent.energy << "/" << MAX_ENERGY;
    render_text(renderer, font, energy_ss.str(), x, current_y, text_color);
    current_y += line_height;
    
    // Exploration rate
    std::stringstream explore_ss;
    explore_ss << "Epsilon: " << std::fixed << std::setprecision(3) << agent.get_explore_rate();
    render_text(renderer, font, explore_ss.str(), x, current_y, text_color);
    current_y += line_height + 5;
    
    // State vector
    render_text(renderer, font, "State Vector:", x, current_y, header_color);
    current_y += line_height;
    for (size_t i = 0; i < state.size() && i < 12; ++i) {
        std::stringstream ss;
        ss << "  [" << i << "]: " << std::fixed << std::setprecision(3) << state[i];
        render_text(renderer, font, ss.str(), x, current_y, text_color);
        current_y += line_height;
    }
    
    current_y += 5;
    
    // Q-Values
    render_text(renderer, font, "Q-Values:", x, current_y, header_color);
    current_y += line_height;
    const char* action_names[] = {"LEFT", "RIGHT", "UP", "DOWN"};
    for (int i = 0; i < 4; ++i) {
        std::stringstream ss;
        ss << "  " << action_names[i] << ": " << std::fixed << std::setprecision(3) 
           << (static_cast<size_t>(i) < q_values.size() ? q_values[static_cast<size_t>(i)] : 0.0);
        render_text(renderer, font, ss.str(), x, current_y, text_color);
        current_y += line_height;
    }
}
