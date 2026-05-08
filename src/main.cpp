#include "config.hpp"
#include "environment.hpp"
#include "renderer.hpp"
#include "menu.hpp"
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <fstream>
#include <csignal>

std::atomic<bool> g_training_stop_flag(false);

void signal_handler(int) {
    g_training_stop_flag.store(true);
    std::cout << "\nStop signal received, finishing episode...\n";
}

// ============================================================================
// Simulation
// ============================================================================

static void run_simulation(const SimulationConfig& sim_config) {

    SCREEN_WIDTH  = GetScreenWidth();
    SCREEN_HEIGHT = GetScreenHeight();
    recalculate_layout();

    SetWindowTitle("CUBES - AI Learning Simulation");
    init_renderer(asset_prefix());

    Environment env(true);

    // Auto-load brain
    if (std::ifstream(brain_file_path()).good()) {
        std::cout << "Loading assets/brain.json...\n";
        for (size_t i = 0; i < env.get_agents().size(); ++i) {
            try {
                env.get_agents()[i].load_brain_from_file(brain_file_path());
            } catch (const std::exception& e) {
                std::cerr << "Failed to load brain for agent " << i << ": " << e.what() << "\n";
            }
        }
    }

    // State
    bool paused = false;
    bool time_warp_mode = DEFAULT_TIME_WARP_MODE;
    double time_warp_factor = DEFAULT_TIME_WARP_FACTOR;
    bool training_mode = false;
    int training_remaining = 0;
    bool debug_mode = false;
    int selected_agent = -1;
    std::string message_text = "";
    double message_timer = 0.0;

    SetTargetFPS(sim_config.fps_cap > 0 ? sim_config.fps_cap : 120);

    while (!WindowShouldClose()) {
        // ── Handle resize ──────────────────────────────────────────────
        if (IsWindowResized()) {
            SCREEN_WIDTH  = GetScreenWidth();
            SCREEN_HEIGHT = GetScreenHeight();
            recalculate_layout();
        }

        // ── Input ───────────────────────────────────────────────────────
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

        // Debug mode agent selection
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

        // Save / Load
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

        // ── Update ──────────────────────────────────────────────────────
        if (training_mode && training_remaining > 0) {
            env.run_learning_step();
            training_remaining--;
            if (training_remaining % 100 == 0)
                std::cout << "Training episode: " << env.get_episode()
                          << " Food eaten: " << env.get_agents()[0].total_food_eaten << "\n";
            if (training_remaining <= 0) {
                training_mode = false;
                message_text = "Training Complete!";
                message_timer = GetTime() + 2.0;
                std::cout << "Training complete. Total episodes: " << env.get_episode() << "\n";
            }
        }

        if (!training_mode && !paused) {
            int steps = time_warp_mode ? (int)time_warp_factor : 1;
            for (int i = 0; i < steps; ++i) env.run_learning_step();
        }

        // ── Render ──────────────────────────────────────────────────────
        BeginDrawing();
        render_environment(env, time_warp_mode, time_warp_factor,
                           debug_mode, selected_agent);

        // Messages overlay
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
// Main
// ============================================================================

int main(int, char**) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "CUBES");
    MaximizeWindow();
    SetTargetFPS(60);

    SetExitKey(KEY_NULL);  // we handle ESC ourselves

    // Training status (persistent across menu cycles)
    TrainingStatus training_status;

    // Training callback
    auto training_callback = [&](const TrainingConfig& config, TrainingStatus* status) {
        for (auto& th : status->threads)
            if (th.joinable()) th.join();

        g_training_stop_flag.store(false);
        status->stop_flag.store(false);
        status->active.store(true);
        status->episodes_done.store(0);
        status->total_episodes.store(config.episodes * config.threads);
        status->threads_done.store(0);
        status->total_threads.store(config.threads);
        status->global_best_food.store(0);
        status->food_counts.assign(config.threads, 0);
        status->auto_save = config.auto_save;

        std::cout << "Starting training (" << config.episodes << " eps/thread, "
                  << config.threads << " threads)...\n";

        if (config.load_brain)
            std::cout << "Loading assets/brain.json...\n";

        status->envs.clear();
        status->threads.clear();

        for (int t = 0; t < config.threads; ++t) {
            status->envs.push_back(std::make_unique<Environment>(true));
            std::cout << "Thread " << t << " created with "
                      << status->envs[t]->get_agents().size() << " agents\n";

            if (config.load_brain) {
                for (size_t i = 0; i < status->envs[t]->get_agents().size(); ++i) {
                    try {
                        status->envs[t]->get_agents()[i].load_brain_from_file(brain_file_path());
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to load brain for thread " << t << ": " << e.what() << "\n";
                    }
                }
            }
        }

        std::signal(SIGINT, signal_handler);

        std::ofstream progress("training_progress.txt");
        if (progress) {
            progress << "Training started: " << config.episodes << " episodes, "
                     << config.threads << " threads\n";
            progress.close();
        }

        for (int t = 0; t < config.threads; ++t) {
            status->threads.emplace_back([&, t, config, status]() {
                auto& env = *status->envs[t];
                int best_food_in_thread = 0;
                int best_agent_in_thread = 0;

                for (int ep = 0; ep < config.episodes &&
                     !g_training_stop_flag.load() &&
                     !status->stop_flag.load(); ++ep) {
                    env.run_learning_step();
                    status->episodes_done.fetch_add(1);

                    if ((ep + 1) % 100 == 0) {
                        int cb = 0, cbi = 0;
                        for (size_t i = 0; i < env.get_agents().size(); ++i) {
                            if (env.get_agents()[i].total_food_eaten > cb) {
                                cb = env.get_agents()[i].total_food_eaten;
                                cbi = (int)i;
                            }
                        }
                        std::cout << "[Thread " << t << "] Ep " << (ep + 1)
                                  << " Best: " << cb << " food\n";

                        std::ofstream prog("training_progress.txt", std::ios::app);
                        if (prog) {
                            prog << "Thread " << t << ": Episode " << (ep + 1)
                                 << ", Best food: " << cb << "\n";
                        }

                        int cg = status->global_best_food.load();
                        if (cb > cg * 1.1) {
                            status->global_best_food.store(cb);
                            std::lock_guard<std::mutex> lock(status->best_brain_mutex);
                            try {
                                env.get_agents()[cbi].save_brain_to_file(brain_file_path());
                            } catch (...) { }
                        }
                    }
                }

                for (size_t i = 0; i < env.get_agents().size(); ++i) {
                    if (env.get_agents()[i].total_food_eaten > best_food_in_thread) {
                        best_food_in_thread = env.get_agents()[i].total_food_eaten;
                        best_agent_in_thread = (int)i;
                    }
                }
                status->food_counts[t] = best_food_in_thread;

                int done = status->threads_done.fetch_add(1) + 1;
                std::cout << "[Thread " << t << "] Done. Best: " << best_food_in_thread
                          << " (agent " << best_agent_in_thread << ")\n";

                if (done >= config.threads) {
                    status->active.store(false);
                    std::cout << "\nTraining complete! Returning to menu...\n";
                }
            });
        }
    };

    Menu menu(&training_status);
    menu.set_training_callback(training_callback);

    bool running_menu = true;

    while (running_menu) {
        SetWindowTitle("CUBES");

        MenuState result = menu.run();

        // Join any remaining training threads
        for (auto& th : training_status.threads)
            if (th.joinable()) th.join();
        training_status.threads.clear();
        training_status.envs.clear();
        training_status.active.store(false);

        switch (result) {
            case MenuState::START_SIM: {
                SimulationConfig sc = menu.get_simulation_config();
                run_simulation(sc);
                break;
            }
            case MenuState::EXIT:
                running_menu = false;
                break;
            default:
                break;
        }
    }

    CloseWindow();
    return 0;
}
