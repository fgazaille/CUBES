/**
 * @file environment.hpp
 * @brief Simulation environment managing agents, food, and episodes.
 * 
 * The Environment:
 * - Manages multiple AI agents
 * - Spawns and tracks food items
 * - Runs simulation steps (agent decisions, moves, rewards)
 * - Handles episode resets and evolution (selection, crossover, mutation)
 * - Uses a thread pool for parallel agent processing
 * 
 * The simulation follows an episodic structure where agents collect food
 * to maintain energy. When all agents die, a new generation is created
 * through genetic algorithms.
 */

#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include "ai_agent.hpp"
#include "config.hpp"
#include <vector>
#include <mutex>
#include <atomic>
#include "BS_thread_pool.hpp"

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
    bool display_enabled;                  ///< Whether display is enabled
    int total_food_spawned;               ///< Total food spawned (for stats)
    std::mutex food_mutex;                ///< Protects food_list modifications
    std::mutex agent_mutex;               ///< Protects agent state changes
    std::atomic<bool> simulation_running;  ///< Controls simulation thread
    BS::thread_pool<> thread_pool;        ///< Thread pool for parallel processing
    std::vector<int> first_episode_food_stats; ///< Food stats from episode 1
    std::mt19937 env_gen;                 ///< Random generator for environment
    
    // Learning metrics
    double total_food_eaten_this_episode; ///< Track food eaten per episode
    double avg_food_per_episode;          ///< Running average of food per episode
    int episode_count_for_avg;             ///< Count for calculating average

    /**
     * @brief Select top-performing agents as parents for next generation.
     * 
     * Uses tournament selection based on total food eaten.
     * 
     * @param num_parents Number of parents to select
     * @return Vector of parent agents (sorted by performance)
     */
    std::vector<AI> select_parents(int num_parents);

    /**
     * @brief Perform uniform crossover between two parent genomes.
     * 
     * Each gene has 50% chance of coming from either parent.
     * 
     * @param parent1 First parent's genome
     * @param parent2 Second parent's genome
     * @return Child genome
     */
    std::vector<double> crossover(const std::vector<double>& parent1, 
                                 const std::vector<double>& parent2);

    /**
     * @brief Apply random mutations to a genome.
     * @param genome Genome to mutate
     * @param mutation_rate Probability of mutating each gene
     */
    void mutate_genome(std::vector<double>& genome, double mutation_rate);

    /**
     * @brief Spawn new food items on the grid.
     * 
     * Clears existing food and spawns FOOD_COUNT new items
     * with random positions and energy values.
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

#endif // ENVIRONMENT_HPP
