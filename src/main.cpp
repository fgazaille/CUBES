/**
 * @file main.cpp
 * @brief Main entry point for CUBES AI Learning Simulation.
 */

#include "config.hpp"
#include "environment.hpp"
#include "rendering/sdl_utils.hpp"
#include "rendering/renderer.hpp"
#include "menu/menu.hpp"
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <fstream>
#include <csignal>

// Global stop flag for training
std::atomic<bool> g_training_stop_flag(false);

void signal_handler(int) {
    g_training_stop_flag.store(true);
    std::cout << "\nStop signal received, finishing episode...\n";
}

// ============================================================================
// Get asset path
// ============================================================================

std::string get_asset_path() {
    char* base_path = SDL_GetBasePath();
    std::string asset_path;
    
    if (base_path) {
        asset_path = base_path;
        SDL_free(base_path);
    } else {
        asset_path = "./";
    }
    
    // Check if path contains "build/"
    std::string build_marker = "build/";
    if (asset_path.find(build_marker) != std::string::npos) {
        size_t pos = asset_path.find(build_marker);
        asset_path.resize(pos);
        asset_path += "assets/";
    } else {
        asset_path += "assets/";
    }
    
    return asset_path;
}

// ============================================================================
// Run simulation
// ============================================================================

void run_simulation(const std::string& asset_path, const SimulationConfig& sim_config) {
    // Initialize SDL and create window/renderer
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    
    // Initialize SDL subsystems
    if (!init_sdl_subsystems()) {
        return;
    }
    
    // Create window and renderer
    window = SDL_CreateWindow("CUBES - AI Learning Simulation", 
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return;
    }
    
    renderer = SDL_CreateRenderer(window, -1, 
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        return;
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // Load fonts
    std::string font_path = asset_path + "Futura.ttf";
    TTF_Font* large_font = load_font(font_path.c_str(), 24);
    TTF_Font* regular_font = load_font(font_path.c_str(), 16);
    
    if (!large_font || !regular_font) {
        std::cerr << "Font loading failed: " << TTF_GetError() << std::endl;
        if (regular_font) TTF_CloseFont(regular_font);
        if (large_font) TTF_CloseFont(large_font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return;
    }
    
    // Load textures
    SDL_Texture* agent_tex = load_scaled_texture(renderer, asset_path + "Player.png", CELL_SIZE);
    SDL_Texture* food_tex = load_scaled_texture(renderer, asset_path + "Food.png", CELL_SIZE);
    
    // Initialize simulation
    Environment env(true);
    
    // Auto-load ./build/brain.json if it exists
    if (std::ifstream("./build/brain.json").good()) {
        std::cout << "Loading ./build/brain.json...\n";
        for (size_t i = 0; i < env.get_agents().size(); ++i) {
            try {
                env.get_agents()[i].load_brain_from_file("./build/brain.json");
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
    std::string message_text = "";
    Uint32 message_timer = 0;
    bool debug_mode = false;
    int selected_agent = -1;
    
    // Main loop
    bool running = true;
    SDL_Event event;
    Uint32 last_time = SDL_GetTicks();
    Uint32 frame_count = 0;
    Uint32 fps = 0;
    
    while (running) {
        Uint32 current_time = SDL_GetTicks();
        frame_count++;
        if (current_time - last_time >= 1000) {
            fps = frame_count;
            frame_count = 0;
            last_time = current_time;
        }
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN && debug_mode) {
                // Select agent on click in debug mode
                int mouse_x = event.button.x;
                int mouse_y = event.button.y;
                // Check if click is on the grid
                if (mouse_x < GRID_WIDTH && mouse_y < GRID_HEIGHT) {
                    int clicked_x = mouse_x / CELL_SIZE;
                    int clicked_y = mouse_y / CELL_SIZE;
                    // Find agent at this position
                    selected_agent = -1;
                    for (size_t i = 0; i < env.get_agents().size(); ++i) {
                        if (env.get_agents()[i].is_alive() &&
                            env.get_agents()[i].pos.x == clicked_x &&
                            env.get_agents()[i].pos.y == clicked_y) {
                            selected_agent = static_cast<int>(i);
                            break;
                        }
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_r: env.reset(); break;
                    case SDLK_SPACE: 
                        if (debug_mode) {
                            // Step one frame in debug mode
                            env.run_learning_step();
                        } else {
                            paused = !paused; 
                        }
                        break;
                    case SDLK_EQUALS: time_warp_factor *= 2.0; break;
                    case SDLK_MINUS: time_warp_factor /= 2.0; break;
                    case SDLK_0: time_warp_factor = 1.0; break;
                    case SDLK_w: time_warp_mode = !time_warp_mode; break;
                    case SDLK_d: 
                        debug_mode = !debug_mode;
                        if (debug_mode) paused = true; // Pause in debug mode
                        break;
                    case SDLK_s:
                         // Save brain of first alive agent
                         {
                             bool any_saved = false;
                             for (const auto& agent : env.get_agents()) {
                                 if (agent.is_alive()) {
                                     try {
                                         agent.save_brain_to_file("./build/brain.json");
                                        message_text = "Brain Saved!";
                                        message_timer = SDL_GetTicks() + 2000;
                                        any_saved = true;
                                        break;
                                    } catch (const std::exception& e) {
                                        std::cerr << "Save error: " << e.what() << std::endl;
                                    }
                                }
                            }
                            if (!any_saved) {
                                message_text = "Save Failed!";
                                message_timer = SDL_GetTicks() + 2000;
                            }
                        }
                        break;
                    case SDLK_l:
                         // Load brain for all agents
                         {
                             bool any_loaded = false;
                             for (size_t i = 0; i < env.get_agents().size(); ++i) {
                                 try {
                                     env.get_agents()[i].load_brain_from_file("./build/brain.json");
                                    any_loaded = true;
                                } catch (const std::exception& e) {
                                    std::cerr << "Load error: " << e.what() << std::endl;
                                }
                            }
                            if (any_loaded) {
                                message_text = "Brain Loaded!";
                            } else {
                                message_text = "Load Failed! Save first with S";
                            }
                            message_timer = SDL_GetTicks() + 2000;
                        }
                        break;
                    case SDLK_F5:
                        // Save brain of first alive agent
                        {
                            bool any_saved = false;
                            for (auto& agent : env.get_agents()) {
                                if (agent.is_alive()) {
                    try {
                                         agent.save_brain_to_file("./build/brain.json");
                                         message_text = "Brain Saved!";
                                         message_timer = SDL_GetTicks() + 2000;
                                         any_saved = true;
                                         break;
                                     } catch (const std::exception& e) {
                                         std::cerr << "Save error: " << e.what() << std::endl;
                                     }
                                 }
                             }
                             if (!any_saved) {
                                 message_text = "Save Failed!";
                                 message_timer = SDL_GetTicks() + 2000;
                             }
                         }
                         break;
                     case SDLK_F9:
                         // Load brain for all agents
                         {
                             bool any_loaded = false;
                             for (size_t i = 0; i < env.get_agents().size(); ++i) {
                                 try {
                                     env.get_agents()[i].load_brain_from_file("./build/brain.json");
                                    any_loaded = true;
                                } catch (const std::exception& e) {
                                    std::cerr << "Load error: " << e.what() << std::endl;
                                }
                            }
                            if (any_loaded) {
                                message_text = "Brain Loaded!";
                            } else {
                                message_text = "Load Failed! Save first with F5";
                            }
                            message_timer = SDL_GetTicks() + 2000;
                        }
                        break;
                }
            }
        }
        
        // Training mode: run one episode per frame (non-blocking)
        if (training_mode && training_remaining > 0) {
            env.run_learning_step();
            training_remaining--;
            
            if (training_remaining % 100 == 0) {
                std::cout << "Training episode: " << env.get_episode() 
                          << " Food eaten: " << env.get_agents()[0].total_food_eaten << "\n";
            }
            
            if (training_remaining <= 0) {
                training_mode = false;
                message_text = "Training Complete!";
                message_timer = SDL_GetTicks() + 2000;
                std::cout << "Training complete. Total episodes: " << env.get_episode() << "\n";
            }
        }
        
        if (!training_mode) {
            if (!paused) {
                if (time_warp_mode) {
                    for (int i = 0; i < static_cast<int>(time_warp_factor); ++i) {
                        env.run_learning_step();
                    }
                } else {
                    env.run_learning_step();
                }
            }
            
            render_environment(renderer, env, agent_tex, food_tex,
                                large_font, regular_font,
                                time_warp_mode, time_warp_factor,
                                debug_mode, selected_agent);
            
            if (regular_font) {
                std::stringstream fps_text;
                fps_text << "FPS: " << fps;
                render_text(renderer, regular_font, fps_text.str(),
                            10, 10, {200, 200, 0, 255});
                
                if (paused) {
                    render_text(renderer, large_font, "PAUSED",
                                SCREEN_WIDTH / 2 - 50, 10,
                                {255, 0, 0, 255});
                }
                
                if (!message_text.empty() && SDL_GetTicks() < message_timer) {
                    render_text(renderer, large_font, message_text,
                                SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2,
                                {0, 255, 0, 255});
                }
            }
            
            SDL_RenderPresent(renderer);
        }
        
        // Frame rate control
        if (!time_warp_mode && !training_mode) {
            if (sim_config.fps_cap > 0) {
                Uint32 frame_time = SDL_GetTicks() - current_time;
                Uint32 target_time = 1000 / sim_config.fps_cap;
                if (frame_time < target_time) {
                    SDL_Delay(target_time - frame_time);
                }
            } else {
                SDL_Delay(SLEEP_MS);
            }
        }
    }
    
    // Cleanup
    clear_text_cache();
    if (agent_tex) SDL_DestroyTexture(agent_tex);
    if (food_tex) SDL_DestroyTexture(food_tex);
    TTF_CloseFont(regular_font);
    TTF_CloseFont(large_font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv; // Unused parameters
    
    // Initialize SDL subsystems once
    if (!init_sdl_subsystems()) {
        return 1;
    }
    
    // Get asset path
    std::string asset_path = get_asset_path();
    std::string font_path = asset_path + "Futura.ttf";
    
    // Main loop - keeps returning to menu after actions
    bool running = true;
    while (running) {
        // Create menu window and renderer
        SDL_Window* menu_window = SDL_CreateWindow(
            "CUBES - Home",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            800, 600, SDL_WINDOW_SHOWN);
        
        if (!menu_window) {
            std::cerr << "Menu window failed: " << SDL_GetError() << std::endl;
            break;
        }
        
        SDL_Renderer* menu_renderer = SDL_CreateRenderer(
            menu_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        
        if (!menu_renderer) {
            SDL_DestroyWindow(menu_window);
            break;
        }
        
        SDL_SetRenderDrawBlendMode(menu_renderer, SDL_BLENDMODE_BLEND);
        
        // Load menu fonts using raw pointers
        TTF_Font* title_font = load_font(font_path.c_str(), 48);
        TTF_Font* button_font = load_font(font_path.c_str(), 24);
        TTF_Font* text_font = load_font(font_path.c_str(), 18);
        
        if (!title_font || !button_font || !text_font) {
            std::cerr << "Menu font loading failed" << std::endl;
            break;
        }
        
        // Run menu
        Menu menu(menu_renderer, title_font, button_font, text_font);
        MenuState result = menu.run();
        
        // Cleanup menu
        TTF_CloseFont(text_font);
        TTF_CloseFont(button_font);
        TTF_CloseFont(title_font);
        SDL_DestroyRenderer(menu_renderer);
        SDL_DestroyWindow(menu_window);
        
        // Act on menu choice
        if (result == MenuState::START_SIM) {
            SimulationConfig sim_config = menu.get_simulation_config();
            run_simulation(asset_path, sim_config);
            } else if (result == MenuState::TRAINING) {
            // Get training config from menu
            TrainingConfig config = menu.get_training_config();
            std::cout << "Starting training (" << config.episodes << " episodes, " 
                      << config.threads << " threads)...\n";
            
            // Set menu to training active state
            // (In a real implementation, we'd need to pass a reference to update menu state)
            // For now, just run training and return to menu

            
            // Load brain before training if requested
             if (config.load_brain) {
                 std::cout << "Loading ./build/brain.json...\n";
             }
            
            // Multi-threaded training
            std::vector<std::thread> threads;
            std::vector<std::unique_ptr<Environment>> envs;
            std::vector<int> food_counts(config.threads, 0);
            
            // Create environments for each thread
            for (int t = 0; t < config.threads; ++t) {
                envs.push_back(std::make_unique<Environment>(true));
                std::cout << "Thread " << t << " environment created with " 
                          << envs[t]->get_agents().size() << " agents\n";
                
                // Load brain if requested
                 if (config.load_brain) {
                     for (size_t i = 0; i < envs[t]->get_agents().size(); ++i) {
                         try {
                             envs[t]->get_agents()[i].load_brain_from_file("./build/brain.json");
                        } catch (const std::exception& e) {
                            std::cerr << "Failed to load brain for thread " << t << ": " << e.what() << "\n";
                        }
                    }
                }
            }
            
            // Launch training threads
            g_training_stop_flag.store(false);
            std::signal(SIGINT, signal_handler);
            
            std::cout << "Training started: " << config.episodes << " episodes, " 
                      << config.threads << " threads\n";
            std::cout << "Press Ctrl+C to stop early\n";
            std::cout << "Progress file: training_progress.txt\n\n";
            
            // Create progress file
            std::ofstream progress("training_progress.txt");
            if (progress) {
                progress << "Training started: " << config.episodes << " episodes, " 
                            << config.threads << " threads\n";
                progress.close();
            }
            
            // Track global best for saving during training
            std::atomic<int> global_best_food(0);
            std::mutex best_brain_mutex;  // Protect ./build/brain.json from concurrent writes
            
            for (int t = 0; t < config.threads; ++t) {
                threads.emplace_back([&, t]() {
                    auto& env = *envs[t];
                    int best_food_in_thread = 0;
                    int best_agent_in_thread = 0;
                    
                    for (int train_ep = 0; train_ep < config.episodes && !g_training_stop_flag.load(); ++train_ep) {
                        env.run_learning_step();
                        
                        if ((train_ep + 1) % 100 == 0) {
                            int current_best = 0;
                            int current_best_idx = 0;
                            for (size_t i = 0; i < env.get_agents().size(); ++i) {
                                if (env.get_agents()[i].total_food_eaten > current_best) {
                                    current_best = env.get_agents()[i].total_food_eaten;
                                    current_best_idx = i;
                                }
                            }
                            std::cout << "[Thread " << t << "] Ep " << (train_ep + 1) 
                                      << " Best: " << current_best << " food\n";
                            
                            // Update progress file
                            std::ofstream prog("training_progress.txt", std::ios::app);
                            if (prog) {
                                prog << "Thread " << t << ": Episode " << (train_ep + 1) 
                                    << ", Best food: " << current_best << "\n";
                            }
                            
                            // Save best brain if improved globally (with 10% threshold)
                            if (current_best > global_best_food.load() * 1.1) {  // Only update if 10% better
                                global_best_food.store(current_best);
                                std::lock_guard<std::mutex> lock(best_brain_mutex);
                                try {
                                     env.get_agents()[current_best_idx].save_brain_to_file("./build/brain.json");
                                } catch (...) { /* ignore save errors */ }
                            }
                        }
                    }
                    
                    // Find best agent after training
                    for (size_t i = 0; i < env.get_agents().size(); ++i) {
                        if (env.get_agents()[i].total_food_eaten > best_food_in_thread) {
                            best_food_in_thread = env.get_agents()[i].total_food_eaten;
                            best_agent_in_thread = i;
                        }
                    }
                    food_counts[t] = best_food_in_thread;
                    std::cout << "[Thread " << t << "] Done. Best: " << best_food_in_thread 
                              << " (agent " << best_agent_in_thread << ")\n";
                });
            }
            
            // Wait for all threads
            for (auto& th : threads) {
                if (th.joinable()) th.join();
            }
            std::cout << "\nTraining complete!\n";
            
            // Find best result
            int best_thread = 0;
            for (int t = 1; t < config.threads; ++t) {
                if (food_counts[t] > food_counts[best_thread]) {
                    best_thread = t;
                }
            }
            
            // Find best agent in best thread
            int best_agent_idx = 0;
            auto& best_env = *envs[best_thread];
            for (size_t i = 1; i < best_env.get_agents().size(); ++i) {
                if (best_env.get_agents()[i].total_food_eaten > 
                    best_env.get_agents()[best_agent_idx].total_food_eaten) {
                    best_agent_idx = i;
                }
            }
            
            // Write training summary to file
            std::ofstream summary("training_summary.txt");
            if (summary) {
                summary << "Training Summary\n";
                summary << "===============\n\n";
                summary << "Episodes per thread: " << config.episodes << "\n";
                summary << "Threads: " << config.threads << "\n\n";
                summary << "Results:\n";
                for (int t = 0; t < config.threads; ++t) {
                    summary << "  Thread " << t << ": " << food_counts[t] << " food\n";
                }
                summary << "\nBest: Thread " << best_thread << " Agent " << best_agent_idx 
                          << " with " << food_counts[best_thread] << " food\n";
                summary.close();
                std::cout << "Summary saved to training_summary.txt\n";
            }
            
            std::cout << "Best result: Thread " << best_thread 
                      << " Agent " << best_agent_idx
                      << " with " << food_counts[best_thread] << " food eaten\n";
            
            if (config.auto_save) {
                 envs[best_thread]->get_agents()[best_agent_idx].save_brain_to_file("./build/brain.json");
                 std::cout << "Best brain auto-saved to ./build/brain.json\n";
            }
            
            std::cout << "Returning to menu...\n";
            continue; // Go back to menu
        } else if (result == MenuState::EXIT) {
            running = false;
        }
    }
    
    // Final cleanup
    quit_sdl();
    
    return 0;
}
