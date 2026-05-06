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
      total_food_eaten_this_episode(0),
      avg_food_per_episode(0),
      episode_count_for_avg(0) {
    
    // Create agents
    agents.resize(AGENT_COUNT);
    
    // Spawn initial food
    spawn_food();
    
    // Initialize first episode stats
    first_episode_food_stats.resize(AGENT_COUNT, 0);
}

// ============================================================================
// Environment: Food Management
// ============================================================================

void Environment::spawn_food() {
    food_list.clear();
    
    // Random distribution for food energy
    std::uniform_int_distribution<int> energy_dist(FOOD_ENERGY / 2, FOOD_ENERGY + FOOD_ENERGY / 2);
    
    for (int i = 0; i < FOOD_COUNT; ++i) {
        Food f;
        f.pos.x = env_gen() % GRID_SIZE;
        f.pos.y = env_gen() % GRID_SIZE;
        f.energy_value = energy_dist(env_gen);
        food_list.push_back(f);
        total_food_spawned++;
    }
}

// ============================================================================
// Environment: Genetic Algorithm
// ============================================================================

std::vector<AI> Environment::select_parents(int num_parents) {
    // Sort agents by performance (food eaten)
    std::vector<AI> sorted_agents = agents;
    std::sort(sorted_agents.begin(), sorted_agents.end(), 
              [](const AI& a, const AI& b) {
                  return a.total_food_eaten > b.total_food_eaten;
              });
    
    // Return top performers
    return std::vector<AI>(sorted_agents.begin(), sorted_agents.begin() + num_parents);
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
    std::uniform_real_distribution<double> dist(-0.1, 0.1);
    
    for (auto& gene : genome) {
        if (dis(env_gen) < mutation_rate) {
            gene += dist(env_gen);  // Small random change
        }
    }
}

// ============================================================================
// Environment: Reset and Evolution
// ============================================================================

void Environment::reset() {
    // Update average food per episode
    if (episode_count_for_avg > 0) {
        avg_food_per_episode = (avg_food_per_episode * (episode_count_for_avg - 1) + total_food_eaten_this_episode) / episode_count_for_avg;
    } else {
        avg_food_per_episode = total_food_eaten_this_episode;
    }
    episode_count_for_avg++;
    total_food_eaten_this_episode = 0;
    
    episode++;
    
    // Reset agent state (keep their brains for DQN learning)
    for (auto& agent : agents) {
        agent.energy = MAX_ENERGY;
        agent.pos.x = env_gen() % GRID_SIZE;
        agent.pos.y = env_gen() % GRID_SIZE;
        agent.total_food_eaten = 0;
    }
    
    // ALWAYS preserve best brain: load brain_best.json if it exists
    {
        std::ifstream best_file("brain_best.json");
        if (best_file.good()) {
            try {
                agents[0].load_brain_from_file("brain_best.json");
            } catch (...) { /* ignore */ }
        }
    }
    
    // Respawn food
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
                    reward += 20.0;  // Big reward for eating
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
            reward += (prev_distance - next_distance) * 0.5;  // Stronger signal
            if (next_closest_food.has_value() && next_distance > prev_distance) {
                reward -= 0.5;  // Stronger penalty for moving away
            }
            
            // Small penalty per step (encourages efficiency)
            reward -= 0.05;
            
            // Penalty for bumping into walls (stronger)
            if (prev_pos == agent.pos) {
                reward -= 1.0;
            }
            
            // Penalty for dying (should never happen with new prevention)
            if (agent.energy <= 0) {
                reward -= 20.0;
                agent.energy = 1;  // Respawn energy
                agent_done = false;  // Never actually done
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
    if (food_list.empty() || food_list.size() < FOOD_COUNT / 2) {
        spawn_food();
    }
    
    // === Check if all agents are dead ===
    bool all_dead = true;
    for (const auto& agent : agents) {
        if (agent.is_alive()) {
            all_dead = false;
            break;
        }
    }
    
    if (all_dead) {
        // Respawn agents with best brain, but keep food the same
        for (auto& agent : agents) {
            agent.energy = MAX_ENERGY;
            agent.pos.x = env_gen() % GRID_SIZE;
            agent.pos.y = env_gen() % GRID_SIZE;
            agent.total_food_eaten = 0;
        }
        // Load best brain into first agent
        {
            std::ifstream best_file("brain_best.json");
            if (best_file.good()) {
                try {
                    agents[0].load_brain_from_file("brain_best.json");
                } catch (...) { /* ignore */ }
            }
        }
        episode++;
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
    return static_cast<int>(total_food_eaten_this_episode);
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
