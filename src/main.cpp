#include "config.hpp"
#include "environment.hpp"
#include "renderer.hpp"
#include "menu.hpp"
#include "debug_repl.hpp"
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
            int gs = RuntimeConfig::instance().grid_size;
            int cs = std::max(1, std::min(GetScreenWidth(), GetScreenHeight()) / gs);
            int gpw = cs * gs;
            if (mp.x < gpw && mp.y < gpw) {
                int cx = (int)mp.x / cs;
                int cy = (int)mp.y / cs;
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
            int gs = RuntimeConfig::instance().grid_size;
            int gpw = std::max(1, std::min(GetScreenWidth(), GetScreenHeight()) / gs) * gs;
            int mw = MeasureText(message_text.c_str(), 24);
            DrawText(message_text.c_str(),
                     (gpw - mw) / 2, GetScreenHeight() / 2,
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

static void run_env_worker(ParallelEnvState* state, int ep_per_env,
                           std::atomic<int>& global_best, std::atomic<int>& global_total,
                           std::atomic<int>& global_done, std::atomic<bool>& stop_flag)
{
    auto& env = state->env;

    if (std::ifstream(brain_file_path()).good()) {
        for (size_t i = 0; i < env->get_agents().size(); ++i) {
            try {
                env->get_agents()[i].load_brain_from_file(brain_file_path());
            } catch (...) {}
        }
    }

    int local_done = 0;
    for (int ep = 0; ep < ep_per_env && !stop_flag.load(); ++ep) {
        env->run_learning_step();

        if (++local_done % 10 == 0) {
            state->episodes_done.store(local_done);
            global_done.fetch_add(10);
        }

        int lb = env->get_best_food_ever();
        int cur = global_best.load();
        while (lb > cur && !global_best.compare_exchange_weak(cur, lb));

        state->local_best.store(lb);
        state->local_total.store(env->get_total_food_all_time());

        int gt = env->get_total_food_all_time();
        global_total.store(gt);
    }
    state->episodes_done.store(local_done);
}

static void run_training(const TrainingConfig& config, TrainingStatus* status) {
    g_training_stop_flag.store(false);
    status->active.store(true);
    status->episodes_done.store(0);
    status->total_episodes.store(config.episodes);
    status->best_food.store(0);
    status->total_food_all_time.store(0);
    status->auto_save = config.auto_save;
    status->parallel_count = config.parallel_envs;
    status->last_saved_best_.store(0);

    if (status->thread.joinable())
        status->thread.join();

    status->envs.clear();
    int num_envs = std::max(1, config.parallel_envs);
    int eps_per_env = (config.episodes + num_envs - 1) / num_envs;

    for (int i = 0; i < num_envs; ++i) {
        auto s = std::make_unique<ParallelEnvState>();
        s->env = std::make_unique<Environment>(true);
        status->envs.push_back(std::move(s));
    }

    status->thread = std::thread([status, num_envs, eps_per_env]() {
        std::atomic<int> global_done{0};

        for (int i = 0; i < num_envs; ++i) {
            status->env_pool.detach_task([status, i, eps_per_env, &global_done]() {
                run_env_worker(status->envs[i].get(), eps_per_env,
                               status->best_food, status->total_food_all_time,
                               global_done, g_training_stop_flag);
            });
        }

        int report_counter = 0;
        int last_best = -1;
        while (status->active.load() && !g_training_stop_flag.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            status->episodes_done.store(global_done.load());

            if (++report_counter % 50 == 0) {
                int cb = status->best_food.load();
                int total = status->total_food_all_time.load();

                {
                    std::lock_guard<std::mutex> hlock(status->history_mutex);
                    status->food_history.push_back({status->episodes_done.load(), cb, total});
                    if (status->food_history.size() > 5000)
                        status->food_history.erase(status->food_history.begin());
                }

                if (status->auto_save) {
                    int save_threshold = status->last_saved_best_.load() + 5;
                    if (cb >= save_threshold) {
                        status->last_saved_best_.store(cb);
                        int best_env_idx = 0;
                        int best_found = 0;
                        for (size_t e = 0; e < status->envs.size(); ++e) {
                            int le = status->envs[e]->local_best.load();
                            if (le > best_found) {
                                best_found = le;
                                best_env_idx = (int)e;
                            }
                        }
                        auto& agents = status->envs[best_env_idx]->env->get_agents();
                        int best_agent = 0;
                        for (size_t a = 0; a < agents.size(); ++a) {
                            if (agents[a].total_food_eaten > agents[best_agent].total_food_eaten)
                                best_agent = (int)a;
                        }
                        try {
                            agents[best_agent].save_brain_to_file(brain_file_path());
                            std::cout << "[" << status->episodes_done.load() << " steps] Brain saved: " << cb << " food\n";
                        } catch (...) {}
                    }
                }

                if (cb != last_best) {
                    last_best = cb;
                    std::cout << "Parallel step " << status->episodes_done.load() << " Best: " << cb << " food\n";
                }
            }

            bool all_done = true;
            for (auto& e : status->envs) {
                if (e->episodes_done.load() < eps_per_env) {
                    all_done = false;
                    break;
                }
            }
            if (all_done || g_training_stop_flag.load()) break;
        }

        status->env_pool.wait();
        status->active.store(false);
        std::cout << "\nTraining complete! Best food: " << status->best_food.load() << "\n";
    });
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--repl") {
            run_debug_repl();
            return 0;
        }
        if (std::string(argv[i]) == "--brain" && i + 1 < argc) {
            std::string src = argv[++i];
            std::string dst = brain_file_path();
            std::ifstream in(src, std::ios::binary);
            if (!in) {
                std::cerr << "Failed to open brain: " << src << "\n";
                return 1;
            }
            std::ofstream out(dst, std::ios::binary);
            out << in.rdbuf();
            std::cout << "Preloaded brain: " << src << "\n";
        }
    }

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
                if (training_status.thread.joinable()) {
                    g_training_stop_flag.store(true);
                    training_status.thread.join();
                    training_status.envs.clear();
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

    if (training_status.thread.joinable()) {
        g_training_stop_flag.store(true);
        training_status.thread.join();
    }
    training_status.envs.clear();

    CloseWindow();
    return 0;
}
