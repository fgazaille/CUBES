#define RAYGUI_IMPLEMENTATION
#include "menu.hpp"
#include "environment.hpp"
#include <sstream>
#include <cmath>
#include <cstring>

// ── Ctor ─────────────────────────────────────────────────────────────────────

Menu::Menu(TrainingStatus* training_status)
    : training_status_ptr_(training_status) {
    snprintf(ep_buf_, sizeof(ep_buf_), "%d", training_config_.episodes);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(CLITERAL(Color){13,17,23,255}));
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   ColorToInt(CLITERAL(Color){30,36,44,255}));
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   ColorToInt(CLITERAL(Color){201,209,217,255}));
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,  ColorToInt(CLITERAL(Color){48,54,61,255}));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,  ColorToInt(CLITERAL(Color){56,58,89,255}));
    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, ColorToInt(CLITERAL(Color){48,54,61,255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,  ColorToInt(CLITERAL(Color){201,209,217,255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt(CLITERAL(Color){88,166,255,255}));
}

// ── Core loop ────────────────────────────────────────────────────────────────

MenuState Menu::run() {
    while (true) {
        if (WindowShouldClose()) return MenuState::EXIT;

        switch (current_state_) {
            case MenuState::HOME:            do_home(); break;
            case MenuState::TRAINING_CONFIG:  do_training_config(); break;
            case MenuState::TRAINING_ACTIVE:  do_training_active(); break;
            case MenuState::SETTINGS:        do_settings(); break;
            case MenuState::ABOUT:           do_about(); break;
            default: return current_state_;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            switch (current_state_) {
                case MenuState::HOME:     current_state_ = MenuState::EXIT; break;
                case MenuState::TRAINING_CONFIG:
                case MenuState::SETTINGS:
                case MenuState::ABOUT:    current_state_ = MenuState::HOME; break;
                case MenuState::TRAINING_ACTIVE:
                    g_training_stop_flag.store(true);
                    current_state_ = MenuState::HOME;
                    break;
                default: break;
            }
        }

        if (current_state_ == MenuState::START_SIM ||
            current_state_ == MenuState::EXIT) {
            MenuState ret = current_state_;
            if (ret != MenuState::EXIT)
                current_state_ = MenuState::HOME;
            return ret;
        }
    }
}

// ── Home ─────────────────────────────────────────────────────────────────────

void Menu::do_home() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int cw = GetScreenWidth();
    float t = GetTime();

    for (int y = 0; y < GetScreenHeight(); y += 6) {
        float pulse = 0.85f + 0.15f * std::sin(t * 0.5f + y * 0.02f);
        Color row = {
            (unsigned char)(13 * pulse),
            (unsigned char)(17 * pulse),
            (unsigned char)(23 * pulse),
            255
        };
        DrawRectangle(0, y, cw, 6, row);
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 48);
    int tw = MeasureText("CUBES", 48);
    DrawText("CUBES", (cw - tw) / 2, 50, 48, CLITERAL(Color){88,166,255,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    int sw = MeasureText("AI Learning Simulation", 18);
    DrawText("AI Learning Simulation", (cw - sw) / 2, 110, 18, CLITERAL(Color){139,148,158,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    struct Btn { const char* label; MenuState target; };
    Btn btns[] = {
        {"Start Simulation", MenuState::START_SIM},
        {"Training",         MenuState::TRAINING_CONFIG},
        {"Settings",         MenuState::SETTINGS},
        {"About",            MenuState::ABOUT},
        {"Exit",             MenuState::EXIT}
    };

    int bx = (cw - 200) / 2;
    int by = 190;
    for (auto& b : btns) {
        if (GuiButton({(float)bx, (float)by, 200, 40}, b.label))
            current_state_ = b.target;
        by += 50;
    }

    bool brain_exists = std::ifstream(brain_file_path()).good();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    const char* msg = brain_exists
        ? "Brain file found - will auto-load"
        : "No brain file - agents will use random brains";
    Color msg_col = brain_exists
        ? CLITERAL(Color){63,185,80,255}
        : CLITERAL(Color){248,81,73,255};
    int mx = MeasureText(msg, 13);
    DrawText(msg, (cw - mx) / 2, GetScreenHeight() - 40, 13, msg_col);

    EndDrawing();
}

// ── Training Config ──────────────────────────────────────────────────────────

void Menu::do_training_config() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int cw = GetScreenWidth();

    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("Training", 36);
    DrawText("Training", (cw - tw) / 2, 25, 36, CLITERAL(Color){88,166,255,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    int sw = MeasureText("Optimise a single simulation at maximum speed.", 14);
    DrawText("Optimise a single simulation at maximum speed.",
             (cw - sw) / 2, 75, 14, CLITERAL(Color){139,148,158,255});

    int px = (cw - 500) / 2;
    DrawRectangleRounded({(float)px, 110, 500, 200}, 0.1f, 8, CLITERAL(Color){22,27,34,255});
    DrawRectangleRoundedLines({(float)px, 110, 500, 200}, 0.1f, 8, CLITERAL(Color){48,54,61,255});

    int lx = px + 30;
    int fx = px + 170;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);

    DrawText("Episodes:", lx, 140, 14, CLITERAL(Color){201,209,217,255});
    Rectangle ep_r = {(float)fx, 135, 160, 30};
    if (GuiTextBox(ep_r, ep_buf_, 31, editing_episodes_))
        editing_episodes_ = !editing_episodes_;
    if (!editing_episodes_) {
        try { training_config_.episodes = std::max(100, std::min(500000, std::stoi(ep_buf_))); }
        catch (...) {}
    }

    DrawText("Auto-save brain:", lx, 190, 14, CLITERAL(Color){201,209,217,255});
    Rectangle as_r = {(float)fx, 185, 80, 28};
    if (GuiButton(as_r, training_config_.auto_save ? "ON" : "OFF"))
        training_config_.auto_save = !training_config_.auto_save;

    // Episode presets
    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    struct { const char* label; int val; } qbtns[] = {
        {"1K", 1000}, {"10K", 10000}, {"50K", 50000}, {"100K", 100000}
    };
    Rectangle qb = {210, 240, 65, 28};
    for (auto& q : qbtns) {
        if (GuiButton(qb, q.label)) {
            training_config_.episodes = q.val;
            snprintf(ep_buf_, sizeof(ep_buf_), "%d", q.val);
        }
        qb.x += qb.width + 12;
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    int btn_w = 140, btn_h = 40, gap = 20;
    int start_x = (cw - 2 * btn_w - gap) / 2;
    if (GuiButton({(float)start_x, 300, (float)btn_w, (float)btn_h}, "Start Training")) {
        if (training_start_callback_)
            training_start_callback_(training_config_, training_status_ptr_);
        snprintf(ep_buf_, sizeof(ep_buf_), "%d", training_config_.episodes);
        current_state_ = MenuState::TRAINING_ACTIVE;
    }
    if (GuiButton({(float)(start_x + btn_w + gap), 300, (float)btn_w, (float)btn_h}, "Back"))
        current_state_ = MenuState::HOME;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    const char* tip = "ESC to go back";
    int tix = (cw - MeasureText(tip, 13)) / 2;
    DrawText(tip, tix, 380, 13, CLITERAL(Color){100,108,118,255});

    EndDrawing();
}

// ── Training Active ──────────────────────────────────────────────────────────

void Menu::do_training_active() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int cw = GetScreenWidth();
    int done = 0, total = 0, best = 0;
    if (training_status_ptr_ && training_status_ptr_->active.load()) {
        done  = training_status_ptr_->episodes_done.load();
        total = training_status_ptr_->total_episodes.load();
        best  = training_status_ptr_->best_food.load();
    }
    float prog = (total > 0) ? (float)done / total : 0.0f;
    prog = std::max(0.0f, std::min(1.0f, prog));

    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("Training", 36);
    DrawText("Training", (cw - tw) / 2, 15, 36, CLITERAL(Color){88,166,255,255});

    std::string best_str = "Best food: " + std::to_string(best);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    int bw = MeasureText(best_str.c_str(), 18);
    DrawText(best_str.c_str(), (cw - bw) / 2, 60, 18, CLITERAL(Color){63,185,80,255});

    // Progress bar
    Rectangle bar = {(float)(cw / 2 - 250), 100, 500, 30};
    DrawRectangleRounded(bar, 0.15f, 8, CLITERAL(Color){48,54,61,255});
    if (prog > 0) {
        Rectangle fill = {bar.x, bar.y, bar.width * prog, bar.height};
        DrawRectangleRounded(fill, 0.15f, 8, CLITERAL(Color){63,185,80,255});
    }
    DrawRectangleRoundedLines(bar, 0.15f, 8, CLITERAL(Color){100,108,118,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    std::string pct_str = std::to_string(done) + " / " + std::to_string(total) + " episodes  (" + std::to_string((int)(prog * 100)) + "%)";
    int pctw = MeasureText(pct_str.c_str(), 14);
    DrawText(pct_str.c_str(), (cw - pctw) / 2, 140, 14, CLITERAL(Color){201,209,217,255});

    // ── Food history graph ────────────────────────────────────────────────
    int graph_x = 60;
    int graph_y = 190;
    int graph_w = cw - 120;
    int graph_h = 140;

    DrawRectangleRounded({(float)graph_x, (float)graph_y, (float)graph_w, (float)graph_h}, 0.1f, 8, CLITERAL(Color){22,27,34,255});

    std::vector<TrainingFoodPoint> history;
    if (training_status_ptr_) {
        std::lock_guard<std::mutex> hlock(training_status_ptr_->history_mutex);
        history = training_status_ptr_->food_history;
    }

    int max_food = 10;
    int min_ep = 0;
    int max_ep = 1;
    for (const auto& p : history) {
        if (p.best_food > max_food) max_food = p.best_food;
        if (p.episodes_done < min_ep) min_ep = p.episodes_done;
        if (p.episodes_done > max_ep) max_ep = p.episodes_done;
    }
    max_food = std::max(10, ((max_food / 10) + 1) * 10);
    int ep_range = std::max(1, max_ep - min_ep);

    if (history.size() >= 2) {
        int plot_x = graph_x + 35;
        int plot_y = graph_y + 12;
        int plot_w = graph_w - 50;
        int plot_h = graph_h - 40;

        for (int i = 0; i <= 4; ++i) {
            int ly = plot_y + plot_h - (plot_h * i / 4);
            DrawLine(plot_x, ly, plot_x + plot_w, ly, CLITERAL(Color){40,46,54,255});
            std::string lbl = std::to_string(max_food * i / 4);
            DrawText(lbl.c_str(), graph_x + 2, ly - 7, 9, CLITERAL(Color){100,108,118,255});
        }

        DrawText("Food per Life", graph_x + 2, plot_y, 9, CLITERAL(Color){100,108,118,255});

        std::string xlbl = "Episodes";
        int xlw = MeasureText(xlbl.c_str(), 9);
        DrawText(xlbl.c_str(), plot_x + plot_w - xlw - 2, graph_y + graph_h - 18, 9, CLITERAL(Color){100,108,118,255});

        // Single line for the one simulation
        Color line_col = {88, 166, 255, 255};
        for (size_t j = 1; j < history.size(); ++j) {
            int x1 = plot_x + (history[j-1].episodes_done - min_ep) * plot_w / ep_range;
            int y1 = plot_y + plot_h - history[j-1].best_food * plot_h / max_food;
            int x2 = plot_x + (history[j].episodes_done - min_ep) * plot_w / ep_range;
            int y2 = plot_y + plot_h - history[j].best_food * plot_h / max_food;
            DrawLine(x1, y1, x2, y2, line_col);
        }

        // Last dot + value label
        const auto& last = history.back();
        int lx = plot_x + (last.episodes_done - min_ep) * plot_w / ep_range;
        int ly = plot_y + plot_h - last.best_food * plot_h / max_food;
        DrawCircle(lx, ly, 4, line_col);
        std::string val = std::to_string(last.best_food);
        int vw = MeasureText(val.c_str(), 11);
        DrawText(val.c_str(), lx - vw / 2, ly - 16, 11, CLITERAL(Color){201,209,217,255});
    } else {
        DrawText("Collecting data...", graph_x + 40, graph_y + graph_h / 2 - 8, 14, CLITERAL(Color){100,108,118,255});
    }

    // STOP button
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    if (GuiButton({(float)(cw / 2 - 60), (float)(graph_y + graph_h + 20), 120, 45}, "STOP")) {
        g_training_stop_flag.store(true);
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    int tipw = MeasureText("ESC to cancel training", 13);
    DrawText("ESC to cancel training", (cw - tipw) / 2, graph_y + graph_h + 80, 13, CLITERAL(Color){100,108,118,255});

    if (training_status_ptr_ && !training_status_ptr_->active.load())
        current_state_ = MenuState::HOME;

    EndDrawing();
}

// ── Settings ──────────────────────────────────────────────────────────────────

void Menu::do_settings() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int cw = GetScreenWidth();
    RuntimeConfig& cfg = RuntimeConfig::instance();

    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("Settings", 36);
    DrawText("Settings", (cw - tw) / 2, 25, 36, CLITERAL(Color){88,166,255,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    int sw = MeasureText("Adjust simulation parameters.", 14);
    DrawText("Adjust simulation parameters.",
             (cw - sw) / 2, 75, 14, CLITERAL(Color){139,148,158,255});

    int px = (cw - 500) / 2;
    DrawRectangleRounded({(float)px, 110, 500, 350}, 0.1f, 8, CLITERAL(Color){22,27,34,255});
    DrawRectangleRoundedLines({(float)px, 110, 500, 350}, 0.1f, 8, CLITERAL(Color){48,54,61,255});

    int lx = px + 30;
    int fx = px + 220;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);

    auto edit_field = [&](const char* label, int y, char* buf, unsigned buf_size, bool& editing, int& val, int min_v, int max_v) {
        DrawText(label, lx, y + 6, 14, CLITERAL(Color){201,209,217,255});
        Rectangle r = {(float)fx, (float)y, 120, 30};
        if (GuiTextBox(r, buf, buf_size - 1, editing))
            editing = !editing;
        if (!editing) {
            try { val = std::max(min_v, std::min(max_v, std::stoi(buf))); }
            catch (...) {}
        }
    };

    snprintf(grid_buf_, sizeof(grid_buf_), "%d", cfg.grid_size);
    edit_field("Grid Size:",        115, grid_buf_,     sizeof(grid_buf_),     editing_grid_,       cfg.grid_size,        5,  30);
    edit_field("Agents:",           170, agents_buf_,   sizeof(agents_buf_),   editing_agents_,     cfg.agent_count,      1,  50);
    edit_field("Food Count:",       225, food_count_buf_, sizeof(food_count_buf_), editing_food_count_, cfg.food_count,   1, 200);
    edit_field("Food Threshold:",   280, food_thresh_buf_, sizeof(food_thresh_buf_), editing_food_thresh_, cfg.food_reset_threshold, 1, 100);

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    if (GuiButton({(float)(cw / 2 - 60), 500, 120, 40}, "Back"))
        current_state_ = MenuState::HOME;

    EndDrawing();
}

// ── About ────────────────────────────────────────────────────────────────────

void Menu::do_about() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int cw = GetScreenWidth();

    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("About CUBES", 36);
    DrawText("About CUBES", (cw - tw) / 2, 40, 36, CLITERAL(Color){88,166,255,255});

    auto blurb = [&](const char* text, int y, int size) {
        GuiSetStyle(DEFAULT, TEXT_SIZE, size);
        int w = MeasureText(text, size);
        DrawText(text, (cw - w) / 2, y, size, CLITERAL(Color){139,148,158,255});
    };

    blurb("CUBES - AI Learning Simulation",                110, 18);
    blurb("Version 2.0 - Raylib Edition",                  140, 14);
    blurb("Uses Deep Q-Network (DQN) reinforcement learning", 180, 16);
    blurb("Agents learn to collect food and survive",      210, 16);
    blurb("Genetic algorithms evolve better agents each generation", 240, 16);

    blurb("Controls:", 290, 16);
    blurb("ESC - Exit  |  R - Reset  |  Space - Pause",      325, 14);
    blurb("W - Warp  |  +/- - Speed  |  0 - Reset Warp",     350, 14);
    blurb("D - Debug  |  S - Save Brain  |  L - Load Brain", 375, 14);

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    if (GuiButton({(float)(cw / 2 - 60), 500, 120, 40}, "Back"))
        current_state_ = MenuState::HOME;

    EndDrawing();
}
