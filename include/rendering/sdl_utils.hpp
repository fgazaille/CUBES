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

#ifndef SDL_UTILS_HPP
#define SDL_UTILS_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <string>

/**
 * @brief Initialize all SDL2 subsystems.
 * 
 * Initializes SDL2 core, TTF, Image, and Mixer.
 * Creates the main window and renderer with vsync enabled.
 * 
 * @param[out] window Pointer to SDL_Window pointer (set on success)
 * @param[out] renderer Pointer to SDL_Renderer pointer (set on success)
 * @param title Window title
 * @param width Window width in pixels
 * @param height Window height in pixels
 * @return true if all initializations succeeded
 * @return false if any initialization failed (cleanup is handled internally)
 */
bool init_sdl(SDL_Window** window, SDL_Renderer** renderer, 
               const char* title, int width, int height);

/**
 * @brief Clean shutdown of all SDL2 subsystems.
 * 
 * @param window SDL_Window to destroy (can be nullptr)
 * @param renderer SDL_Renderer to destroy (can be nullptr)
 * @param large_font TTF_Font to close (can be nullptr)
 * @param regular_font TTF_Font to close (can be nullptr)
 */
void quit_sdl(SDL_Window* window, SDL_Renderer* renderer, 
               TTF_Font* large_font, TTF_Font* regular_font);

/**
 * @brief Load a TTF font with automatic fallback paths.
 * 
 * Tries multiple common font paths before falling back to the
 * provided path. Supports macOS, Linux, and Windows.
 * 
 * @param font_path Fallback font path
 * @param size Font size in points
 * @return TTF_Font pointer (caller must call TTF_CloseFont), or nullptr on failure
 */
TTF_Font* load_font(const char* font_path, int size);

/**
 * @brief Load an image and scale it to target size.
 * 
 * If loading fails, creates a fallback colored square.
 * 
 * @param renderer SDL_Renderer for texture creation
 * @param path Path to image file
 * @param target_size Desired width and height in pixels
 * @return SDL_Texture pointer (caller must call SDL_DestroyTexture), or nullptr on failure
 */
SDL_Texture* load_scaled_texture(SDL_Renderer* renderer, 
                                  const std::string& path, 
                                  int target_size);

#endif // SDL_UTILS_HPP
