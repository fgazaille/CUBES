/**
 * @file environment.hpp"
 * @brief Simulation environment managing agents, food, and episodes."
 * "
 * The Environment:
 * - Manages multiple AI agents
 * - Spawns and tracks food items
 * - Runs simulation steps (agent decisions, moves, rewards)
 * - Handles episode resets and evolution (selection, crossover, mutation)
 *
 * The simulation follows an episodic structure where agents collect food
 * to maintain energy. When all agents die, a new generation is created
 * through genetic algorithms.
 */

#pragma once

#include "ai_agent.hpp"
#include "config.hpp"
#include "BS_thread_pool.hpp"
#include <vector>
#include <atomic>
#include <mutex>

/**
 * @brief Simulation environment for AI agents.
 * 
 * Manages the full lifecycle of the simulation including:
 * - Agent creation and evolution
 * - Food spawning and consumption
 * - Experience collection and learning
 * - Episode management and reset
 */
class Environment {
private:
    std::vector<AI> agents;               ///< AI agents in the simulation
    std::vector<Food> food_list;          ///< Currently available food items
    int episode;                           ///< Current episode number
    int total_food_spawned;               ///< Total food spawned (for stats)
    std::atomic<bool> simulation_running;  ///< Controls simulation thread
    std::vector<int> first_episode_food_stats; ///< Food stats from episode 1
    std::mt19937 env_gen;                 ///< Random generator for environment
    BS::light_thread_pool pool;           ///< Thread pool for parallel agent processing
    
    // Learning metrics
    std::atomic<int> total_food_eaten_this_episode{0}; ///< Track food eaten per episode
    double avg_food_per_episode;          ///< Running average of food per episode
    int episode_count_for_avg;             ///< Count for calculating average
    int last_gen_best_food = 0;            ///< Best food in the most recently completed generation

    // Respawn tracking
    std::vector<int> respawn_counters;     ///< Frames until dead agents respawn (0 = alive or no delay pending)

    // Continuous best-food tracking (for live training progress)
    std::atomic<int> best_food_ever{0};           ///< All-time best food eaten by any single agent
    int last_saved_best_food_ = 0;                ///< Last best_food_ever value that triggered a brain save
    int steps_since_last_reset_ = 0;              ///< Steps since the last generation reset
    int stagnation_baseline_ = 0;                 ///< best_food_ever value at last stagnation check
    std::atomic<int> total_food_eaten_all_time{0}; ///< Total food across all agents and episodes

    /**
     * @brief Apply random mutations to a genome.
     * @param genome Genome to mutate
     * @param mutation_rate Probability of mutating each gene
     */
    void mutate_genome(std::vector<double>& genome, double mutation_rate);

    /**
     * @brief Spawn new food items on the grid.
     * 
     * Spawns just enough food to reach cfg.food_count if below threshold.
     */
    void spawn_food();

public:
    /**
     * @brief Construct a new Environment.
     * 
     * @param enable_display Whether to enable visual display
     */
    Environment(bool enable_display);
    
    /**
     * @brief Get the current agent count from runtime config.
     * @return Agent count
     */
    int get_agent_count() const { return agents.size(); }

    int get_total_food_all_time() const { return total_food_eaten_all_time.load(); }
    
    /**
     * @brief Check and respawn food if below threshold.
     */
    void check_food_reset();

    /**
     * @brief Reset the simulation for a new generation.
     * 
     * Performs:
     * 1. Selection of top agents as parents
     * 2. Crossover and mutation to create offspring
     * 3. Elitism: keep top performers unchanged
     * 4. Reset positions and energy for all agents
     * 5. Respawn food
     */
    void reset();

    /**
     * @brief Execute one learning step for all agents.
     * 
     * For each live agent:
     * 1. Get current state
     * 2. Choose and execute action
     * 3. Calculate reward (food, proximity, penalties)
     * 4. Store experience
     * 5. Learn from experiences
     * 6. Decay exploration rate
     * 
     * Runs in parallel using the thread pool.
     */
    void run_learning_step();

    /**
     * @brief Check if a grid cell is a wall.
     *
     * Border cells (x=0, x=grid_size-1, y=0, y=grid_size-1) are walls.
     * @return true if the cell is a wall
     */
    bool is_wall(int x, int y) const;

    /**
     * @brief Check if simulation is still running.
     * @return true if simulation is active
     */
    bool is_running() const;

    /**
     * @brief Get number of alive agents.
     * @return Count of agents with energy >0
     */
    int get_alive_count() const;

    /**
     * @brief Get current episode number.
     * @return Episode count
     */
    int get_episode() const;

    /**
     * @brief Get total food spawned.
     * @return Total food count
     */
    int get_total_food_spawned() const;
    
    /**
     * @brief Get average food eaten per episode.
     * @return Running average of food per episode
     */
    double get_avg_food_per_episode() const;
    
    /**
     * @brief Get food eaten in current episode.
     * @return Food count for current episode
     */
    int get_current_episode_food() const;
    
    /**
     * @brief Get best food from the most recently completed generation.
     * @return Best food count from last generation
     */
    int get_last_gen_best_food() const;

    /**
     * @brief Get all-time best food eaten by any agent.
     * @return Best food count ever
     */
    int get_best_food_ever() const;

    /**
     * @brief Get average exploration rate across all agents.
     * @return Average epsilon value
     */
    double get_avg_exploration_rate() const;

    /**
     * @brief Get reference to agents vector.
     * @return Reference to agents
     */
    std::vector<AI>& get_agents();

    /**
     * @brief Get reference to agents vector.
     * @return Const reference to agents
     */
    const std::vector<AI>& get_agents() const;

    /**
     * @brief Get reference to food list.
     * @return Const reference to food items
     */
    const std::vector<Food>& get_food_list() const;
};
