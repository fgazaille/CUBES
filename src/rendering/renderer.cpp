#include "renderer.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <set>
#include <climits>

namespace {
    Texture2D agent_tex{};
    Texture2D food_tex{};
    Font game_font{};
    bool resources_loaded = false;

    constexpr int MAX_HISTORY = 500;
    struct FoodPoint { int episode; int food; };
    std::vector<FoodPoint> agent_history[16];
    int last_agent_count = -1;

    void clear_food_history() {
        for (auto& h : agent_history) h.clear();
        last_agent_count = -1;
    }

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

    game_font = LoadFontEx((asset_path + "Futura.ttf").c_str(), 32, nullptr, 0);
    if (game_font.texture.id == 0)
        game_font = GetFontDefault();

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
    RuntimeConfig& cfg = RuntimeConfig::instance();
    int cell_size = std::min(SCREEN_WIDTH - SIDEBAR_WIDTH, SCREEN_HEIGHT) / cfg.grid_size;
    int grid_pixel_width = cell_size * cfg.grid_size;

    // ── Record food history ───────────────────────────────────────────────
    {
        const auto& agents = env.get_agents();
        int current_count = (int)agents.size();
        if (current_count != last_agent_count) {
            clear_food_history();
            last_agent_count = current_count;
        }
        for (size_t i = 0; i < agents.size() && i < 16; ++i) {
            int food = agents[i].total_food_eaten;
            auto& hist = agent_history[i];
            int idx = hist.empty() ? 0 : hist.back().episode + 1;
            hist.push_back({idx, food});
            if (hist.size() > MAX_HISTORY)
                hist.erase(hist.begin());
        }
    }

    // ── Grid ────────────────────────────────────────────────────────────────
    DrawRectangle(0, 0, grid_pixel_width, grid_pixel_width, UI::GRID_BG);

    for (int i = 0; i <= cfg.grid_size; ++i) {
        int x = i * cell_size;
        DrawLine(x, 0, x, grid_pixel_width, UI::GRID_LINE);
        DrawLine(0, x, grid_pixel_width, x, UI::GRID_LINE);
    }

    // ── Walls ───────────────────────────────────────────────────────────────
    {
        int gs = cfg.grid_size;
        for (int x = 0; x < gs; ++x) {
            for (int y = 0; y < gs; ++y) {
                if (x != 0 && x != gs - 1 && y != 0 && y != gs - 1) continue;
                int wx = x * cell_size, wy = y * cell_size;
                DrawRectangle(wx, wy, cell_size, cell_size, UI::WALL_FILL);
                DrawRectangleLines(wx, wy, cell_size, cell_size, UI::WALL_EDGE);
            }
        }
    }

    // ── Food ────────────────────────────────────────────────────────────────
    for (const auto& food : env.get_food_list()) {
        int fx = food.pos.x * cell_size;
        int fy = food.pos.y * cell_size;
        float pulse = 0.75f + 0.25f * std::sin(GetTime() * 3.0f + food.pos.x * 7.0f + food.pos.y * 13.0f);
        int side = (int)(cell_size * 0.7f * pulse);
        int pad = (cell_size - side) / 2;
        Color fc = UI::FOOD_COLOUR;
        fc.a = (unsigned char)(180 + 75 * pulse);
        DrawRectangle(fx + pad, fy + pad, side, side, fc);
        if (side > 4) {
            Color glow = UI::FOOD_COLOUR;
            glow.a = 30;
            DrawRectangle(fx + pad - 1, fy + pad - 1, side + 2, side + 2, glow);
        }
    }

    // ── Agents ──────────────────────────────────────────────────────────────
    for (const auto& agent : env.get_agents()) {
        if (!agent.is_alive()) continue;

        int ax = agent.pos.x * cell_size;
        int ay = agent.pos.y * cell_size;

        int pad = cell_size / 8;
        int side = cell_size - pad * 2;
        DrawRectangle(ax + pad, ay + pad, side, side, agent.color);
        DrawRectangleLines(ax + pad, ay + pad, side, side, Fade(agent.color, 0.3f));

        int bar_w = cell_size - 4;
        int bar_h = std::max(3, cell_size / 8);
        render_agent_energy_bar(ax + 2, ay + cell_size - bar_h - 2, bar_w, bar_h,
                                agent.get_energy_percentage());
    }

    // ── Highlight selected agent (debug) ────────────────────────────────────
    if (debug_mode && selected_agent >= 0 && (size_t)selected_agent < env.get_agents().size()) {
        const AI& agent = env.get_agents()[(size_t)selected_agent];
        int ax = agent.pos.x * cell_size;
        int ay = agent.pos.y * cell_size;
        DrawRectangleLinesEx({(float)ax - 2, (float)ay - 2,
                              (float)cell_size + 4, (float)cell_size + 4}, 2, UI::ACCENT);
    }

    // ── Sidebar ─────────────────────────────────────────────────────────────
    DrawRectangle(grid_pixel_width, 0, SIDEBAR_WIDTH, SCREEN_HEIGHT, UI::SIDEBAR_BG);
    if (grid_pixel_width + SIDEBAR_WIDTH < SCREEN_WIDTH)
        DrawRectangle(grid_pixel_width + SIDEBAR_WIDTH, 0,
                      SCREEN_WIDTH - grid_pixel_width - SIDEBAR_WIDTH, SCREEN_HEIGHT,
                      UI::SIDEBAR_BG);

    DrawRectangle(grid_pixel_width, 0, SIDEBAR_WIDTH, 50, UI::SIDEBAR_HDR);
    draw_text("AI Simulation Stats", grid_pixel_width + 15, 14, 18, UI::ACCENT);

    auto stat_line = [&](int y, const std::string& label, const std::string& value) {
        draw_text(label, grid_pixel_width + 15, y, 14, UI::TEXT_DIM);
        draw_text(value, grid_pixel_width + SIDEBAR_WIDTH - 15 - MeasureTextEx(game_font, value.c_str(), 14, 1).x, y, 14, UI::TEXT);
    };

    int sy = 65;
    {
        std::stringstream ss; ss << env.get_episode();
        stat_line(sy, "Episode", ss.str()); sy += 20;
    }
    {
        std::stringstream ss; ss << env.get_alive_count() << "/" << cfg.agent_count;
        stat_line(sy, "Active Agents", ss.str()); sy += 20;
    }
    {
        std::stringstream ss; ss << env.get_food_list().size() << "/" << cfg.food_count;
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

    // ── Learning Progress graph ───────────────────────────────────────────
    sy += 8;
    render_sidebar_header(sy, "Learning Progress");
    sy += 22;

    int graph_x = grid_pixel_width + 10;
    int graph_y = sy;
    int graph_w = SIDEBAR_WIDTH - 20;
    int graph_h = 130;

    draw_rounded_rect(graph_x, graph_y, graph_w, graph_h, 4, UI::SIDEBAR_HDR);

    // Find max food across all history for Y axis scaling
    int max_food = 10;
    int min_ep = INT_MAX;
    int max_ep = 1;
    bool any_data = false;
    for (const auto& hist : agent_history) {
        if (hist.empty()) continue;
        any_data = true;
        for (const auto& p : hist) {
            if (p.food > max_food) max_food = p.food;
            if (p.episode > max_ep) max_ep = p.episode;
        }
        if (hist.front().episode < min_ep) min_ep = hist.front().episode;
    }
    max_food = std::max(10, ((max_food / 10) + 1) * 10);

    // Build best-food-per-episode trace (across all recorded episodes)
    std::vector<FoodPoint> best_trace;
    {
        // Collect unique episodes from all histories
        std::set<int> all_eps;
        for (const auto& h : agent_history)
            for (const auto& p : h)
                all_eps.insert(p.episode);
        for (int ep : all_eps) {
            int best = 0;
            for (const auto& h : agent_history) {
                for (const auto& p : h) {
                    if (p.episode == ep && p.food > best)
                        best = p.food;
                }
            }
            best_trace.push_back({ep, best});
        }
    }

    if (any_data) {
        int plot_x = graph_x + 30;
        int plot_y = graph_y + 8;
        int plot_w = graph_w - 40;
        int plot_h = graph_h - 38;

        // Grid lines (horizontal)
        for (int i = 0; i <= 4; ++i) {
            int ly = plot_y + plot_h - (plot_h * i / 4);
            DrawLine(plot_x, ly, plot_x + plot_w, ly, CLITERAL(Color){40,46,54,255});
            std::stringstream sl; sl << max_food * i / 4;
            draw_text(sl.str(), graph_x + 2, ly - 7, 9, UI::TEXT_DIM);
        }

        // Y axis label
        draw_text("Food/Life", graph_x + 2, plot_y, 9, UI::TEXT_DIM);

        // X axis label
        std::stringstream sx; sx << "Step";
        int sxw = (int)MeasureTextEx(game_font, sx.str().c_str(), 9, 1).x;
        draw_text(sx.str(), plot_x + plot_w - sxw - 2, graph_y + graph_h - 16, 9, UI::TEXT_DIM);

        int ep_range = std::max(1, max_ep - min_ep);

        // Best trace (overlay on per-agent lines)
        Color best_col = {88, 166, 255, 255};
        {
            std::vector<Vector2> verts;
            verts.reserve(best_trace.size() * 2);
            for (const auto& bt : best_trace) {
                float x = (float)(plot_x + (bt.episode - min_ep) * plot_w / ep_range);
                float y = (float)(plot_y + plot_h - bt.food * plot_h / max_food);
                verts.push_back({x, (float)(plot_y + plot_h)});
                verts.push_back({x, y});
            }
            Color fill_col = best_col;
            fill_col.a = 18;
            DrawTriangleStrip(verts.data(), (int)verts.size(), fill_col);
        }
        for (size_t j = 1; j < best_trace.size(); ++j) {
            int x1 = plot_x + (best_trace[j-1].episode - min_ep) * plot_w / ep_range;
            int y1 = plot_y + plot_h - best_trace[j-1].food * plot_h / max_food;
            int x2 = plot_x + (best_trace[j].episode - min_ep) * plot_w / ep_range;
            int y2 = plot_y + plot_h - best_trace[j].food * plot_h / max_food;
            y1 = std::max(plot_y, std::min(plot_y + plot_h, y1));
            y2 = std::max(plot_y, std::min(plot_y + plot_h, y2));
            DrawLine(x1, y1, x2, y2, best_col);
        }
        if (!best_trace.empty()) {
            const auto& last = best_trace.back();
            int lx = plot_x + (last.episode - min_ep) * plot_w / ep_range;
            int ly = plot_y + plot_h - last.food * plot_h / max_food;
            ly = std::max(plot_y, std::min(plot_y + plot_h, ly));
            DrawCircle(lx, ly, 4, best_col);
            std::string val = std::to_string(last.food);
            int vw = (int)MeasureTextEx(game_font, val.c_str(), 11, 1).x;
            draw_text(val, lx - vw / 2, ly - 16, 11, UI::TEXT);
        }

        // Lines for each agent
        char agent_id = 'A';
        for (size_t ai = 0; ai < env.get_agents().size() && ai < 16; ++ai, ++agent_id) {
            const auto& hist = agent_history[ai];
            if (hist.size() < 2) continue;

            Color col = env.get_agents()[ai].color;

            // Draw polyline
            for (size_t j = 1; j < hist.size(); ++j) {
                int x1 = plot_x + (hist[j-1].episode - min_ep) * plot_w / ep_range;
                int y1 = plot_y + plot_h - hist[j-1].food * plot_h / max_food;
                int x2 = plot_x + (hist[j].episode - min_ep) * plot_w / ep_range;
                int y2 = plot_y + plot_h - hist[j].food * plot_h / max_food;
                y1 = std::max(plot_y, std::min(plot_y + plot_h, y1));
                y2 = std::max(plot_y, std::min(plot_y + plot_h, y2));
                DrawLine(x1, y1, x2, y2, col);
            }
        }

        // Legend
        int lx = plot_x;
        int ly = graph_y + graph_h - 14;
        DrawRectangle(lx, ly, 8, 8, CLITERAL(Color){88, 166, 255, 255});
        draw_text(" Best", lx + 10, ly - 2, 10, UI::TEXT);
        lx += (int)MeasureTextEx(game_font, " Best", 10, 1).x + 16;
        agent_id = 'A';
        for (size_t ai = 0; ai < env.get_agents().size() && ai < 16; ++ai, ++agent_id) {
            if (agent_history[ai].empty()) continue;
            Color col = env.get_agents()[ai].color;
            std::string label = " "; label += agent_id;
            DrawRectangle(lx, ly, 8, 8, col);
            draw_text(label, lx + 10, ly - 2, 10, UI::TEXT);
            lx += 22;
            if (lx + 22 > graph_x + graph_w) break;
        }
    }

    sy += graph_h + 10;

    // ── Agent Statistics section ───────────────────────────────────────────
    render_sidebar_header(sy, "Agent Statistics");
    sy += 25;
    char agent_id = 'A';
    for (size_t i = 0; i < env.get_agents().size(); ++i, ++agent_id) {
        const AI& agent = env.get_agents()[i];

        int card_h = 70;
        draw_rounded_rect(grid_pixel_width + 8, sy, SIDEBAR_WIDTH - 16, card_h, 6, UI::SIDEBAR_HDR);

        Color name_col = agent.color;
        if (!agent.is_alive()) name_col = Fade(name_col, 0.4f);
        std::string name = "Agent "; name += agent_id;
        draw_text(name, grid_pixel_width + 18, sy + 6, 14, name_col);

        std::string status = agent.is_alive() ? "Active" : "Inactive";
        Color status_col = agent.is_alive() ? UI::COL_GREEN : UI::COL_RED;
        int sw = (int)MeasureTextEx(game_font, status.c_str(), 12, 1).x;
        draw_text(status, grid_pixel_width + SIDEBAR_WIDTH - 20 - sw, sy + 6, 12, status_col);

        std::stringstream fs; fs << "Food: " << agent.total_food_eaten;
        draw_text(fs.str(), grid_pixel_width + 18, sy + 28, 13, UI::TEXT);

        draw_text("Energy", grid_pixel_width + 18, sy + 48, 12, UI::TEXT_DIM);
        render_agent_energy_bar(grid_pixel_width + 80, sy + 49,
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
        int cx = grid_pixel_width + 10 + (c % 3) * col_w;
        int cy = sy + (c / 3) * 18;
        draw_text(controls[c], cx, cy, 12, UI::TEXT_DIM);
    }

    // ── Debug overlay ─────────────────────────────────────────────────────
    if (debug_mode && selected_agent >= 0 && (size_t)selected_agent < env.get_agents().size()) {
        const AI& agent = env.get_agents()[(size_t)selected_agent];
        if (agent.is_alive())
            render_debug_overlay(agent, env.get_food_list(),
                                 grid_pixel_width + 15, sy + 60);
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
