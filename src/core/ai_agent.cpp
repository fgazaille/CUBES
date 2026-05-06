/**
 * @file ai_agent.cpp
 * @brief Implementation of the AI agent with neural network learning.
 * 
 * Implements a DQN agent with:
 * - Experience replay buffer
 * - Target network for stable training
 * - Epsilon-greedy exploration with decay
 * - JSON serialization for brain saving/loading
 */

#include "ai_agent.hpp"
#include <SDL2/SDL.h>
#include <random>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <json.hpp>

using json = nlohmann::json;

// ============================================================================
// Position Methods
// ============================================================================

bool Position::operator==(const Position& other) const {
    return x == other.x && y == other.y;
}

double Position::distance(const Position& other) const {
    return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
}

// ============================================================================
// AI Agent: Construction and Copying
// ============================================================================

AI::AI() : update_counter(0), brain_learning_rate(0), 
            gen(std::random_device{}()), 
            dis(0.0, 1.0) {
    
    // Random starting position
    pos.x = gen() % GRID_SIZE;
    pos.y = gen() % GRID_SIZE;
    
    // Initial energy at maximum
    energy = MAX_ENERGY;
    explore_rate = INITIAL_EXPLORE_RATE;
    total_food_eaten = 0;
    
    // Generate unique color for visualization
    color.r = 100 + (gen() % 155);
    color.g = 100 + (gen() % 155);
    color.b = 100 + (gen() % 155);
    color.a = 255;
    
    // Define neural network architecture
    layer_sizes = {INPUT_SIZE, HIDDEN_LAYER1_SIZE, HIDDEN_LAYER2_SIZE, OUTPUT_SIZE};
    
    // Create neural networks
    neural_network = std::make_unique<NeuralNetwork>(layer_sizes, gen, dis);
    target_network = std::make_unique<NeuralNetwork>(layer_sizes, gen, dis);
    target_network->copy_weights_from(*neural_network);
    
    // Initialize action history
    last_actions.resize(3, LEFT);
    
    // Initialize visualization parameters
    synaptic_strength.resize(HIDDEN_LAYER1_SIZE, 0.0);
}

AI::AI(const AI& other) 
    : pos(other.pos), energy(other.energy), 
      total_food_eaten(other.total_food_eaten), 
      color(other.color),
      brain_learning_rate(other.brain_learning_rate), 
      synaptic_strength(other.synaptic_strength),
      explore_rate(other.explore_rate), 
      update_counter(other.update_counter),
      last_q_values(other.last_q_values), 
      last_actions(other.last_actions),
      gen(std::random_device{}()),  // New RNG for copy
      dis(0.0, 1.0),
      layer_sizes(other.layer_sizes) {
    
    // Deep copy neural networks
    neural_network = std::make_unique<NeuralNetwork>(*other.neural_network);
    target_network = std::make_unique<NeuralNetwork>(*other.target_network);
}

AI& AI::operator=(const AI& other) {
    if (this != &other) {
        pos = other.pos;
        energy = other.energy;
        total_food_eaten = other.total_food_eaten;
        color = other.color;
        brain_learning_rate = other.brain_learning_rate;
        synaptic_strength = other.synaptic_strength;
        explore_rate = other.explore_rate;
        update_counter = other.update_counter;
        last_q_values = other.last_q_values;
        last_actions = other.last_actions;
        layer_sizes = other.layer_sizes;
        
        // Deep copy neural networks
        neural_network = std::make_unique<NeuralNetwork>(*other.neural_network);
        target_network = std::make_unique<NeuralNetwork>(*other.target_network);
    }
    return *this;
}

// ============================================================================
// Brain Serialization
// ============================================================================

std::string AI::save_brain_to_json() const {
    std::vector<double> genome = neural_network->get_genome();
    
    // Validate genome - replace NaN/Inf with valid values
    bool has_invalid = false;
    for (double& val : genome) {
        if (std::isnan(val) || std::isinf(val)) {
            val = 0.0;  // Reset to zero (or could use random init)
            has_invalid = true;
        }
    }
    
    if (has_invalid) {
        std::cerr << "Warning: Invalid values detected in brain, resetting to zero" << std::endl;
        neural_network->set_genome(genome);
    }
    
    json j;
    j["genome"] = genome;
    return j.dump(4);
}

void AI::load_brain_from_json(const std::string& json_str) {
    try {
        json j = json::parse(json_str);
        
        // Check if genome exists and is not null
        if (j.contains("genome") && !j["genome"].is_null()) {
            std::vector<double> genome = j["genome"].get<std::vector<double>>();
            
            // Validate genome - check for NaN or invalid values
            for (double val : genome) {
                if (std::isnan(val) || std::isinf(val)) {
                    throw std::runtime_error("Invalid value in genome (NaN or Inf)");
                }
            }
            
            neural_network->set_genome(genome);
        } else {
            throw std::runtime_error("Genome is null or missing in JSON");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading brain: " << e.what() << std::endl;
        throw;  // Re-throw so caller knows it failed
    }
}

void AI::save_brain_to_file(const std::string& filename) const {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << save_brain_to_json();
        file.close();
    } else {
        throw std::runtime_error("Could not save to file: " + filename);
    }
}

void AI::load_brain_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        load_brain_from_json(buffer.str());
        file.close();
    } else {
        throw std::runtime_error("Could not open file: " + filename);
    }
}

// ============================================================================
// State Representation
// ============================================================================

std::vector<double> AI::get_state_representation(const std::vector<Food>& food_list) const {
    std::vector<double> state(INPUT_SIZE);
    
    // [0-1]: Normalized position
    state[0] = static_cast<double>(pos.x) / GRID_SIZE;
    state[1] = static_cast<double>(pos.y) / GRID_SIZE;
    
    // [2]: Normalized energy (CRUCIAL for learning energy management)
    state[2] = static_cast<double>(energy) / MAX_ENERGY;
    
    // [3-7]: Closest food information
    Position closest_food = find_closest_food(food_list);
    if (closest_food.x != -1) {
        // [3]: Normalized distance to food
        double distance = pos.distance(closest_food) / std::sqrt(GRID_SIZE * GRID_SIZE * 2);
        state[3] = distance;
        
        // [4-7]: Direction to food (one-hot encoded style)
        state[4] = closest_food.x > pos.x ? (closest_food.x - pos.x) / static_cast<double>(GRID_SIZE) : 0;
        state[5] = closest_food.x < pos.x ? (pos.x - closest_food.x) / static_cast<double>(GRID_SIZE) : 0;
        state[6] = closest_food.y > pos.y ? (closest_food.y - pos.y) / static_cast<double>(GRID_SIZE) : 0;
        state[7] = closest_food.y < pos.y ? (pos.y - closest_food.y) / static_cast<double>(GRID_SIZE) : 0;
    } else {
        // No food: set to defaults
        state[3] = 1.0;  // Maximum distance
        state[4] = state[5] = state[6] = state[7] = 0.0;
    }
    
    // [8-11]: Boundary awareness (distance from each edge)
    state[8] = static_cast<double>(pos.x) / GRID_SIZE;        // Left edge
    state[9] = static_cast<double>(GRID_SIZE - pos.x - 1) / GRID_SIZE;  // Right edge
    state[10] = static_cast<double>(pos.y) / GRID_SIZE;       // Top edge
    state[11] = static_cast<double>(GRID_SIZE - pos.y - 1) / GRID_SIZE; // Bottom edge
    
    return state;
}

// ============================================================================
// Decision Making
// ============================================================================

int AI::choose_action(const std::vector<Food>& food_list, const std::vector<double>* precomputed_state) {
    std::vector<double> state = precomputed_state ? *precomputed_state : get_state_representation(food_list);
    
        // Epsilon-greedy exploration
    if (dis(gen) < explore_rate) {
        // Smart exploration: sometimes move toward food
        if (dis(gen) < 0.3) {
            Position closest_food_pos = find_closest_food(food_list);
            if (closest_food_pos.x != -1) {
                if (closest_food_pos.x < pos.x) return LEFT;
                if (closest_food_pos.x > pos.x) return RIGHT;
                if (closest_food_pos.y < pos.y) return UP;
                if (closest_food_pos.y > pos.y) return DOWN;
            }
        }
        // Random action
        return gen() % 4;
    }
    
    // Exploitation: choose action with highest Q-value
    last_q_values = neural_network->forward(state);
    int best_action = 0;
    double max_q = last_q_values[0];
    for (int i = 1; i < OUTPUT_SIZE; ++i) {
        if (last_q_values[i] > max_q) {
            max_q = last_q_values[i];
            best_action = i;
        }
    }
    
    // Update action history (for potential visualization)
    last_actions.push_back(static_cast<Action>(best_action));
    if (last_actions.size() > 3) last_actions.erase(last_actions.begin());
    
    // Update visualization parameters
    brain_learning_rate = 5 + static_cast<int>(30 * explore_rate);
    for (size_t i = 0; i < synaptic_strength.size(); ++i) {
        if (gen() % 100 < brain_learning_rate) {
            synaptic_strength[i] = dis(gen);
        }
    }
    
    return best_action;
}

Position AI::find_closest_food(const std::vector<Food>& food_list) const {
    if (food_list.empty()) {
        return {-1, -1};  // Sentinel value for "no food"
    }
    
    double min_distance = 1e9;
    Position closest{-1, -1};
    
    for (const auto& food : food_list) {
        double dist = pos.distance(food.pos);
        if (dist < min_distance) {
            min_distance = dist;
            closest = food.pos;
        }
    }
    
    return closest;
}

// ============================================================================
// Environment Interaction
// ============================================================================

void AI::move(int action) {
    Position old_pos = pos;
    
    switch (action) {
        case LEFT:  if (pos.x > 0) pos.x--; break;
        case RIGHT: if (pos.x < GRID_SIZE - 1) pos.x++; break;
        case UP:    if (pos.y > 0) pos.y--; break;
        case DOWN:  if (pos.y < GRID_SIZE - 1) pos.y++; break;
    }
    
    energy -= ENERGY_DECAY;
}

void AI::add_experience(const std::vector<double>& state, int action, 
                         double reward, const std::vector<double>& next_state, 
                         bool done) {
    experience_buffer.push_back({state, action, reward, next_state, done});
    
    // Maintain buffer size
    if (experience_buffer.size() > EXPERIENCE_BUFFER_SIZE) {
        experience_buffer.pop_front();
    }
}

void AI::learn_from_experience() {
    if (experience_buffer.size() < BATCH_SIZE) return;
    
    // Sample mini-batch and train
    std::vector<int> batch_indices;
    for (int i = 0; i < BATCH_SIZE; ++i) {
        batch_indices.push_back(gen() % experience_buffer.size());
    }
    
    // Train on batch
    for (int idx : batch_indices) {
        const Experience& exp = experience_buffer[idx];
        
        // Get Q-values from both networks
        std::vector<double> current_q_values = neural_network->forward(exp.state);
        std::vector<double> next_q_values = target_network->forward(exp.next_state);
        
        // Maximum Q-value for next state
        double max_next_q = *std::max_element(next_q_values.begin(), next_q_values.end());
        
        // Target Q-values (DQN update rule)
        std::vector<double> target_q_values = current_q_values;
        if (exp.done) {
            target_q_values[exp.action] = exp.reward;
        } else {
            target_q_values[exp.action] = exp.reward + DISCOUNT_FACTOR * max_next_q;
        }
        
        // Train the network
        neural_network->train(exp.state, target_q_values, LEARNING_RATE);
    }
    
    // Periodically update target network
    update_counter++;
    if (update_counter >= TARGET_UPDATE_FREQUENCY) {
        target_network->copy_weights_from(*neural_network);
        update_counter = 0;
    }
}

// ============================================================================
// State Queries
// ============================================================================

bool AI::is_alive() const { 
    return energy > 0; 
}

void AI::decay_exploration() { 
    explore_rate = std::max(MIN_EXPLORE_RATE, explore_rate * EXPLORE_DECAY); 
}

double AI::get_explore_rate() const { 
    return explore_rate; 
}

float AI::get_energy_percentage() const { 
    return static_cast<float>(energy) / MAX_ENERGY; 
}

void AI::mutate(double mutation_rate) { 
    neural_network->mutate(mutation_rate, gen, dis); 
}

NeuralNetwork* AI::get_neural_network() const { 
    return neural_network.get(); 
}

double AI::get_layer_output(int layer_index, int neuron_index) const {
    return neural_network->get_layer_output(layer_index, neuron_index);
}

double AI::get_weight(int layer_index, int neuron_index, int next_neuron_index) const {
    return neural_network->get_weight(layer_index, neuron_index, next_neuron_index);
}

std::vector<double> AI::get_last_q_values() const {
    return last_q_values;
}

std::vector<double> AI::get_state_for_debug(const std::vector<Food>& food_list) const {
    return get_state_representation(food_list);
}
