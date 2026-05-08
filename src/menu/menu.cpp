#define RAYGUI_IMPLEMENTATION
#include "menu.hpp"
#include "environment.hpp"
#include <sstream>
#include <cmath>
#include <cstring>

// ── Ctor / Dtor ──────────────────────────────────────────────────────────────

Menu::Menu(TrainingStatus* training_status)
    : training_status_ptr_(training_status) {
    // Pre-fill edit buffers
    snprintf(ep_buf_, sizeof(ep_buf_), "%d", training_config_.episodes);
    snprintf(th_buf_, sizeof(th_buf_), "%d", training_config_.threads);
    // raygui style
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

Menu::~Menu() {}

// ── Core loop ────────────────────────────────────────────────────────────────

MenuState Menu::run() {
    while (true) {
        if (WindowShouldClose()) return MenuState::EXIT;

        switch (current_state_) {
            case MenuState::HOME:            do_home(); break;
            case MenuState::TRAINING_SCREEN: do_training_config(); break;
            case MenuState::TRAINING_ACTIVE: do_training_active(); break;
            case MenuState::SETTINGS:        do_settings(); break;
            case MenuState::ABOUT:           do_about(); break;
            default: return current_state_;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            switch (current_state_) {
                case MenuState::HOME:            current_state_ = MenuState::EXIT; break;
                case MenuState::TRAINING_SCREEN: current_state_ = MenuState::HOME; break;
                case MenuState::SETTINGS:        current_state_ = MenuState::HOME; break;
                case MenuState::ABOUT:           current_state_ = MenuState::HOME; break;
                case MenuState::TRAINING_ACTIVE:
                    g_training_stop_flag.store(true);
                    if (training_status_ptr_)
                        training_status_ptr_->stop_flag.store(true);
                    current_state_ = MenuState::HOME;
                    break;
                default: break;
            }
        }

        if (current_state_ == MenuState::START_SIM ||
            current_state_ == MenuState::TRAINING ||
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

    // Animated background rows
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

    // Title
    GuiSetStyle(DEFAULT, TEXT_SIZE, 48);
    int tw = MeasureText("CUBES", 48);
    DrawText("CUBES", (cw - tw) / 2, 50, 48, CLITERAL(Color){88,166,255,255});

    // Subtitle
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    int sw = MeasureText("AI Learning Simulation", 18);
    DrawText("AI Learning Simulation", (cw - sw) / 2, 110, 18, CLITERAL(Color){139,148,158,255});

    // Hint
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    int hw = MeasureText("Use mouse or ENTER to choose. ESC to return.", 14);
    DrawText("Use mouse or ENTER to choose. ESC to return.",
             (cw - hw) / 2, 140, 14, CLITERAL(Color){100,108,118,255});

    // Buttons
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    struct Btn { const char* label; MenuState target; };
    Btn btns[] = {
        {"Start Simulation", MenuState::START_SIM},
        {"Training",         MenuState::TRAINING_SCREEN},
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

    // Brain status
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

    // Title
    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("Training Configuration", 36);
    DrawText("Training Configuration", (cw - tw) / 2, 25, 36, CLITERAL(Color){88,166,255,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    int sw = MeasureText("Quickly tune training parameters before launching.", 14);
    DrawText("Quickly tune training parameters before launching.",
             (cw - sw) / 2, 75, 14, CLITERAL(Color){139,148,158,255});

    // Panel
    int px = (cw - 500) / 2;
    DrawRectangleRounded({(float)px, 110, 500, 320}, 0.1f, 8, CLITERAL(Color){22,27,34,255});
    DrawRectangleRoundedLines({(float)px, 110, 500, 320}, 0.1f, 8, CLITERAL(Color){48,54,61,255});

    int lx = px + 30;   // label x
    int fx = px + 170;  // field x

    // ── Episodes text box ────────────────────────────────────────────────
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    DrawText("Episodes:", lx, 140, 14, CLITERAL(Color){201,209,217,255});
    Rectangle ep_r = {(float)fx, 135, 160, 30};

    if (GuiTextBox(ep_r, ep_buf_, 31, editing_episodes_))
        editing_episodes_ = !editing_episodes_;

    if (!editing_episodes_) {
        try { training_config_.episodes = std::max(100, std::min(500000, std::stoi(ep_buf_))); }
        catch (...) {}
    }

    // ── Threads text box ─────────────────────────────────────────────────
    DrawText("Threads:", lx, 190, 14, CLITERAL(Color){201,209,217,255});
    Rectangle th_r = {(float)fx, 185, 160, 30};

    if (GuiTextBox(th_r, th_buf_, 31, editing_threads_))
        editing_threads_ = !editing_threads_;

    if (!editing_threads_) {
        try { training_config_.threads = std::max(1, std::min(16, std::stoi(th_buf_))); }
        catch (...) {}
    }

    // ── Toggles ──────────────────────────────────────────────────────────
    DrawText("Auto-save:", lx, 240, 14, CLITERAL(Color){201,209,217,255});
    Rectangle as_r = {(float)fx, 235, 80, 28};
    if (GuiButton(as_r, training_config_.auto_save ? "ON" : "OFF"))
        training_config_.auto_save = !training_config_.auto_save;

    DrawText("Load Brain:", lx, 280, 14, CLITERAL(Color){201,209,217,255});
    Rectangle lb_r = {(float)fx, 275, 80, 28};
    if (GuiButton(lb_r, training_config_.load_brain ? "ON" : "OFF"))
        training_config_.load_brain = !training_config_.load_brain;

    // ── Quick episode preset buttons ─────────────────────────────────────
    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    struct { const char* label; int val; } qbtns[] = {
        {"1K", 1000}, {"10K", 10000}, {"50K", 50000}, {"100K", 100000}
    };
    Rectangle qb = {210, 340, 65, 28};
    for (auto& q : qbtns) {
        if (GuiButton(qb, q.label)) {
            training_config_.episodes = q.val;
            snprintf(ep_buf_, sizeof(ep_buf_), "%d", q.val);
        }
        qb.x += qb.width + 12;
    }

    // ── Start / Back ─────────────────────────────────────────────────────
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    int btn_w = 140, btn_h = 40, gap = 20;
    int start_x = (cw - 2 * btn_w - gap) / 2;
    if (GuiButton({(float)start_x, 400, (float)btn_w, (float)btn_h}, "Start Training")) {
        if (training_start_callback_)
            training_start_callback_(training_config_, training_status_ptr_);
        // Pre-fill buffers for next visit
        snprintf(ep_buf_, sizeof(ep_buf_), "%d", training_config_.episodes);
        snprintf(th_buf_, sizeof(th_buf_), "%d", training_config_.threads);
        current_state_ = MenuState::TRAINING_ACTIVE;
    }
    if (GuiButton({(float)(start_x + btn_w + gap), 400, (float)btn_w, (float)btn_h}, "Back")) {
        current_state_ = MenuState::HOME;
    }

    // Tip
    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    const char* tip = "ENTER to start, ESC to go back";
    int tix = (cw - MeasureText(tip, 13)) / 2;
    DrawText(tip, tix, 480, 13, CLITERAL(Color){100,108,118,255});

    EndDrawing();
}

// ── Training Active ──────────────────────────────────────────────────────────

void Menu::do_training_active() {
    BeginDrawing();
    ClearBackground(CLITERAL(Color){13,17,23,255});

    int cw = GetScreenWidth();

    // Gather progress
    int done = 0, total = 0, threads = 0;
    if (training_status_ptr_ && training_status_ptr_->active.load()) {
        done    = training_status_ptr_->episodes_done.load();
        total   = training_status_ptr_->total_episodes.load();
        threads = training_status_ptr_->total_threads.load();
    }
    float prog = (total > 0) ? (float)done / total : 0.0f;
    prog = std::max(0.0f, std::min(1.0f, prog));

    // Title
    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    int tw = MeasureText("Training in Progress", 36);
    DrawText("Training in Progress", (cw - tw) / 2, 50, 36, CLITERAL(Color){88,166,255,255});

    // Info
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    std::string info = "Threads: " + std::to_string(threads)
                     + "  |  Total episodes: " + std::to_string(total);
    int iw = MeasureText(info.c_str(), 16);
    DrawText(info.c_str(), (cw - iw) / 2, 120, 16, CLITERAL(Color){139,148,158,255});

    // Progress bar
    Rectangle bar = {(float)(cw / 2 - 250), 200, 500, 40};
    DrawRectangleRounded(bar, 0.15f, 8, CLITERAL(Color){48,54,61,255});

    if (prog > 0) {
        Rectangle fill = {bar.x, bar.y, bar.width * prog, bar.height};
        DrawRectangleRounded(fill, 0.15f, 8, CLITERAL(Color){63,185,80,255});
    }
    DrawRectangleRoundedLines(bar, 0.15f, 8, CLITERAL(Color){100,108,118,255});

    // Text
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    std::string pt = std::to_string(done) + " / " + std::to_string(total) + " episodes";
    int pw = MeasureText(pt.c_str(), 18);
    DrawText(pt.c_str(), (cw - pw) / 2, 255, 18, CLITERAL(Color){201,209,217,255});

    GuiSetStyle(DEFAULT, TEXT_SIZE, 48);
    std::string pct = std::to_string((int)(prog * 100)) + "%";
    int ppw = MeasureText(pct.c_str(), 48);
    DrawText(pct.c_str(), (cw - ppw) / 2, 300, 48, CLITERAL(Color){63,185,80,255});

    // Stop button
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    if (GuiButton({(float)(cw / 2 - 60), 400, 120, 50}, "STOP")) {
        g_training_stop_flag.store(true);
        if (training_status_ptr_)
            training_status_ptr_->stop_flag.store(true);
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
    int tipw = MeasureText("ESC to cancel training", 13);
    DrawText("ESC to cancel training", (cw - tipw) / 2, 480, 13, CLITERAL(Color){100,108,118,255});

    // Check completion
    if (training_status_ptr_ && !training_status_ptr_->active.load())
        current_state_ = MenuState::TRAINING_SCREEN;

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

    // Back button
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

// ── Status helpers ───────────────────────────────────────────────────────────

void Menu::set_training_active(bool active, int total_episodes, int threads) {
    if (training_status_ptr_ && active) {
        training_status_ptr_->active.store(true);
        training_status_ptr_->episodes_done.store(0);
        training_status_ptr_->total_episodes.store(total_episodes);
        training_status_ptr_->total_threads.store(threads);
    }
    current_state_ = MenuState::TRAINING_ACTIVE;
}

bool Menu::is_training_active() const {
    return training_status_ptr_ && training_status_ptr_->active.load();
}
