#pragma once

#include "raylib.h"
#include "environment.hpp"
#include <string>
#include <algorithm>
#include <cmath>

// Scale factor from OS DPI settings (1.0 on standard displays, >1 on HiDPI).
// Works on all platforms: uses the render/screen ratio on Windows/Wayland/macOS
// (most reliable), falls back to physical-DPI-based detection on X11.
// Always picks the max of x/y for rotated or non-square-pixel displays.
inline float os_scale() {
    Vector2 dpi = GetWindowScaleDPI();
    float window_scale = std::max(dpi.x, dpi.y);
    float render_ratio = (float)GetRenderWidth() / (float)std::max(1, GetScreenWidth());
    if (render_ratio > 1.0f) return render_ratio;
    return std::max(1.0f, window_scale);
}

inline float sp(int pt) { return std::round(pt * os_scale()); }

void init_renderer(const std::string& asset_path = "assets/");
void close_renderer();

void render_environment(const Environment& env,
                        bool time_warp_mode, double time_warp_factor,
                        bool debug_mode = false, int selected_agent = -1);

void render_debug_overlay(const AI& agent,
                           const std::vector<Food>& food_list,
                           int x, int y);

// Progress bar helper
void render_progress_bar(int x, int y, int w, int h,
                         float percentage, Color fill, Color bg);

// Colour palette
namespace UI {
    inline const Color BACKGROUND   = {13, 17, 23, 255};
    inline const Color GRID_BG      = {22, 27, 34, 255};
    inline const Color GRID_LINE    = {48, 54, 61, 255};
    inline const Color SIDEBAR_BG   = {22, 27, 34, 255};
    inline const Color SIDEBAR_HDR  = {30, 36, 44, 255};
    inline const Color TEXT         = {201, 209, 217, 255};
    inline const Color TEXT_DIM     = {139, 148, 158, 255};
    inline const Color ACCENT       = {88, 166, 255, 255};
    inline const Color COL_GREEN    = {63, 185, 80, 255};
    inline const Color COL_RED      = {248, 81, 73, 255};
    inline const Color COL_GOLD     = {210, 153, 34, 255};
    inline const Color FOOD_COLOUR  = {86, 201, 100, 255};
    inline const Color WALL_FILL    = {36, 32, 28, 255};
    inline const Color WALL_EDGE    = {54, 48, 42, 255};
    inline const Color ENERGY_BAR   = {48, 160, 80, 255};
    inline const Color ENERGY_BG    = {48, 54, 61, 255};
}
