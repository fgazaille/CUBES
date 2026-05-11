/**
 * @file ai_agent.hpp
 * @brief AI agent that uses a neural network for reinforcement learning.
 * 
 * Each agent:
 * - Has a neural network (policy network) for Q-value approximation
 * - Has a target network for stable DQN training
 * - Uses epsilon-greedy exploration
 * - Maintains an experience replay buffer
 * - Can save/load its brain to/from JSON files
 * - Has a unique color for visualization
 * 
 * The agent learns to navigate a grid world, collect food,
 * and avoid running out of energy.
 */

#pragma once

#include "config.hpp"
#include "neural_network.hpp"
#include "experience.hpp"
#include <vector>
#include <deque>
#include <memory>
#include <random>
#include <optional>
#include "raylib.h"

/**
 * @brief Possible actions an agent can take.
 */
enum Action { LEFT, RIGHT, UP, DOWN };

/**
 * @brief 2D position on the grid.
 */
struct Position {
    int x, y;
    
    /**
     * @brief Check if two positions are the same.
     * @param other Position to compare with
     * @return true if positions are equal
     */
    [[nodiscard]] constexpr bool operator==(const Position& other) const noexcept {
        return x == other.x && y == other.y;
    }
    
    /**
     * @brief Calculate Euclidean distance to another position.
     * @param other Target position
     * @return Distance between positions
     */
    [[nodiscard]] double distance(const Position& other) const;
};

/**
 * @brief Food item on the grid.
 */
struct Food {
    Position pos;         ///< Position of the food
    int energy_value;      ///< Energy restored when consumed
};

/**
 * @brief AI agent using Deep Q-Network (DQN) for learning.
 * 
 * Implements a DQN agent with:
 * - Experience replay buffer
 * - Target network for stable training
 * - Epsilon-greedy exploration with decay
 * - Neural network for Q-value approximation
 * - Genetic operations (mutation) for evolution
 */
class AI {
private:
    double explore_rate;
    std::unique_ptr<NeuralNetwork> neural_network;
    std::unique_ptr<NeuralNetwork> target_network;
    std::deque<Experience> experience_buffer;
    int update_counter = 0;
    std::vector<double> last_q_values;
    std::vector<Action> last_actions;
    std::mt19937 gen;
    std::uniform_real_distribution<double> dis;
    int train_step_counter = 0;

    // Pre-allocated buffers for no-alloc forward/training
    std::vector<double> state_buffer_;
    std::vector<double> q_buffer_;
    std::vector<double> target_buffer_;

public:
    // State variables (public for easy access)
    Position pos;                    ///< Current position on grid
    int energy;                      ///< Current energy level
    int total_food_eaten;            ///< Total food consumed in current episode
    Color color;                     ///< Unique color for visualization
    int brain_learning_rate;         ///< Visualization parameter
    std::vector<double> synaptic_strength;  ///< Visualization parameter
    std::vector<int> layer_sizes;    ///< Neural network architecture

    /**
     * @brief Construct a new AI agent with random initialization.
     */
    AI();
    
    /**
     * @brief Construct AI agent with specific genome.
     * @param genome The neural network genome to use
     */
    AI(const std::vector<double>& genome);

    /**
     * @brief Copy constructor (deep copy of neural networks).
     * @param other Agent to copy from
     */
    AI(const AI& other);

    /**
     * @brief Copy assignment operator (deep copy of neural networks).
     * @param other Agent to copy from
     * @return Reference to this agent
     */
    AI& operator=(const AI& other);

    // ========================================================================
    // Brain serialization
    // ========================================================================

    /**
     * @brief Serialize neural network weights to JSON string.
     * @return JSON string containing the genome
     */
    std::string save_brain_to_json() const;

    /**
     * @brief Load neural network weights from JSON string.
     * @param json_str JSON string containing the genome
     */
    void load_brain_from_json(const std::string& json_str);

    /**
     * @brief Save brain to a JSON file.
     * @param filename Path to output file
     */
    void save_brain_to_file(const std::string& filename) const;

    /**
     * @brief Load brain from a JSON file.
     * @param filename Path to input file
     */
    void load_brain_from_file(const std::string& filename);

    // ========================================================================
    // Decision making
    // ========================================================================

    /**
     * @brief Convert current world state to neural network input (no-alloc version).
     */
    void get_state_representation(const std::vector<Food>& food_list, std::vector<double>& state) const;

    [[nodiscard]] std::vector<double> get_state_representation(const std::vector<Food>& food_list) const {
        std::vector<double> state(INPUT_SIZE);
        get_state_representation(food_list, state);
        return state;
    }

    /**
     * @brief Choose an action using epsilon-greedy policy.
     */
    [[nodiscard]] int choose_action(const std::vector<Food>& food_list, const std::vector<double>* precomputed_state = nullptr);

    /**
     * @brief Find the closest food item to the agent.
     * @param food_list List of food items
     * @return Position of closest food, or std::nullopt if none exist
     */
    [[nodiscard]] std::optional<Position> find_closest_food(const std::vector<Food>& food_list) const;

    // ========================================================================
    // Environment interaction
    // ========================================================================

    /**
     * @brief Respawn the agent after death.
     * 
     * Resets position, energy, and food count.
     * Preserves neural networks and experience buffer.
     */
    void respawn();

    /**
     * @brief Move the agent in the specified direction.
     * 
     * Handles boundary checking and energy consumption.
     * 
     * @param action Direction to move (LEFT, RIGHT, UP, DOWN)
     */
    void move(int action);

    /**
     * @brief Store an experience in the replay buffer.
     * @param state State before action
     * @param action Action taken
     * @param reward Reward received
     * @param next_state State after action
     * @param done Whether the episode terminated
     */
    void add_experience(const std::vector<double>& state, int action, double reward,
                       const std::vector<double>& next_state, bool done);

    /**
     * @brief Sample from replay buffer and train the neural network.
     * 
     * Uses DQN algorithm:
     * 1. Sample a mini-batch from experience buffer
     * 2. Compute target Q-values using target network
     * 3. Train policy network using MSE loss
     * 4. Periodically update target network
     */
    void learn_from_experience();

    // ========================================================================
    // State queries
    // ========================================================================

    /**
     * @brief Check if the agent is still alive.
     * @return true if energy > 0
     */
    bool is_alive() const;

    /**
     * @brief Decay the exploration rate.
     * 
     * Multiplies explore_rate by EXPLORE_DECAY, bounded by MIN_EXPLORE_RATE.
     */
    void decay_exploration();

    /**
     * @brief Get current exploration rate.
     * @return Exploration rate (epsilon)
     */
    double get_explore_rate() const;

    /**
     * @brief Get energy as a percentage of maximum.
     * @return Energy percentage (0.0 to 1.0)
     */
    float get_energy_percentage() const;

    // ========================================================================
    // Evolution / Genetic operations
    // ========================================================================

    /**
     * @brief Apply random mutations to the neural network.
     * @param mutation_rate Probability of mutating each parameter
     */
    void mutate(double mutation_rate);

    // ========================================================================
    // Visualization helpers
    // ========================================================================

    /**
     * @brief Get pointer to the neural network (for visualization).
     * @return Raw pointer to policy network
     */
    NeuralNetwork* get_neural_network() const;

    /**
     * @brief Get activation value of a specific neuron.
     * @param layer_index Index of the layer (0-based)
     * @param neuron_index Index of the neuron within the layer
     * @return Last activation value of the neuron
     */
    double get_layer_output(int layer_index, int neuron_index) const;
    
    /**
     * @brief Get weight between two neurons.
     * @param layer_index Index of the source layer
     * @param neuron_index Index of the source neuron
     * @param next_neuron_index Index of the target neuron
     * @return Weight value
     */
    double get_weight(int layer_index, int neuron_index, int next_neuron_index) const;
    
    /**
     * @brief Get the last computed Q-values.
     * @return Vector of Q-values for each action
     */
    [[nodiscard]] std::vector<double> get_last_q_values() const;
    
    /**
     * @brief Get current state representation.
     * @param food_list Current food positions
     * @return State vector
     */
    [[nodiscard]] std::vector<double> get_state_for_debug(const std::vector<Food>& food_list) const;
};
