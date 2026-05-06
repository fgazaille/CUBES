/**
 * @file sdl_utils.cpp
 * @brief Implementation of SDL2 utility functions.
 * 
 * Handles initialization, resource loading, and cleanup for SDL2 subsystems.
 */

#include "sdl_utils.hpp"
#include <iostream>
#include <cstdlib>

// ============================================================================
// SDL Initialization
// ============================================================================

bool init_sdl(SDL_Window** window, SDL_Renderer** renderer, 
               const char* title, int width, int height) {
    
    // Initialize SDL core video and audio subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize TTF font subsystem
    if (TTF_Init() == -1) {
        std::cerr << "TTF Initialization Failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    
    // Initialize SDL_image for PNG support
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        std::cerr << "IMG Initialization Failed: " << IMG_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    
    // Initialize SDL_mixer for audio (non-fatal if fails)
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Warning: Mix Initialization Failed: " << Mix_GetError() << std::endl;
        // Continue without audio
    }
    
    // Create main window (centered on screen)
    *window = SDL_CreateWindow(title, 
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width, height, SDL_WINDOW_SHOWN);
    if (!*window) {
        std::cerr << "Window Creation Failed: " << SDL_GetError() << std::endl;
        Mix_CloseAudio();
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    
    // Create hardware-accelerated renderer with vsync
    *renderer = SDL_CreateRenderer(*window, -1, 
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer) {
        std::cerr << "Renderer Creation Failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(*window);
        Mix_CloseAudio();
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    
    // Enable alpha blending for transparency support
    SDL_SetRenderDrawBlendMode(*renderer, SDL_BLENDMODE_BLEND);
    
    // Load and set window icon (if available)
    SDL_Surface* icon = IMG_Load("icon.png");
    if (icon) {
        SDL_SetWindowIcon(*window, icon);
        SDL_FreeSurface(icon);
    }
    
    return true;
}

// ============================================================================
// SDL Cleanup
// ============================================================================

void quit_sdl(SDL_Window* window, SDL_Renderer* renderer, 
               TTF_Font* large_font, TTF_Font* regular_font) {
    
    // Close fonts
    if (regular_font) TTF_CloseFont(regular_font);
    if (large_font) TTF_CloseFont(large_font);
    
    // Destroy renderer and window
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    
    // Quit all SDL subsystems
    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

// ============================================================================
// Font Loading
// ============================================================================

TTF_Font* load_font(const char* font_path, int size) {
    // Try common font paths for different OS
    const char* font_paths[] = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",    // macOS
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // Linux
        "/usr/share/fonts/TTF/DejaVuSans.ttf",            // Linux alt
        "C:\\Windows\\Fonts\\arial.ttf",                   // Windows
        "arial.ttf",                                        // Relative
        "fonts/arial.ttf"                                    // Project fonts
    };
    
    // Try each path
    TTF_Font* font = nullptr;
    for (const char* path : font_paths) {
        font = TTF_OpenFont(path, size);
        if (font) break;
    }
    
    // Fallback to provided path
    if (!font) {
        font = TTF_OpenFont(font_path, size);
    }
    
    return font;
}

// ============================================================================
// Texture Loading
// ============================================================================

SDL_Texture* load_scaled_texture(SDL_Renderer* renderer, 
                                  const std::string& path, 
                                  int target_size) {
    SDL_Texture* texture = nullptr;
    
    // Try to load image
    SDL_Surface* surface = IMG_Load(path.c_str());
    
    if (!surface) {
        // Create fallback colored square
        SDL_Surface* fallback = SDL_CreateRGBSurface(0, target_size, target_size, 32, 0,0,0,0);
        if (fallback) {
            // Orange color for food as fallback
            SDL_FillRect(fallback, NULL, SDL_MapRGB(fallback->format, 200, 150, 0));
            texture = SDL_CreateTextureFromSurface(renderer, fallback);
            SDL_FreeSurface(fallback);
        }
    } else {
        // Scale the loaded surface to target size
        SDL_Surface* scaled = SDL_CreateRGBSurface(0, target_size, target_size, 32, 0,0,0,0);
        if (scaled) {
            SDL_Rect rect = {0, 0, target_size, target_size};
            SDL_BlitScaled(surface, NULL, scaled, &rect);
            texture = SDL_CreateTextureFromSurface(renderer, scaled);
            SDL_FreeSurface(scaled);
        } else {
            // Fallback to unscaled texture
            texture = SDL_CreateTextureFromSurface(renderer, surface);
        }
        SDL_FreeSurface(surface);
    }
    
    return texture;
}
