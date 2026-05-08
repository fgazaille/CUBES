/**
 * @file environment.cpp
 * @brief Implementation of the simulation environment.
 * 
 * Manages the full simulation lifecycle including:
 * - Agent creation and evolution through genetic algorithms
 * - Food spawning and consumption
 * - Parallel agent processing using thread pool
 * - Experience collection and DQN learning
 */

#include "environment.hpp"
#include <algorithm>
#include <random>
#include <iostream>
#include <fstream>

// ============================================================================
// Environment: Construction
// ============================================================================

Environment::Environment(bool) 
    : episode(1), 
      total_food_spawned(0), simulation_running(true), 
      thread_pool(NUM_THREADS),
      env_gen(std::random_device{}()),
      avg_food_per_episode(0),
      episode_count_for_avg(0) {
    
    RuntimeConfig& cfg = RuntimeConfig::instance();
    
    // Create agents
    agents.resize(cfg.agent_count);
    
    // Initialize respawn counters (0 = alive, no respawn pending)
    respawn_counters.resize(cfg.agent_count, 0);
    
    // Spawn initial food
    spawn_food();
    
    // Initialize first episode stats
    first_episode_food_stats.resize(cfg.agent_count, 0);
}

// ============================================================================
// Environment: Food Management
// ============================================================================

void Environment::spawn_food() {
    RuntimeConfig& cfg = RuntimeConfig::instance();
    
    std::uniform_int_distribution<int> energy_dist(cfg.food_energy / 2, cfg.food_energy + cfg.food_energy / 2);
    
    int to_spawn = cfg.food_count - static_cast<int>(food_list.size());
    if (to_spawn <= 0) return;
    
    for (int i = 0; i < to_spawn; ++i) {
        Food f;
        f.pos.x = env_gen() % cfg.grid_size;
        f.pos.y = env_gen() % cfg.grid_size;
        f.energy_value = energy_dist(env_gen);
        food_list.push_back(f);
        total_food_spawned++;
    }
}

void Environment::check_food_reset() {
    RuntimeConfig& cfg = RuntimeConfig::instance();
    if (static_cast<int>(food_list.size()) < cfg.food_reset_threshold) {
        spawn_food();
    }
}

// ============================================================================
// Environment: Genetic Algorithm
// ============================================================================

std::vector<AI> Environment::select_parents(int num_parents) {
    // Tournament selection: pick random agents, the best one becomes a parent
    // This maintains diversity better than truncation selection
    std::vector<AI> selected;
    std::uniform_int_distribution<size_t> dist(0, agents.size() - 1);
    
    for (int p = 0; p < num_parents; ++p) {
        size_t best_idx = dist(env_gen);
        int best_food = agents[best_idx].total_food_eaten;
        
        // Tournament of 3 random agents
        for (int t = 0; t < 3; ++t) {
            size_t candidate = dist(env_gen);
            if (agents[candidate].total_food_eaten > best_food) {
                best_food = agents[candidate].total_food_eaten;
                best_idx = candidate;
            }
        }
        
        selected.push_back(agents[best_idx]);
    }
    
    return selected;
}

std::vector<double> Environment::crossover(const std::vector<double>& parent1, 
                                           const std::vector<double>& parent2) {
    // Uniform crossover: each gene has 50% chance from either parent
    std::vector<double> offspring(parent1.size());
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    
    for (size_t i = 0; i < parent1.size(); ++i) {
        offspring[i] = (dis(env_gen) < 0.5) ? parent1[i] : parent2[i];
    }
    
    return offspring;
}

void Environment::mutate_genome(std::vector<double>& genome, double mutation_rate) {
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    std::uniform_real_distribution<double> dist(-0.2, 0.2);
    
    for (auto& gene : genome) {
        if (dis(env_gen) < mutation_rate) {
            gene += dist(env_gen);
        }
    }
}

// ============================================================================
// Environment: Reset and Evolution
// ============================================================================

void Environment::reset() {
    // Update average food per episode
    int food_this_episode = total_food_eaten_this_episode.load();
    if (episode_count_for_avg > 0) {
        avg_food_per_episode = (avg_food_per_episode * (episode_count_for_avg - 1) + food_this_episode) / episode_count_for_avg;
    } else {
        avg_food_per_episode = food_this_episode;
    }
    episode_count_for_avg++;
    total_food_eaten_this_episode.store(0);
    
    episode++;
    
    // Find the best genome (highest food eaten) to use as template
    std::vector<double> template_genome;
    int best_food = -1;
    for (const auto& agent : agents) {
        if (agent.total_food_eaten > best_food) {
            best_food = agent.total_food_eaten;
            template_genome = agent.get_neural_network()->get_genome();
        }
    }
    
    std::vector<AI> new_agents;
    for (size_t i = 0; i < agents.size(); ++i) {
        std::vector<double> child_genome;
        if (!template_genome.empty()) {
            child_genome = template_genome;
            mutate_genome(child_genome, 0.02);
        }
        AI new_agent = child_genome.empty() ? AI() : AI(child_genome);
        new_agent.energy = MAX_ENERGY;
        new_agent.pos.x = env_gen() % GRID_SIZE;
        new_agent.pos.y = env_gen() % GRID_SIZE;
        new_agent.total_food_eaten = 0;
        new_agents.push_back(std::move(new_agent));
    }
    
    agents = std::move(new_agents);
    
    // Reset respawn counters
    respawn_counters.assign(agents.size(), 0);
    
    // Respawn food (fill up to FOOD_COUNT)
    spawn_food();
}

// ============================================================================
// Environment: Simulation Step
// ============================================================================

void Environment::run_learning_step() {
    // Collect food consumption results from parallel agent processing
    std::vector<std::pair<int, int>> food_consumed;  // agent_idx, food_idx
    std::mutex consumed_mutex;
    
    // Process each agent in parallel using thread pool
    for (size_t idx = 0; idx < agents.size(); ++idx) {
        if (!agents[idx].is_alive()) continue;
        
        thread_pool.detach_task([this, idx, &food_consumed, &consumed_mutex]() {
            auto& agent = agents[idx];
            
            // Use a local food snapshot to compute current and next states consistently.
            std::vector<Food> local_food = food_list;
            
            // === Get current state once ===
            std::vector<double> current_state = agent.get_state_representation(local_food);
            auto prev_closest_food = agent.find_closest_food(local_food);
            double prev_distance = prev_closest_food.has_value() ? agent.pos.distance(prev_closest_food.value()) : 0.0;
            
            // === Choose and execute action ===
            int action = agent.choose_action(local_food, &current_state);
            Position prev_pos = agent.pos;
            agent.move(action);
            
            // === Calculate reward ===
            double reward = 0.0;
            bool agent_done = false;
            int consumed_index = -1;
            
            // Check for food consumption
            for (size_t k = 0; k < local_food.size(); ++k) {
                if (agent.pos.x == local_food[k].pos.x && agent.pos.y == local_food[k].pos.y) {
                    int food_value = local_food[k].energy_value;
                    reward += 5.0 + 0.1 * food_value;  // Scaled reward for eating
                    agent.energy += food_value;
                    if (agent.energy > MAX_ENERGY) agent.energy = MAX_ENERGY;
                    agent.total_food_eaten++;
                    total_food_eaten_this_episode++;
                    consumed_index = static_cast<int>(k);
                    {
                        std::lock_guard<std::mutex> lock2(consumed_mutex);
                        food_consumed.push_back({static_cast<int>(idx), static_cast<int>(k)});
                    }
                    break;
                }
            }
            
            if (consumed_index >= 0) {
                local_food.erase(local_food.begin() + consumed_index);
            }
            
            // Reward moving toward food and penalize moving away.
            auto next_closest_food = agent.find_closest_food(local_food);
            double next_distance = next_closest_food.has_value() ? agent.pos.distance(next_closest_food.value()) : prev_distance;
            double distance_change = prev_distance - next_distance;
            reward += distance_change * 0.3;
            
            // Small penalty per step (encourages efficiency)
            reward -= 0.1;
            
            // Penalty for bumping into walls (teaches boundary awareness)
            if (prev_pos == agent.pos) {
                reward -= 0.5;
            }
            
            // Handle death when energy depleted
            if (agent.energy <= 0) {
                agent.energy = 0;
                reward -= 5.0;
                agent_done = true;
            }
            
            // === Store experience ===
            std::vector<double> next_state = agent.get_state_representation(local_food);
            agent.add_experience(current_state, action, reward, next_state, agent_done);
            
            // === Learn from experiences (train every step) ===
            agent.learn_from_experience();
            
            // === Decay exploration ===
            agent.decay_exploration();
        });
    }
    
    // Wait for all submitted tasks to complete
    thread_pool.wait();
    
    // Remove consumed food in reverse order
    std::sort(food_consumed.rbegin(), food_consumed.rend());
    {
        std::lock_guard<std::mutex> lock(food_mutex);
        for (const auto& consumed : food_consumed) {
            if (consumed.second >= 0 && consumed.second < static_cast<int>(food_list.size())) {
                food_list.erase(food_list.begin() + consumed.second);
            }
        }
    }
    
    // === Replenish food if needed ===
    check_food_reset();
    
    // === Agent respawning ===
    // Find which agents are dead and pick a template genome
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
    // If no dead agent was found (all alive), use first live agent's genome as fallback
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
                new_agent.pos.x = env_gen() % GRID_SIZE;
                new_agent.pos.y = env_gen() % GRID_SIZE;
                new_agent.total_food_eaten = 0;
                agents[i] = std::move(new_agent);
            }
        }
    }
}

// ============================================================================
// Environment: Query Methods
// ============================================================================

bool Environment::is_running() const { 
    return simulation_running.load(); 
}

int Environment::get_alive_count() const {
    int count = 0;
    for (const auto& agent : agents) {
        if (agent.is_alive()) count++;
    }
    return count;
}

int Environment::get_episode() const { 
    return episode; 
}

std::vector<AI>& Environment::get_agents() {
    return agents;
}

int Environment::get_total_food_spawned() const { 
    return total_food_spawned; 
}

double Environment::get_avg_food_per_episode() const {
    return avg_food_per_episode;
}

int Environment::get_current_episode_food() const {
    return total_food_eaten_this_episode.load();
}

double Environment::get_avg_exploration_rate() const {
    if (agents.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& agent : agents) {
        sum += agent.get_explore_rate();
    }
    return sum / agents.size();
}

const std::vector<AI>& Environment::get_agents() const { 
    return agents; 
}

const std::vector<Food>& Environment::get_food_list() const { 
    return food_list; 
}
