#include "renderer.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace {
    Texture2D agent_tex{};
    Texture2D food_tex{};
    Font game_font{};
    bool resources_loaded = false;

    void draw_rounded_rect(int x, int y, int w, int h, int /*radius*/, Color color) {
        DrawRectangleRounded({(float)x, (float)y, (float)w, (float)h}, 0.15f, 8, color);
    }
}

void init_renderer(const std::string& asset_path) {
    Image player_img = LoadImage((asset_path + "Player.png").c_str());
    Image food_img   = LoadImage((asset_path + "Food.png").c_str());

    if (player_img.data) ImageResize(&player_img, CELL_SIZE, CELL_SIZE);
    if (food_img.data)   ImageResize(&food_img,   CELL_SIZE, CELL_SIZE);

    agent_tex = LoadTextureFromImage(player_img);
    food_tex  = LoadTextureFromImage(food_img);
    if (player_img.data) UnloadImage(player_img);
    if (food_img.data)   UnloadImage(food_img);

    game_font = LoadFont((asset_path + "Futura.ttf").c_str());
    if (game_font.texture.id == 0) {
        game_font = GetFontDefault();
    }

    SetTextureFilter(agent_tex, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(food_tex, TEXTURE_FILTER_BILINEAR);

    resources_loaded = true;
}

void close_renderer() {
    if (resources_loaded) {
        if (agent_tex.id > 0) UnloadTexture(agent_tex);
        if (food_tex.id > 0)  UnloadTexture(food_tex);
        if (game_font.texture.id > 0 &&
            game_font.texture.id != GetFontDefault().texture.id)
            UnloadFont(game_font);
        resources_loaded = false;
    }
}

static void draw_text(const std::string& text, int x, int y, int size, Color color) {
    DrawTextEx(game_font, text.c_str(), {(float)x, (float)y}, (float)size, 1, color);
}

void render_progress_bar(int x, int y, int w, int h,
                         float percentage, Color fill, Color bg) {
    draw_rounded_rect(x, y, w, h, 6, bg);
    if (percentage > 0) {
        int fw = (int)(w * std::max(0.0f, std::min(1.0f, percentage)));
        draw_rounded_rect(x, y, fw, h, 6, fill);
    }
    DrawRectangleLines(x, y, w, h, UI::TEXT_DIM);
}

// ── Helpers ──────────────────────────────────────────────────────────────────

static void render_agent_energy_bar(int x, int y, int w, int h, float pct) {
    DrawRectangle(x, y, w, h, UI::ENERGY_BG);
    DrawRectangle(x, y, (int)(w * pct), h, UI::ENERGY_BAR);
}

static void render_sidebar_header(int y, const std::string& text) {
    draw_text(text, GRID_WIDTH + 15, y, 18, UI::ACCENT);
}

// ── Main render ──────────────────────────────────────────────────────────────

void render_environment(const Environment& env,
                        bool time_warp_mode, double time_warp_factor,
                        bool debug_mode, int selected_agent) {

    ClearBackground(UI::BACKGROUND);

    // ── Grid ────────────────────────────────────────────────────────────────
    DrawRectangle(0, 0, GRID_WIDTH, GRID_HEIGHT, UI::GRID_BG);

    for (int i = 0; i <= GRID_SIZE; ++i) {
        int x = i * CELL_SIZE;
        DrawLine(x, 0, x, GRID_HEIGHT, UI::GRID_LINE);
        DrawLine(0, x, GRID_WIDTH, x, UI::GRID_LINE);
    }

    // ── Food ────────────────────────────────────────────────────────────────
    for (const auto& food : env.get_food_list()) {
        int fx = food.pos.x * CELL_SIZE;
        int fy = food.pos.y * CELL_SIZE;
        float pulse = 0.80f + 0.20f * std::sin(GetTime() * 3.0f + food.pos.x * 7.0f + food.pos.y * 13.0f);
        int pad = (int)(CELL_SIZE * (1.0f - 0.7f * pulse) / 2.0f);
        int side = CELL_SIZE - pad * 2;
        DrawRectangle(fx + pad, fy + pad, side, side, UI::FOOD_COLOUR);
    }

    // ── Agents ──────────────────────────────────────────────────────────────
    for (const auto& agent : env.get_agents()) {
        if (!agent.is_alive()) continue;

        int ax = agent.pos.x * CELL_SIZE;
        int ay = agent.pos.y * CELL_SIZE;

        // agent cube
        int pad = CELL_SIZE / 8;
        int side = CELL_SIZE - pad * 2;
        DrawRectangle(ax + pad, ay + pad, side, side, agent.color);
        DrawRectangleLines(ax + pad, ay + pad, side, side, Fade(agent.color, 0.3f));

        // energy bar below agent
        int bar_w = CELL_SIZE - 4;
        int bar_h = std::max(3, CELL_SIZE / 8);
        render_agent_energy_bar(ax + 2, ay + CELL_SIZE - bar_h - 2, bar_w, bar_h,
                                agent.get_energy_percentage());
    }

    // ── Highlight selected agent (debug) ────────────────────────────────────
    if (debug_mode && selected_agent >= 0 && (size_t)selected_agent < env.get_agents().size()) {
        const AI& agent = env.get_agents()[(size_t)selected_agent];
        int ax = agent.pos.x * CELL_SIZE;
        int ay = agent.pos.y * CELL_SIZE;
        DrawRectangleLinesEx({(float)ax - 2, (float)ay - 2,
                              (float)CELL_SIZE + 4, (float)CELL_SIZE + 4}, 2, UI::ACCENT);
    }

    // ── Sidebar ─────────────────────────────────────────────────────────────
    DrawRectangle(GRID_WIDTH, 0, SIDEBAR_WIDTH, SCREEN_HEIGHT, UI::SIDEBAR_BG);

    // Header bar
    DrawRectangle(GRID_WIDTH, 0, SIDEBAR_WIDTH, 50, UI::SIDEBAR_HDR);
    draw_text("AI Simulation Stats", GRID_WIDTH + 15, 14, 18, UI::ACCENT);

    // Stats
    auto stat_line = [&](int y, const std::string& label, const std::string& value) {
        draw_text(label, GRID_WIDTH + 15, y, 14, UI::TEXT_DIM);
        draw_text(value, GRID_WIDTH + SIDEBAR_WIDTH - 15 - MeasureTextEx(game_font, value.c_str(), 14, 1).x, y, 14, UI::TEXT);
    };

    int sy = 65;
    {
        std::stringstream ss; ss << env.get_episode();
        stat_line(sy, "Episode", ss.str()); sy += 20;
    }
    {
        std::stringstream ss; ss << env.get_alive_count() << "/" << AGENT_COUNT;
        stat_line(sy, "Active Agents", ss.str()); sy += 20;
    }
    {
        std::stringstream ss; ss << env.get_food_list().size() << "/" << FOOD_COUNT;
        stat_line(sy, "Food", ss.str()); sy += 20;
    }
    {
        std::stringstream ss; ss << env.get_total_food_spawned();
        stat_line(sy, "Total Food Spawned", ss.str()); sy += 20;
    }
    {
        std::stringstream ss; ss << env.get_current_episode_food();
        stat_line(sy, "Food This Episode", ss.str()); sy += 20;
    }
    {
        std::stringstream ss; ss << std::fixed << std::setprecision(1) << env.get_avg_food_per_episode();
        stat_line(sy, "Avg Food/Ep", ss.str()); sy += 20;
    }
    {
        std::stringstream ss; ss << std::fixed << std::setprecision(3) << env.get_avg_exploration_rate();
        stat_line(sy, "Avg Epsilon", ss.str()); sy += 20;
    }
    {
        std::stringstream ss; ss << (time_warp_mode ? "ON" : "OFF") << " (x" << time_warp_factor << ")";
        stat_line(sy, "Time Warp", ss.str()); sy += 25;
    }

    // ── Agent Statistics section ───────────────────────────────────────────
    sy += 5;
    render_sidebar_header(sy, "Agent Statistics");
    sy += 25;

    char agent_id = 'A';
    for (size_t i = 0; i < env.get_agents().size(); ++i, ++agent_id) {
        const AI& agent = env.get_agents()[i];

        // card bg
        int card_h = 70;
        draw_rounded_rect(GRID_WIDTH + 8, sy, SIDEBAR_WIDTH - 16, card_h, 6, UI::SIDEBAR_HDR);

        // agent name
        Color name_col = agent.color;
        if (!agent.is_alive()) name_col = Fade(name_col, 0.4f);
        std::string name = "Agent "; name += agent_id;
        draw_text(name, GRID_WIDTH + 18, sy + 6, 14, name_col);

        // status badge
        std::string status = agent.is_alive() ? "Active" : "Inactive";
        Color status_col = agent.is_alive() ? UI::COL_GREEN : UI::COL_RED;
        int sw = (int)MeasureTextEx(game_font, status.c_str(), 12, 1).x;
        draw_text(status, GRID_WIDTH + SIDEBAR_WIDTH - 20 - sw, sy + 6, 12, status_col);

        // food
        std::stringstream fs; fs << "Food: " << agent.total_food_eaten;
        draw_text(fs.str(), GRID_WIDTH + 18, sy + 28, 13, UI::TEXT);

        // energy bar
        draw_text("Energy", GRID_WIDTH + 18, sy + 48, 12, UI::TEXT_DIM);
        render_agent_energy_bar(GRID_WIDTH + 80, sy + 49,
                                SIDEBAR_WIDTH - 100, 12,
                                agent.get_energy_percentage());

        sy += card_h + 6;
    }

    // ── Controls section ──────────────────────────────────────────────────
    sy += 10;
    render_sidebar_header(sy, "Controls");
    sy += 22;

    const char* controls[] = {
        "ESC  Exit", "R  Reset", "Space  Pause",
        "W  Warp", "+/-  Speed", "0  Reset Warp",
        "D  Debug", "S  Save Brain", "L  Load Brain"
    };
    int col_w = SIDEBAR_WIDTH / 3;
    for (int c = 0; c < 9; ++c) {
        int cx = GRID_WIDTH + 10 + (c % 3) * col_w;
        int cy = sy + (c / 3) * 18;
        draw_text(controls[c], cx, cy, 12, UI::TEXT_DIM);
    }

    // ── Debug overlay ─────────────────────────────────────────────────────
    if (debug_mode && selected_agent >= 0 && (size_t)selected_agent < env.get_agents().size()) {
        const AI& agent = env.get_agents()[(size_t)selected_agent];
        if (agent.is_alive())
            render_debug_overlay(agent, env.get_food_list(),
                                 GRID_WIDTH + 15, sy + 60);
    }
}

// ── Debug overlay ────────────────────────────────────────────────────────────

void render_debug_overlay(const AI& agent,
                           const std::vector<Food>& food_list,
                           int x, int y) {

    draw_text("=== DEBUG AGENT ===", x, y, 14, UI::ACCENT);
    int cy = y + 22;

    std::vector<double> state = agent.get_state_for_debug(food_list);
    std::vector<double> q_values = agent.get_last_q_values();

    auto line = [&](const std::string& s) {
        draw_text(s, x + 4, cy, 13, UI::TEXT);
        cy += 18;
    };

    std::stringstream ps; ps << "Pos: (" << agent.pos.x << ", " << agent.pos.y << ")"; line(ps.str());
    std::stringstream es; es << "Energy: " << agent.energy << "/" << MAX_ENERGY; line(es.str());
    std::stringstream eps; eps << "Epsilon: " << std::fixed << std::setprecision(3) << agent.get_explore_rate(); line(eps.str());

    cy += 4;
    draw_text("State Vector:", x + 4, cy, 13, UI::ACCENT);
    cy += 18;
    for (size_t i = 0; i < state.size() && i < 12; ++i) {
        std::stringstream ss; ss << "  [" << i << "]: " << std::fixed << std::setprecision(3) << state[i]; line(ss.str());
    }

    cy += 4;
    draw_text("Q-Values:", x + 4, cy, 13, UI::ACCENT);
    cy += 18;
    const char* anames[] = {"LEFT", "RIGHT", "UP", "DOWN"};
    for (int i = 0; i < 4; ++i) {
        std::stringstream ss; ss << "  " << anames[i] << ": " << std::fixed << std::setprecision(3)
                                 << ((size_t)i < q_values.size() ? q_values[(size_t)i] : 0.0); line(ss.str());
    }
}
