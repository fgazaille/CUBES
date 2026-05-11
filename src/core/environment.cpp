#include "environment.hpp"
#include <algorithm>
#include <random>
#include <iostream>
#include <fstream>
#include <mutex>
#include <set>

Environment::Environment(bool)
    : episode(1),
      total_food_spawned(0), simulation_running(true),
      env_gen(std::random_device{}()),
      avg_food_per_episode(0),
      episode_count_for_avg(0) {

    RuntimeConfig& cfg = RuntimeConfig::instance();
    agents.resize(cfg.agent_count);
    respawn_counters.resize(cfg.agent_count, 0);
    spawn_food();
    first_episode_food_stats.resize(cfg.agent_count, 0);
}

void Environment::spawn_food() {
    RuntimeConfig& cfg = RuntimeConfig::instance();
    std::uniform_int_distribution<int> energy_dist(cfg.food_energy / 2, cfg.food_energy + cfg.food_energy / 2);

    int to_spawn = cfg.food_count - static_cast<int>(food_list.size());
    if (to_spawn <= 0) return;

    for (int i = 0; i < to_spawn; ++i) {
        Food f;
        do {
            f.pos.x = env_gen() % cfg.grid_size;
            f.pos.y = env_gen() % cfg.grid_size;
        } while (is_wall(f.pos.x, f.pos.y));
        f.energy_value = energy_dist(env_gen);
        food_list.push_back(f);
        total_food_spawned++;
    }
}

void Environment::check_food_reset() {
    RuntimeConfig& cfg = RuntimeConfig::instance();
    if (static_cast<int>(food_list.size()) < cfg.food_reset_threshold)
        spawn_food();
}

void Environment::mutate_genome(std::vector<double>& genome, double mutation_rate) {
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    std::uniform_real_distribution<double> dist(-0.2, 0.2);
    for (auto& gene : genome) {
        if (dis(env_gen) < mutation_rate)
            gene += dist(env_gen);
    }
}

void Environment::reset() {
    RuntimeConfig& cfg = RuntimeConfig::instance();

    steps_since_last_reset_ = 0;
    stagnation_baseline_ = total_food_eaten_all_time.load();

    int best_idx = 0;
    last_gen_best_food = 0;
    for (size_t i = 0; i < agents.size(); ++i) {
        if (agents[i].total_food_eaten > last_gen_best_food) {
            last_gen_best_food = agents[i].total_food_eaten;
            best_idx = i;
        }
    }
    if (!agents.empty()) {
        if (last_gen_best_food > last_saved_best_food_) {
            last_saved_best_food_ = last_gen_best_food;
            try {
                agents[best_idx].save_brain_to_file(brain_file_path());
            } catch (const std::exception& e) {
                std::cerr << "Auto-save brain failed: " << e.what() << "\n";
            }
        }
    }

    int food_this_episode = total_food_eaten_this_episode.load();
    if (episode_count_for_avg > 0)
        avg_food_per_episode = (avg_food_per_episode * (episode_count_for_avg - 1) + food_this_episode) / episode_count_for_avg;
    else
        avg_food_per_episode = food_this_episode;
    episode_count_for_avg++;
    total_food_eaten_this_episode.store(0);

    episode++;

    std::vector<AI> new_agents;
    for (int i = 0; i < cfg.agent_count; ++i) {
        std::vector<double> child_genome;
        if (!agents.empty()) {
            size_t src_idx = i % agents.size();
            std::vector<double> template_genome = agents[src_idx].get_neural_network()->get_genome();
            if (!template_genome.empty()) {
                child_genome = template_genome;
                mutate_genome(child_genome, 0.02);
            }
        }
        AI new_agent = child_genome.empty() ? AI() : AI(child_genome);
        new_agent.energy = MAX_ENERGY;
        do {
            new_agent.pos.x = 1 + (env_gen() % std::max(1, cfg.grid_size - 2));
            new_agent.pos.y = 1 + (env_gen() % std::max(1, cfg.grid_size - 2));
        } while (is_wall(new_agent.pos.x, new_agent.pos.y));
        new_agent.total_food_eaten = 0;
        new_agents.push_back(std::move(new_agent));
    }

    agents = std::move(new_agents);
    respawn_counters.assign(agents.size(), 0);
    spawn_food();
}

void Environment::run_learning_step() {
    std::vector<std::pair<int, int>> food_consumed;
    std::mutex consumed_mutex;
    RuntimeConfig& cfg = RuntimeConfig::instance();
    steps_since_last_reset_++;

    for (size_t idx = 0; idx < agents.size(); ++idx) {
        if (!agents[idx].is_alive()) continue;

        pool.detach_task([this, idx, &food_consumed, &consumed_mutex]() {
            auto& agent = agents[idx];
            std::vector<Food> local_food = food_list;

            std::vector<double> current_state = agent.get_state_representation(local_food);
            auto prev_closest_food = agent.find_closest_food(local_food);
            double prev_distance = prev_closest_food.has_value() ? agent.pos.distance(prev_closest_food.value()) : 0.0;

            int action = agent.choose_action(local_food, &current_state);
            Position prev_pos = agent.pos;
            agent.move(action);

            double reward = 0.0;
            bool agent_done = false;
            int consumed_index = -1;

            for (size_t k = 0; k < local_food.size(); ++k) {
                if (agent.pos.x == local_food[k].pos.x && agent.pos.y == local_food[k].pos.y) {
                    int food_value = local_food[k].energy_value;
                    reward += 5.0 + 0.1 * food_value;
                    agent.energy += food_value;
                    if (agent.energy > MAX_ENERGY) agent.energy = MAX_ENERGY;
                    agent.total_food_eaten++;
                    total_food_eaten_this_episode++;
                    total_food_eaten_all_time++;
                    consumed_index = static_cast<int>(k);
                    {
                        std::lock_guard<std::mutex> lock2(consumed_mutex);
                        food_consumed.push_back({static_cast<int>(idx), static_cast<int>(k)});
                    }
                    break;
                }
            }

            // Track best-food-ever from this agent
            {
                int ate = agent.total_food_eaten;
                int cur = best_food_ever.load();
                while (ate > cur && !best_food_ever.compare_exchange_weak(cur, ate));
            }

            if (consumed_index >= 0)
                local_food.erase(local_food.begin() + consumed_index);

            auto next_closest_food = agent.find_closest_food(local_food);
            double next_distance = next_closest_food.has_value() ? agent.pos.distance(next_closest_food.value()) : prev_distance;
            double distance_change = prev_distance - next_distance;
            reward += distance_change * 0.3;

            reward -= 0.1;
            if (prev_pos == agent.pos) reward -= 0.5;

            if (agent.energy <= 0) {
                agent.energy = 0;
                reward -= 5.0;
                agent_done = true;
            }

            std::vector<double> next_state = agent.get_state_representation(local_food);
            agent.add_experience(current_state, action, reward, next_state, agent_done);
            agent.learn_from_experience();
            agent.decay_exploration();
            
            if (agent_done) {
                {
                    std::lock_guard<std::mutex> lock(consumed_mutex);
                    std::cout << "Agent " << idx << " died! Respawning..." << std::endl;
                }
                agent.respawn();
            }
        });
    }

    pool.wait();

    // Fix C2: deduplicate food indices before removal
    {
        std::set<int, std::greater<int>> consumed_set;
        for (const auto& c : food_consumed)
            consumed_set.insert(c.second);
        for (int idx : consumed_set) {
            if (idx >= 0 && idx < static_cast<int>(food_list.size()))
                food_list.erase(food_list.begin() + idx);
        }
    }

    check_food_reset();

    // Agent respawning
    int alive_count = 0;
    std::vector<double> template_genome;
    bool found_template = false;
    for (const auto& agent : agents) {
        if (agent.is_alive()) alive_count++;
        if (!agent.is_alive() && !found_template) {
            template_genome = agent.get_neural_network()->get_genome();
            found_template = true;
        }
    }
    if (!found_template) {
        for (const auto& agent : agents) {
            if (agent.is_alive()) {
                template_genome = agent.get_neural_network()->get_genome();
                break;
            }
        }
    }

    if (!template_genome.empty()) {
        for (size_t i = 0; i < agents.size(); ++i) {
            if (!agents[i].is_alive()) {
                std::vector<double> child_genome = template_genome;
                mutate_genome(child_genome, 0.02);
                AI new_agent(child_genome);
                new_agent.energy = MAX_ENERGY;
                do {
                    new_agent.pos.x = 1 + (env_gen() % std::max(1, cfg.grid_size - 2));
                    new_agent.pos.y = 1 + (env_gen() % std::max(1, cfg.grid_size - 2));
                } while (is_wall(new_agent.pos.x, new_agent.pos.y));
                new_agent.total_food_eaten = 0;
                agents[i] = std::move(new_agent);
            }
        }
    }

    // Save brain when best_food_ever improves significantly (≥5 new food, not every step)
    {
        int cur_best = best_food_ever.load();
        if (cur_best >= last_saved_best_food_ + 5) {
            last_saved_best_food_ = cur_best;
            int best_idx = 0;
            for (size_t i = 0; i < agents.size(); ++i)
                if (agents[i].total_food_eaten > agents[best_idx].total_food_eaten)
                    best_idx = i;
            try {
                agents[best_idx].save_brain_to_file(brain_file_path());
                std::cout << "Brain saved: " << cur_best << " food\n";
            } catch (const std::exception& e) {
                std::cerr << "Auto-save error: " << e.what() << "\n";
            }
        }
    }

    // Reset stagnation counter if any food has been eaten since last check
    int current_total = total_food_eaten_all_time.load();
    if (current_total > stagnation_baseline_) {
        steps_since_last_reset_ = 0;
        stagnation_baseline_ = current_total;
    }

    // Generation reset when all dead OR stagnation (3000 steps without progress)
    bool all_dead = (alive_count == 0);
    bool stagnant = (steps_since_last_reset_ >= 3000);
    if (all_dead || stagnant) reset();
}

bool Environment::is_running() const { return simulation_running.load(); }

int Environment::get_alive_count() const {
    int count = 0;
    for (const auto& agent : agents) {
        if (agent.is_alive()) count++;
    }
    return count;
}

int Environment::get_episode() const { return episode; }

bool Environment::is_wall(int x, int y) const {
    RuntimeConfig& cfg = RuntimeConfig::instance();
    return x == 0 || y == 0 || x == cfg.grid_size - 1 || y == cfg.grid_size - 1;
}

std::vector<AI>& Environment::get_agents() { return agents; }

int Environment::get_total_food_spawned() const { return total_food_spawned; }

double Environment::get_avg_food_per_episode() const {
    return avg_food_per_episode;
}

int Environment::get_last_gen_best_food() const {
    return last_gen_best_food;
}

int Environment::get_current_episode_food() const {
    return total_food_eaten_this_episode.load();
}

double Environment::get_avg_exploration_rate() const {
    if (agents.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& agent : agents)
        sum += agent.get_explore_rate();
    return sum / agents.size();
}

const std::vector<AI>& Environment::get_agents() const { return agents; }

int Environment::get_best_food_ever() const {
    return best_food_ever.load();
}

const std::vector<Food>& Environment::get_food_list() const { return food_list; }
