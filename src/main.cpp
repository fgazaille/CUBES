#include "config.hpp"
#include "environment.hpp"
#include "renderer.hpp"
#include "menu.hpp"
#include <sstream>
#include <string>
#include <thread>
#include <fstream>
#include <iostream>

std::atomic<bool> g_training_stop_flag(false);

// ============================================================================
// Visual Simulation
// ============================================================================

static void run_simulation() {

    SCREEN_WIDTH  = GetScreenWidth();
    SCREEN_HEIGHT = GetScreenHeight();
    recalculate_layout();

    SetWindowTitle("CUBES - AI Learning Simulation");
    init_renderer(asset_prefix());

    Environment env(true);

    bool brain_loaded = false;
    if (std::ifstream(brain_file_path()).good()) {
        std::cout << "Loading assets/brain.json...\n";
        for (size_t i = 0; i < env.get_agents().size(); ++i) {
            try {
                env.get_agents()[i].load_brain_from_file(brain_file_path());
                brain_loaded = true;
            } catch (const std::exception& e) {
                std::cerr << "Failed to load brain for agent " << i << ": " << e.what() << "\n";
            }
        }
    }

    bool paused = false;
    bool time_warp_mode = DEFAULT_TIME_WARP_MODE;
    double time_warp_factor = DEFAULT_TIME_WARP_FACTOR;
    bool debug_mode = false;
    int selected_agent = -1;
    std::string message_text = "";
    double message_timer = 0.0;

    if (brain_loaded) {
        message_text = "Brain Loaded!";
        message_timer = GetTime() + 2.0;
    }

    SetTargetFPS(120);

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            SCREEN_WIDTH  = GetScreenWidth();
            SCREEN_HEIGHT = GetScreenHeight();
            recalculate_layout();
        }

        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (IsKeyPressed(KEY_R))      env.reset();

        if (IsKeyPressed(KEY_SPACE)) {
            if (debug_mode) {
                env.run_learning_step();
            } else {
                paused = !paused;
            }
        }

        if (IsKeyPressed(KEY_EQUAL))        time_warp_factor *= 2.0;
        if (IsKeyPressed(KEY_MINUS))        time_warp_factor /= 2.0;
        if (IsKeyPressed(KEY_ZERO))         time_warp_factor = 1.0;
        if (IsKeyPressed(KEY_W))            time_warp_mode = !time_warp_mode;

        if (IsKeyPressed(KEY_D)) {
            debug_mode = !debug_mode;
            if (debug_mode) paused = true;
        }

        if (debug_mode && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            if (mp.x < GRID_WIDTH && mp.y < GRID_HEIGHT) {
                int cx = (int)mp.x / CELL_SIZE;
                int cy = (int)mp.y / CELL_SIZE;
                selected_agent = -1;
                for (size_t i = 0; i < env.get_agents().size(); ++i) {
                    if (env.get_agents()[i].is_alive() &&
                        env.get_agents()[i].pos.x == cx &&
                        env.get_agents()[i].pos.y == cy) {
                        selected_agent = (int)i;
                        break;
                    }
                }
            }
        }

        if (IsKeyPressed(KEY_S)) {
            bool any = false;
            for (auto& agent : env.get_agents()) {
                if (agent.is_alive()) {
                    try {
                        agent.save_brain_to_file(brain_file_path());
                        message_text = "Brain Saved!";
                        message_timer = GetTime() + 2.0;
                        any = true;
                        break;
                    } catch (const std::exception& e) {
                        std::cerr << "Save error: " << e.what() << std::endl;
                    }
                }
            }
            if (!any) { message_text = "Save Failed!"; message_timer = GetTime() + 2.0; }
        }

        if (IsKeyPressed(KEY_L)) {
            bool any = false;
            for (size_t i = 0; i < env.get_agents().size(); ++i) {
                try {
                    env.get_agents()[i].load_brain_from_file(brain_file_path());
                    any = true;
                } catch (const std::exception& e) {
                    std::cerr << "Load error: " << e.what() << std::endl;
                }
            }
            message_text = any ? "Brain Loaded!" : "Load Failed! Save first with S";
            message_timer = GetTime() + 2.0;
        }

        if (!paused) {
            int steps = time_warp_mode ? (int)time_warp_factor : 1;
            for (int i = 0; i < steps; ++i) env.run_learning_step();
        }

        BeginDrawing();
        render_environment(env, time_warp_mode, time_warp_factor,
                           debug_mode, selected_agent);

        if (!message_text.empty() && GetTime() < message_timer) {
            int mw = MeasureText(message_text.c_str(), 24);
            DrawText(message_text.c_str(),
                     (GRID_WIDTH - mw) / 2, SCREEN_HEIGHT / 2,
                     24, UI::COL_GREEN);
        }

        if (debug_mode && paused) {
            DrawText("DEBUG MODE  (Space = step)", 10, 10, 14, UI::ACCENT);
        }

        EndDrawing();
    }

    close_renderer();
}

// ============================================================================
// Headless Training
// ============================================================================

static void run_training(const TrainingConfig& config, TrainingStatus* status) {
    g_training_stop_flag.store(false);
    status->active.store(true);
    status->episodes_done.store(0);
    status->total_episodes.store(config.episodes);
    status->best_food.store(0);
    status->auto_save = config.auto_save;

    status->thread = std::thread([status, config]() {
        auto& env = status->env;
        env = std::make_unique<Environment>(true);

        if (std::ifstream(brain_file_path()).good()) {
            std::cout << "Loading assets/brain.json...\n";
            for (size_t i = 0; i < env->get_agents().size(); ++i) {
                try {
                    env->get_agents()[i].load_brain_from_file(brain_file_path());
                } catch (const std::exception& e) {
                    std::cerr << "Failed to load brain: " << e.what() << "\n";
                }
            }
        }

        int last_best = -1;
        int episode_counter = 0;

        for (int ep = 0; ep < config.episodes &&
             !g_training_stop_flag.load(); ++ep) {

            env->run_learning_step();

            if (++episode_counter % 10 == 0)
                status->episodes_done.fetch_add(10);

            if (episode_counter % 100 == 0) {
                int cb = env->get_last_gen_best_food();

                {
                    std::lock_guard<std::mutex> hlock(status->history_mutex);
                    status->food_history.push_back({status->episodes_done.load(), cb});
                    if (status->food_history.size() > 5000)
                        status->food_history.erase(status->food_history.begin());
                }

                if (cb != last_best) {
                    last_best = cb;
                    std::cout << "Ep " << (ep + 1) << " Best: " << cb << " food\n";

                    if (cb > status->best_food.load()) {
                        status->best_food.store(cb);
                    }

                    if (config.auto_save) {
                        try {
                            // Save best agent's brain
                            int best_idx = 0;
                            for (size_t i = 0; i < env->get_agents().size(); ++i) {
                                if (env->get_agents()[i].total_food_eaten >
                                    env->get_agents()[best_idx].total_food_eaten)
                                    best_idx = i;
                            }
                            env->get_agents()[best_idx].save_brain_to_file(brain_file_path());
                        } catch (const std::exception& e) {
                            std::cerr << "Auto-save error: " << e.what() << "\n";
                        }
                    }
                }
            }
        }

        status->active.store(false);
        std::cout << "\nTraining complete! Best food: " << last_best << "\n";
    });
}

// ============================================================================
// Main
// ============================================================================

int main(int, char**) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "CUBES");
    MaximizeWindow();
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    TrainingStatus training_status;

    Menu menu(&training_status);
    menu.set_training_callback(run_training);

    bool running = true;
    while (running) {
        SetWindowTitle("CUBES");

        switch (menu.run()) {
            case MenuState::START_SIM: {
                // Join any lingering training thread
                if (training_status.thread.joinable()) {
                    g_training_stop_flag.store(true);
                    training_status.thread.join();
                    training_status.env.reset();
                    g_training_stop_flag.store(false);
                }
                run_simulation();
                break;
            }
            case MenuState::EXIT:
                running = false;
                break;
            default:
                break;
        }
    }

    // Final cleanup of training thread
    if (training_status.thread.joinable()) {
        g_training_stop_flag.store(true);
        training_status.thread.join();
    }
    training_status.env.reset();

    CloseWindow();
    return 0;
}
