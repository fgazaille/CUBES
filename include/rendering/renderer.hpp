/**
 * @file renderer.hpp
 * @brief Main simulation rendering functions.
 * 
 * Handles rendering of:
 * - Grid and background
 * - Food items
 * - AI agents with unique colors
 * - Sidebar with statistics
 * - Control instructions
 */

#pragma once

#include "config.hpp"
#include "environment.hpp"
#include "sdl_utils.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>

// ============================================================================
// Text Cache for SDL Rendering
// ============================================================================

/**
 * @brief Cached text texture to avoid reallocating per frame.
 */
struct CachedText {
    SDL_Texture* texture;
    int width;
    int height;
    
    CachedText() : texture(nullptr), width(0), height(0) {}
    CachedText(SDL_Texture* tex, int w, int h) : texture(tex), width(w), height(h) {}
};

/**
 * @brief Simple text cache using hash of text content.
 * 
 * Note: For simplicity, this uses a static cache. 
 * Call clear_text_cache() when changing fonts or on cleanup.
 */
void clear_text_cache();
void render_cached_text(SDL_Renderer* renderer, TTF_Font* font, 
                        const std::string& text, int x, int y, SDL_Color color);

/**
 * @brief RGBA color structure.
 */
struct Color { Uint8 r, g, b, a; };

// ============================================================================
// Color Constants
// ============================================================================

const Color COLOR_BACKGROUND = {20, 20, 30, 255};       ///< Main background
const Color COLOR_GRID = {60, 60, 80, 255};            ///< Grid lines
const Color COLOR_SIDEBAR = {40, 40, 60, 255};         ///< Sidebar background
const Color COLOR_TEXT = {220, 220, 240, 255};         ///< Default text
const Color COLOR_HEADER = {180, 180, 255, 255};       ///< Header text
const Color COLOR_HIGHLIGHT = {100, 100, 140, 255};    ///< Highlight color
const Color COLOR_AGENT_ENERGY_BAR = {0, 180, 100, 255};   ///< Energy bar fill
const Color COLOR_AGENT_ENERGY_BG = {60, 60, 60, 255};    ///< Energy bar background

/**
 * @brief Render text to the screen.
 */
void render_text(SDL_Renderer* renderer, TTF_Font* font, 
                    const std::string& text, int x, int y, SDL_Color color);

/**
 * @brief Render a progress bar.
 */
void render_progress_bar(SDL_Renderer* renderer, int x, int y, int w, int h,
                        float percentage, const Color& fill, const Color& bg);

/**
 * @brief Render the entire simulation environment.
 * 
 * @param renderer SDL renderer
 * @param env Environment to render
 * @param agent_tex Texture for agent sprite
 * @param food_tex Texture for food sprite
 * @param large_font Font for headers
 * @param regular_font Font for body text
 * @param time_warp_mode Whether time warp is active
 * @param time_warp_factor Current warp speed multiplier
 * @param debug_mode Whether to show debug overlay
 * @param selected_agent Index of agent to show debug info for (-1 for none)
 */
void render_environment(SDL_Renderer* renderer, const Environment& env,
                        SDL_Texture* agent_tex, SDL_Texture* food_tex,
                        TTF_Font* large_font, TTF_Font* regular_font,
                        bool time_warp_mode, double time_warp_factor,
                        bool debug_mode = false, int selected_agent = -1);

/**
 * @brief Render debug overlay for an agent.
 * 
 * @param renderer SDL renderer
 * @param agent Agent to debug
 * @param food_list Current food positions
 * @param x X position for debug panel
 * @param y Y position for debug panel
 * @param font Font for text
 */
void render_debug_overlay(SDL_Renderer* renderer, const AI& agent, 
                           const std::vector<Food>& food_list,
                           int x, int y, TTF_Font* font);
