/**
 * @file sdl_utils.hpp
 * @brief Utility functions for SDL2 initialization and resource management.
 * 
 * Provides helper functions for:
 * - Initializing SDL2, SDL2_image, SDL2_ttf, SDL2_mixer
 * - Loading fonts with fallback paths
 * - Loading and scaling textures
 * - Clean shutdown of all SDL subsystems
 */

#pragma once

#include "config.hpp"
#include "rendering/renderer.hpp"
#include "menu/menu.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <memory>
#include <string>

// ============================================================================
// SDL Initialization
// ============================================================================

/**
 * @brief Initialize SDL2 subsystems (call once at startup).
 * @return true if all subsystems initialized successfully.
 */
bool init_sdl_subsystems();

/**
 * @brief Create a window and renderer.
 * 
 * Creates a window and renderer with vsync enabled.
 * 
 * @note Call init_sdl_subsystem() first.
 * 
 * @param title Window title
 * @param width Window width in pixels
 * @param height Window height in pixels
 * @return Pair of window and renderer pointers (both nullptr on failure)
 */
std::pair<SDL_Window*, SDL_Renderer*> create_window_and_renderer(const char* title, int width, int height);

/**
 * @brief Initialize all SDL2 subsystems and create window/renderer.
 * 
 * Convenience function that calls init_sdl_subsystem() and then
 * create_window_and_renderer().
 * 
 * @param title Window title
 * @param width Window width in pixels
 * @param height Window height in pixels
 * @return Pair of window and renderer pointers (both nullptr on failure)
 */
std::pair<SDL_Window*, SDL_Renderer*> init_sdl(const char* title, int width, int height);

/**
 * @brief Clean shutdown of all SDL2 subsystems.
 * 
 * Note: With smart pointers, explicit cleanup is usually not needed.
 * This function is provided for backward compatibility.
 */
void quit_sdl();

// ============================================================================
// Font Loading
// ============================================================================

/**
 * @brief Load a TTF font with automatic fallback paths.
 * 
 * Tries multiple common font paths before falling back to the
 * provided path. Supports macOS, Linux, and Windows.
 * 
 * @param font_path Fallback font path
 * @param size Font size in points
 * @return TTF_Font pointer (nullptr on failure)
 */
TTF_Font* load_font(const char* font_path, int size);

// ============================================================================
// Texture Loading
// ============================================================================

/**
 * @brief Load an image and scale it to target size.
 * 
 * If loading fails, creates a fallback colored square.
 * 
 * @param renderer SDL_Renderer for texture creation
 * @param path Path to image file
 * @param target_size Desired width and height in pixels
 * @return SDL_Texture pointer (nullptr on failure)
 */
SDL_Texture* load_scaled_texture(SDL_Renderer* renderer, 
                                   const std::string& path, 
                                   int target_size);
