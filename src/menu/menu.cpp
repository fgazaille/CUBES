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
    snprintf(par_buf_, sizeof(par_buf_), "%d", training_config_.parallel_envs);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(CLITERAL(Color){13,17,23,255}));
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   ColorToInt(CLITERAL(Color){30,36,44,255}));
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   ColorToInt(CLITERAL(Color){201,209,217,255}));
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED,  ColorToInt(CLITERAL(Color){255,255,255,255}));
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,  ColorToInt(CLITERAL(Color){48,54,61,255}));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,  ColorToInt(CLITERAL(Color){56,58,89,255}));
    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, ColorToInt(CLITERAL(Color){48,54,61,255}));
    GuiSetStyle(BUTTON, BORDER_COLOR_FOCUSED, ColorToInt(CLITERAL(Color){88,166,255,255}));
    GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
    GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(CLITERAL(Color){201,209,217,255}));
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
                case MenuState::ABOUT:    current_state_ = MenuState::HOME; break;
                case MenuState::SETTINGS: settings_inited_ = false; current_state_ = MenuState::HOME; break;
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

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float t = GetTime();

    for (int y = 0; y < sh; y += 6) {
        float pulse = 0.85f + 0.15f * std::sin(t * 0.5f + y * 0.02f);
        Color row = {
            (unsigned char)(13 * pulse),
            (unsigned char)(17 * pulse),
            (unsigned char)(23 * pulse),
            255
        };
        DrawRectangle(0, y, sw, 6, row);
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 48);
    int tw = MeasureText("CUBES", 48);
    for (int i = 3; i >= 0; --i) {
        Color glow = {88, 166, 255, (unsigned char)(20 - i * 5)};
        DrawText("CUBES", (sw - tw) / 2 - i, 50 - i, 48, glow);
        DrawText("CUBES", (sw - tw) / 2 + i, 50 + i, 48, glow);
    }
    DrawText("CUBES", (sw - tw) / 2, 50, 48, CLITERAL(Color){88,166,255,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    tw = MeasureText("AI Learning Simulation", 18);
    DrawText("AI Learning Simulation", (sw - tw) / 2, 110, 18, CLITERAL(Color){139,148,158,255});
    int ul_w = sw + 40;
    DrawRectangle((sw - ul_w) / 2, 135, ul_w, 1, CLITERAL(Color){48,54,61,255});

    int card_x = (sw - 240) / 2;
    int card_y = 170;
    int card_w = 240;
    int card_h = 270;
    DrawRectangleRounded({(float)card_x, (float)card_y, (float)card_w, (float)card_h}, 0.12f, 8, CLITERAL(Color){22,27,34,255});
    DrawRectangleRoundedLines({(float)card_x, (float)card_y, (float)card_w, (float)card_h}, 0.12f, 8, CLITERAL(Color){48,54,61,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    struct Btn { const char* label; MenuState target; };
    Btn btns[] = {
        {"Start Simulation", MenuState::START_SIM},
        {"Training",         MenuState::TRAINING_CONFIG},
        {"Settings",         MenuState::SETTINGS},
        {"About",            MenuState::ABOUT},
        {"Exit",             MenuState::EXIT}
    };

    int bx = (sw - 200) / 2;
    int by = card_y + 20;
    for (auto& b : btns) {
        if (GuiButton({(float)bx, (float)by, 200, 40}, b.label))
            current_state_ = b.target;
        by += 48;
    }

    bool brain_exists = std::ifstream(brain_file_path()).good();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    const char* msg = brain_exists
        ? "Brain file found - will auto-load"
        : "No brain file - agents will use random brains";
    Color msg_col = brain_exists
        ? CLITERAL(Color){63,185,80,255}
        : CLITERAL(Color){248,81,73,255};
    tw = MeasureText(msg, 13);
    DrawText(msg, (sw - tw) / 2, sh - 40, 13, msg_col);

    EndDrawing();
}

// ── Training Config ──────────────────────────────────────────────────────────

void Menu::do_training_config() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int sw = GetScreenWidth();

    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("Training", 36);
    DrawText("Training", (sw - tw) / 2, 25, 36, CLITERAL(Color){88,166,255,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    tw = MeasureText("Run N parallel simulations for faster exploration.", 14);
    DrawText("Run N parallel simulations for faster exploration.",
             (sw - tw) / 2, 75, 14, CLITERAL(Color){139,148,158,255});

    int px = (sw - 500) / 2;
    DrawRectangleRounded({(float)(px + 3), (float)(110 + 3), 500, 280}, 0.1f, 8, CLITERAL(Color){8,11,16,255});
    DrawRectangleRounded({(float)px, 110, 500, 280}, 0.1f, 8, CLITERAL(Color){22,27,34,255});
    DrawRectangleRoundedLines({(float)px, 110, 500, 280}, 0.1f, 8, CLITERAL(Color){48,54,61,255});

    int lx = px + 30;
    int fx = px + 180;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);

    DrawText("Episodes:", lx, 135, 14, CLITERAL(Color){201,209,217,255});
    Rectangle ep_r = {(float)fx, 130, 160, 30};
    if (GuiTextBox(ep_r, ep_buf_, 31, editing_episodes_))
        editing_episodes_ = !editing_episodes_;
    if (!editing_episodes_) {
        try { training_config_.episodes = std::max(100, std::min(500000, std::stoi(ep_buf_))); }
        catch (...) {}
    }

    DrawText("Parallel Envs:", lx, 178, 14, CLITERAL(Color){201,209,217,255});
    Rectangle par_r = {(float)fx, 173, 160, 30};
    if (GuiTextBox(par_r, par_buf_, 31, editing_parallel_))
        editing_parallel_ = !editing_parallel_;
    if (!editing_parallel_) {
        try { training_config_.parallel_envs = std::max(1, std::min(64, std::stoi(par_buf_))); }
        catch (...) {}
    }

    DrawText("Auto-save brain:", lx, 221, 14, CLITERAL(Color){201,209,217,255});
    Rectangle as_r = {(float)fx, 216, 80, 28};
    if (GuiButton(as_r, training_config_.auto_save ? "ON" : "OFF"))
        training_config_.auto_save = !training_config_.auto_save;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    struct { const char* label; int val; } qbtns[] = {
        {"1K", 1000}, {"10K", 10000}, {"50K", 50000}, {"100K", 100000}
    };
    Rectangle qb = {(float)px + 170, 270, 65, 28};
    for (auto& q : qbtns) {
        if (GuiButton(qb, q.label)) {
            training_config_.episodes = q.val;
            snprintf(ep_buf_, sizeof(ep_buf_), "%d", q.val);
        }
        qb.x += qb.width + 12;
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    int btn_w = 140, btn_h = 40, gap = 20;
    int start_x = (sw - 2 * btn_w - gap) / 2;
    if (GuiButton({(float)start_x, 330, (float)btn_w, (float)btn_h}, "Start Training")) {
        if (training_status_ptr_) {
            std::lock_guard<std::mutex> hlock(training_status_ptr_->history_mutex);
            training_status_ptr_->food_history.clear();
        }
        if (training_start_callback_)
            training_start_callback_(training_config_, training_status_ptr_);
        snprintf(ep_buf_, sizeof(ep_buf_), "%d", training_config_.episodes);
        snprintf(par_buf_, sizeof(par_buf_), "%d", training_config_.parallel_envs);
        current_state_ = MenuState::TRAINING_ACTIVE;
    }
    if (GuiButton({(float)(start_x + btn_w + gap), 330, (float)btn_w, (float)btn_h}, "Back"))
        current_state_ = MenuState::HOME;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    const char* tip = "ESC to go back";
    int tix = (sw - MeasureText(tip, 13)) / 2;
    DrawText(tip, tix, 410, 13, CLITERAL(Color){100,108,118,255});

    EndDrawing();
}

// ── Training Active ──────────────────────────────────────────────────────────

void Menu::do_training_active() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int sw = GetScreenWidth();
    int done = 0, total = 0, best = 0, par = 1;
    if (training_status_ptr_ && training_status_ptr_->active.load()) {
        done  = training_status_ptr_->episodes_done.load();
        total = training_status_ptr_->total_episodes.load();
        best  = training_status_ptr_->best_food.load();
        par   = training_status_ptr_->parallel_count;
    }
    float prog = (total > 0) ? (float)done / total : 0.0f;
    prog = std::max(0.0f, std::min(1.0f, prog));

    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("Training", 36);
    DrawText("Training", (sw - tw) / 2, 15, 36, CLITERAL(Color){88,166,255,255});

    std::string best_str = "Best food: " + std::to_string(best) + "  (" + std::to_string(par) + " parallel)";
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    tw = MeasureText(best_str.c_str(), 18);
    DrawText(best_str.c_str(), (sw - tw) / 2, 60, 18, CLITERAL(Color){63,185,80,255});

    // Progress bar
    Rectangle bar = {(float)(sw / 2 - 250), 100, 500, 28};
    DrawRectangleRounded(bar, 0.14f, 8, CLITERAL(Color){30,36,44,255});
    if (prog > 0) {
        Rectangle fill = {bar.x + 1, bar.y + 1, (bar.width - 2) * prog, bar.height - 2};
        DrawRectangleRounded(fill, 0.14f, 8, CLITERAL(Color){63,185,80,255});
    }
    DrawRectangleRoundedLines(bar, 0.14f, 8, CLITERAL(Color){48,54,61,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    std::string pct_str = std::to_string(done) + " / " + std::to_string(total) + " episodes  (" + std::to_string((int)(prog * 100)) + "%)";
    int pctw = MeasureText(pct_str.c_str(), 14);
    DrawText(pct_str.c_str(), (sw - pctw) / 2, 140, 14, CLITERAL(Color){201,209,217,255});

    // ── Food history graph ────────────────────────────────────────────────
    int graph_x = 60;
    int graph_y = 190;
    int graph_w = sw - 120;
    int graph_h = 160;

    DrawRectangleRounded({(float)(graph_x + 3), (float)(graph_y + 3), (float)graph_w, (float)graph_h}, 0.1f, 8, CLITERAL(Color){8,11,16,255});
    DrawRectangleRounded({(float)graph_x, (float)graph_y, (float)graph_w, (float)graph_h}, 0.1f, 8, CLITERAL(Color){22,27,34,255});

    std::vector<TrainingFoodPoint> history;
    if (training_status_ptr_) {
        std::lock_guard<std::mutex> hlock(training_status_ptr_->history_mutex);
        history = training_status_ptr_->food_history;
    }

    int max_best = 10;
    int max_rate = 10;
    int min_ep = 0;
    int max_ep = 1;
    for (const auto& p : history) {
        if (p.best_food > max_best) max_best = p.best_food;
        if (p.episodes_done < min_ep) min_ep = p.episodes_done;
        if (p.episodes_done > max_ep) max_ep = p.episodes_done;
    }
    // Compute max rate (food per 100 steps) from total_food diffs
    for (size_t j = 1; j < history.size(); ++j) {
        int rate = history[j].total_food - history[j-1].total_food;
        if (rate > max_rate) max_rate = rate;
    }
    max_best = std::max(10, ((max_best / 10) + 1) * 10);
    max_rate = std::max(10, ((max_rate / 10) + 1) * 10);
    int ep_range = std::max(1, max_ep - min_ep);

    if (history.size() >= 2) {
        int plot_x = graph_x + 45;
        int plot_y = graph_y + 12;
        int plot_w = graph_w - 90;
        int plot_h = graph_h - 48;

        Color best_col = {88, 166, 255, 255};
        Color rate_col = {63, 185, 80, 255};

        // ── Left Y-axis: Best Ever ────────────────────────────────────────
        for (int i = 0; i <= 4; ++i) {
            int ly = plot_y + plot_h - (plot_h * i / 4);
            DrawLine(plot_x, ly, plot_x + plot_w, ly, CLITERAL(Color){60,70,85,255});
            int val = max_best * i / 4;
            std::string lbl;
            if (max_best * i % 4 == 0)
                lbl = std::to_string(val);
            else
                lbl = std::to_string(max_best * i / 4.0).substr(0, 4);
            DrawText(lbl.c_str(), graph_x + 2, ly - 7, 9, CLITERAL(Color){100,108,118,255});
        }
        DrawText("Best Ever", graph_x + 2, plot_y, 9, best_col);

        // ── Right Y-axis: Food/100 steps ──────────────────────────────────
        int rx = plot_x + plot_w + 4;
        for (int i = 0; i <= 4; ++i) {
            int ly = plot_y + plot_h - (plot_h * i / 4);
            int val = max_rate * i / 4;
            std::string lbl;
            if (max_rate * i % 4 == 0)
                lbl = std::to_string(val);
            else
                lbl = std::to_string(max_rate * i / 4.0).substr(0, 4);
            DrawText(lbl.c_str(), rx, ly - 7, 9, CLITERAL(Color){100,108,118,255});
        }
        DrawText("Rate", rx, plot_y, 9, rate_col);

        // ── X-axis ────────────────────────────────────────────────────────
        std::string xlbl = "Steps";
        int xlw = MeasureText(xlbl.c_str(), 9);
        DrawText(xlbl.c_str(), plot_x + plot_w - xlw - 2, graph_y + graph_h - 18, 9, CLITERAL(Color){100,108,118,255});

        // ── Trace 1: Best Ever ────────────────────────────────────────────
        {
            std::vector<Vector2> verts;
            verts.reserve(history.size() * 2);
            for (const auto& p : history) {
                float x = (float)(plot_x + (p.episodes_done - min_ep) * plot_w / ep_range);
                float y = (float)(plot_y + plot_h - p.best_food * plot_h / max_best);
                verts.push_back({x, (float)(plot_y + plot_h)});
                verts.push_back({x, y});
            }
            Color fill_col = best_col;
            fill_col.a = 18;
            DrawTriangleStrip(verts.data(), (int)verts.size(), fill_col);
        }
        for (size_t j = 1; j < history.size(); ++j) {
            int x1 = plot_x + (history[j-1].episodes_done - min_ep) * plot_w / ep_range;
            int y1 = plot_y + plot_h - history[j-1].best_food * plot_h / max_best;
            int x2 = plot_x + (history[j].episodes_done - min_ep) * plot_w / ep_range;
            int y2 = plot_y + plot_h - history[j].best_food * plot_h / max_best;
            y1 = std::max(plot_y, std::min(plot_y + plot_h, y1));
            y2 = std::max(plot_y, std::min(plot_y + plot_h, y2));
            DrawLine(x1, y1, x2, y2, best_col);
        }
        {
            const auto& last = history.back();
            int lx = plot_x + (last.episodes_done - min_ep) * plot_w / ep_range;
            int ly = plot_y + plot_h - last.best_food * plot_h / max_best;
            ly = std::max(plot_y, std::min(plot_y + plot_h, ly));
            DrawCircle(lx, ly, 4, best_col);
            std::string val = std::to_string(last.best_food);
            int vw = MeasureText(val.c_str(), 11);
            DrawText(val.c_str(), lx - vw / 2, ly - 16, 11, CLITERAL(Color){201,209,217,255});
        }

        // ── Trace 2: Food/100 steps (right Y-axis) ────────────────────────
        for (size_t j = 2; j < history.size(); ++j) {
            int rate = history[j].total_food - history[j-1].total_food;
            int prev_rate = history[j-1].total_food - history[j-2].total_food;
            int x1 = plot_x + (history[j-1].episodes_done - min_ep) * plot_w / ep_range;
            int y1 = plot_y + plot_h - prev_rate * plot_h / max_rate;
            int x2 = plot_x + (history[j].episodes_done - min_ep) * plot_w / ep_range;
            int y2 = plot_y + plot_h - rate * plot_h / max_rate;
            y1 = std::max(plot_y, std::min(plot_y + plot_h, y1));
            y2 = std::max(plot_y, std::min(plot_y + plot_h, y2));
            Color rc = rate_col;
            rc.a = 200;
            DrawLine(x1, y1, x2, y2, rc);
        }
        // Last dot for rate
        if (history.size() >= 3) {
            int last_rate = history.back().total_food - history[history.size()-2].total_food;
            const auto& lp = history.back();
            int lx = plot_x + (lp.episodes_done - min_ep) * plot_w / ep_range;
            int ly = plot_y + plot_h - last_rate * plot_h / max_rate;
            ly = std::max(plot_y, std::min(plot_y + plot_h, ly));
            DrawCircle(lx, ly, 3, rate_col);
        }

        // ── Legend ────────────────────────────────────────────────────────
        {
            int lx = plot_x;
            int ly = graph_y + graph_h - 16;
            DrawRectangle(lx, ly, 8, 8, best_col);
            DrawText(" Best", lx + 10, ly - 2, 10, CLITERAL(Color){201,209,217,255});
            lx += 55;
            DrawRectangle(lx, ly, 8, 8, rate_col);
            DrawText(" Rate", lx + 10, ly - 2, 10, CLITERAL(Color){201,209,217,255});
        }
    } else {
        DrawText("Collecting data...", graph_x + 40, graph_y + graph_h / 2 - 8, 14, CLITERAL(Color){100,108,118,255});
    }

    // STOP button
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    if (GuiButton({(float)(sw / 2 - 60), (float)(graph_y + graph_h + 20), 120, 45}, "STOP")) {
        g_training_stop_flag.store(true);
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    int tipw = MeasureText("ESC to cancel training", 13);
    DrawText("ESC to cancel training", (sw - tipw) / 2, graph_y + graph_h + 80, 13, CLITERAL(Color){100,108,118,255});

    if (training_status_ptr_ && !training_status_ptr_->active.load()) {
        {
            std::lock_guard<std::mutex> hlock(training_status_ptr_->history_mutex);
            training_status_ptr_->food_history.clear();
        }
        current_state_ = MenuState::HOME;
    }

    EndDrawing();
}

// ── Settings ──────────────────────────────────────────────────────────────────

void Menu::do_settings() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int sw = GetScreenWidth();
    RuntimeConfig& cfg = RuntimeConfig::instance();

    // Initialize buffers once when entering settings
    if (!settings_inited_) {
        settings_inited_ = true;
        snprintf(grid_buf_, sizeof(grid_buf_), "%d", cfg.grid_size);
        snprintf(agents_buf_, sizeof(agents_buf_), "%d", cfg.agent_count);
        snprintf(food_count_buf_, sizeof(food_count_buf_), "%d", cfg.food_count);
        snprintf(food_energy_buf_, sizeof(food_energy_buf_), "%d", cfg.food_energy);
        snprintf(food_thresh_buf_, sizeof(food_thresh_buf_), "%d", cfg.food_reset_threshold);
        snprintf(max_energy_buf_, sizeof(max_energy_buf_), "%d", cfg.max_energy);
        snprintf(energy_decay_buf_, sizeof(energy_decay_buf_), "%d", cfg.energy_decay);
        snprintf(lr_buf_, sizeof(lr_buf_), "%.4f", cfg.learning_rate);
        snprintf(discount_buf_, sizeof(discount_buf_), "%.3f", cfg.discount_factor);
        snprintf(explore_init_buf_, sizeof(explore_init_buf_), "%.3f", cfg.initial_explore_rate);
        snprintf(explore_decay_buf_, sizeof(explore_decay_buf_), "%.4f", cfg.explore_decay);
        snprintf(explore_min_buf_, sizeof(explore_min_buf_), "%.3f", cfg.min_explore_rate);
        snprintf(buf_size_buf_, sizeof(buf_size_buf_), "%d", cfg.experience_buffer_size);
        snprintf(batch_size_buf_, sizeof(batch_size_buf_), "%d", cfg.batch_size);
        snprintf(target_freq_buf_, sizeof(target_freq_buf_), "%d", cfg.target_update_frequency);
        snprintf(max_threads_buf_, sizeof(max_threads_buf_), "%d", cfg.max_threads);
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("Settings", 36);
    DrawText("Settings", (sw - tw) / 2, 25, 36, CLITERAL(Color){88,166,255,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    tw = MeasureText("Adjust simulation parameters.", 14);
    DrawText("Adjust simulation parameters.",
             (sw - tw) / 2, 75, 14, CLITERAL(Color){139,148,158,255});

    int px = (sw - 500) / 2;
    DrawRectangleRounded({(float)(px + 3), (float)(110 + 3), 500, 350}, 0.1f, 8, CLITERAL(Color){8,11,16,255});
    DrawRectangleRounded({(float)px, 110, 500, 350}, 0.1f, 8, CLITERAL(Color){22,27,34,255});
    DrawRectangleRoundedLines({(float)px, 110, 500, 350}, 0.1f, 8, CLITERAL(Color){48,54,61,255});

    int col1_x = px + 20;
    int col2_x = px + card_w / 2 + 10;
    int field_w = 170;
    int label_w = 130;
    int row_h = 32;
    int start_y = 100;

    struct Field {
        const char* label;
        char* buf;
        unsigned buf_size;
        bool* editing;
        void* val_ptr;
        int type; // 0=int, 1=double
        double min_d;
        double max_d;
        int min_i;
        int max_i;
    };

    auto draw_field = [&](const Field& f, int x, int y) {
        Color c = *f.editing ? CLITERAL(Color){88,166,255,255} : CLITERAL(Color){201,209,217,255};
        DrawText(f.label, x, y + 7, 13, c);
        Rectangle r = {(float)(x + label_w), (float)y, (float)field_w, 28};
        if (GuiTextBox(r, f.buf, f.buf_size - 1, *f.editing))
            *f.editing = !*f.editing;
        if (!*f.editing) {
            try {
                if (f.type == 0) {
                    int* ip = static_cast<int*>(f.val_ptr);
                    int nv = std::max(f.min_i, std::min(f.max_i, std::stoi(f.buf)));
                    if (nv != *ip) {
                        *ip = nv;
                        snprintf(f.buf, f.buf_size, "%d", nv);
                    }
                } else {
                    double* dp = static_cast<double*>(f.val_ptr);
                    double nv = std::max(f.min_d, std::min(f.max_d, std::stod(f.buf)));
                    if (std::abs(nv - *dp) > 1e-9) {
                        *dp = nv;
                        snprintf(f.buf, f.buf_size, "%.4f", nv);
                    }
                }
            } catch (...) {
                if (f.type == 0)
                    snprintf(f.buf, f.buf_size, "%d", *static_cast<int*>(f.val_ptr));
                else
                    snprintf(f.buf, f.buf_size, "%.4f", *static_cast<double*>(f.val_ptr));
            }
        }
    };

    Field fields[] = {
        {"Grid Size:",        grid_buf_,     sizeof(grid_buf_),     &editing_grid_,       &cfg.grid_size,               0, 0,0, 5,30},
        {"Agents:",           agents_buf_,   sizeof(agents_buf_),   &editing_agents_,     &cfg.agent_count,             0, 0,0, 1,50},
        {"Food Count:",       food_count_buf_, sizeof(food_count_buf_), &editing_food_count_, &cfg.food_count,          0, 0,0, 1,500},
        {"Food Energy:",      food_energy_buf_, sizeof(food_energy_buf_), &editing_food_energy_, &cfg.food_energy,       0, 0,0, 1,500},
        {"Food Threshold:",   food_thresh_buf_, sizeof(food_thresh_buf_), &editing_food_thresh_, &cfg.food_reset_threshold, 0,0,0,1,200},
        {"Max Energy:",       max_energy_buf_, sizeof(max_energy_buf_), &editing_max_energy_, &cfg.max_energy,           0, 0,0, 1,1000},
        {"Energy Decay:",     energy_decay_buf_, sizeof(energy_decay_buf_), &editing_energy_decay_, &cfg.energy_decay,  0, 0,0, 0,100},
        {"Learning Rate:",    lr_buf_,       sizeof(lr_buf_),       &editing_lr_,         &cfg.learning_rate,           1, 1e-6,1.0, 0,0},
        {"Discount Factor:",  discount_buf_, sizeof(discount_buf_), &editing_discount_,   &cfg.discount_factor,         1, 0.0,1.0, 0,0},
        {"Explore Init:",     explore_init_buf_, sizeof(explore_init_buf_), &editing_explore_init_, &cfg.initial_explore_rate, 1,0.0,1.0,0,0},
        {"Explore Decay:",    explore_decay_buf_, sizeof(explore_decay_buf_), &editing_explore_decay_, &cfg.explore_decay, 1,0.9,1.0,0,0},
        {"Explore Min:",      explore_min_buf_, sizeof(explore_min_buf_), &editing_explore_min_, &cfg.min_explore_rate,   1, 0.0,1.0, 0,0},
        {"Buffer Size:",      buf_size_buf_, sizeof(buf_size_buf_), &editing_buf_size_,   &cfg.experience_buffer_size,  0, 0,0, 100,500000},
        {"Batch Size:",       batch_size_buf_, sizeof(batch_size_buf_), &editing_batch_size_, &cfg.batch_size,            0, 0,0, 1,1024},
        {"Target Freq:",      target_freq_buf_, sizeof(target_freq_buf_), &editing_target_freq_, &cfg.target_update_frequency, 0,0,0,1,10000},
        {"CPU Threads:",      max_threads_buf_, sizeof(max_threads_buf_), &editing_max_threads_, &cfg.max_threads,             0, 0,0, 0,64},
    };

    for (size_t i = 0; i < sizeof(fields)/sizeof(fields[0]); ++i) {
        int col_x = (i < 8) ? col1_x : col2_x;
        int row = (i < 8) ? static_cast<int>(i) : static_cast<int>(i) - 8;
        draw_field(fields[i], col_x, start_y + row * row_h);
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 11);
    const char* tip = "CPU Threads: 0 = auto (use all cores)";
    int tip_x = col2_x + label_w;
    int tip_y = start_y + (8 + 1) * row_h;
    DrawText(tip, tip_x, tip_y + 8, 11, CLITERAL(Color){100,108,118,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    if (GuiButton({(float)(px + 190), 420, 120, 32}, "Back"))
        current_state_ = MenuState::HOME;
    }

    EndDrawing();
}

// ── About ────────────────────────────────────────────────────────────────────

void Menu::do_about() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int sw = GetScreenWidth();

    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("About CUBES", 36);
    DrawText("About CUBES", (sw - tw) / 2, 40, 36, CLITERAL(Color){88,166,255,255});

    auto blurb = [&](const char* text, int y, int size) {
        GuiSetStyle(DEFAULT, TEXT_SIZE, size);
        tw = MeasureText(text, size);
        DrawText(text, (sw - tw) / 2, y, size, CLITERAL(Color){139,148,158,255});
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
    if (GuiButton({(float)(sw / 2 - 60), 500, 120, 40}, "Back"))
        current_state_ = MenuState::HOME;

    EndDrawing();
}
