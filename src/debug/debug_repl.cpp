#include "debug_repl.hpp"
#include "config.hpp"
#include "environment.hpp"
#include "json.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <atomic>

using json = nlohmann::json;

static json serialize_state(const Environment& env) {
    json j;
    j["episode"] = env.get_episode();
    j["alive"] = env.get_alive_count();
    j["total_agents"] = static_cast<int>(env.get_agents().size());
    j["total_food_spawned"] = env.get_total_food_spawned();
    j["avg_food_per_episode"] = env.get_avg_food_per_episode();
    j["last_gen_best_food"] = env.get_last_gen_best_food();
    j["best_food_ever"] = env.get_best_food_ever();
    j["current_episode_food"] = env.get_current_episode_food();
    j["total_food_all_time"] = env.get_total_food_all_time();
    j["avg_exploration_rate"] = env.get_avg_exploration_rate();

    json agents_json = json::array();
    for (const auto& agent : env.get_agents()) {
        json a;
        a["alive"] = agent.is_alive();
        a["pos"] = {{"x", agent.pos.x}, {"y", agent.pos.y}};
        a["energy"] = agent.energy;
        a["total_food_eaten"] = agent.total_food_eaten;
        a["explore_rate"] = agent.get_explore_rate();
        agents_json.push_back(a);
    }
    j["agents"] = agents_json;

    json food_json = json::array();
    for (const auto& f : env.get_food_list()) {
        json fj;
        fj["pos"] = {{"x", f.pos.x}, {"y", f.pos.y}};
        fj["energy_value"] = f.energy_value;
        food_json.push_back(fj);
    }
    j["food"] = food_json;

    return j;
}

void run_debug_repl() {
    Environment env(true);

    std::cout << "{\"ok\":true,\"result\":\"CUBES debug REPL ready\"}\n" << std::flush;

    std::string line;
    while (std::getline(std::cin, line)) {
        json response;
        try {
            json cmd = json::parse(line);
            std::string command = cmd.value("cmd", "");

            if (command == "quit") {
                std::cout << "{\"ok\":true,\"result\":\"bye\"}\n" << std::flush;
                break;
            } else if (command == "step") {
                int n = cmd.value("n", 1);
                for (int i = 0; i < n; ++i)
                    env.run_learning_step();
                response["ok"] = true;
                response["result"] = serialize_state(env);
            } else if (command == "state") {
                response["ok"] = true;
                response["result"] = serialize_state(env);
            } else if (command == "reset") {
                env.reset();
                response["ok"] = true;
                response["result"] = serialize_state(env);
            } else if (command == "load") {
                std::string path = cmd.value("path", brain_file_path());
                bool any = false;
                for (auto& agent : env.get_agents()) {
                    try {
                        agent.load_brain_from_file(path);
                        any = true;
                    } catch (const std::exception& e) {
                        response = json{{"ok", false},
                                        {"error", "Load failed: " + std::string(e.what())}};
                    }
                }
                if (any) {
                    response["ok"] = true;
                    response["result"] = "Brain loaded from " + path;
                }
            } else if (command == "save") {
                std::string path = cmd.value("path", brain_file_path());
                bool any = false;
                for (auto& agent : env.get_agents()) {
                    if (agent.is_alive()) {
                        try {
                            agent.save_brain_to_file(path);
                            any = true;
                            break;
                        } catch (const std::exception& e) {
                            response = json{{"ok", false},
                                            {"error", "Save failed: " + std::string(e.what())}};
                        }
                    }
                }
                if (any) {
                    response["ok"] = true;
                    response["result"] = "Brain saved to " + path;
                }
            } else if (command == "param") {
                std::string key = cmd.value("key", "");
                auto& cfg = RuntimeConfig::instance();
                if (key == "grid_size") {
                    cfg.grid_size = cmd["value"].get<int>();
                } else if (key == "food_count") {
                    cfg.food_count = cmd["value"].get<int>();
                } else if (key == "agent_count") {
                    cfg.agent_count = cmd["value"].get<int>();
                } else if (key == "food_energy") {
                    cfg.food_energy = cmd["value"].get<int>();
                } else if (key == "food_reset_threshold") {
                    cfg.food_reset_threshold = cmd["value"].get<int>();
                } else {
                    response = json{{"ok", false},
                                    {"error", "Unknown param: " + key}};
                }
                if (!response.contains("error"))
                    response["result"] = "param set";
            } else {
                response = json{{"ok", false},
                                {"error", "Unknown command: " + command}};
            }
        } catch (const json::parse_error& e) {
            response = json{{"ok", false},
                            {"error", "JSON parse error: " + std::string(e.what())}};
        } catch (const std::exception& e) {
            response = json{{"ok", false},
                            {"error", std::string(e.what())}};
        }

        std::cout << response.dump() << "\n" << std::flush;
    }
}
