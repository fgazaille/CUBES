/**
 * @file neural_network.hpp
 * @brief Neural network implementation with backpropagation for reinforcement learning.
 * 
 * This module implements a feedforward neural network with:
 * - Multiple hidden layers with ReLU activation
 * - Xavier initialization for weights
 * - Backpropagation for training
 * - Genetic algorithm support (mutation, genome serialization)
 * - Target network support for DQN (Deep Q-Network)
 * 
 * The network is used by AI agents to approximate Q-values for
 * state-action pairs in the reinforcement learning setup.
 */

#ifndef NEURAL_NETWORK_HPP
#define NEURAL_NETWORK_HPP

#include <vector>
#include <random>

/**
 * @brief A single layer in the neural network.
 * 
 * Each layer contains:
 * - Weights matrix (output_size x input_size)
 * - Biases vector (output_size)
 * - Cached last output for backpropagation
 * 
 * Uses ReLU activation: f(x) = max(0, x)
 */
class Layer {
private:
    int input_size;                          ///< Number of inputs to this layer
    int output_size;                         ///< Number of neurons in this layer
    std::vector<std::vector<double>> weights; ///< Weight matrix [output][input]
    std::vector<double> biases;               ///< Bias vector [output]
    std::vector<double> last_output;          ///< Cached output from last forward pass

public:
    /**
     * @brief Construct a new Layer with Xavier initialization.
     * 
     * @param input_size Number of input connections per neuron
     * @param output_size Number of neurons in this layer
     * @param gen Random number generator (must be initialized by caller)
     * @param dis Uniform distribution for bias initialization
     */
    Layer(int input_size, int output_size, std::mt19937& gen, std::uniform_real_distribution<double>& dis);

    /**
     * @brief ReLU activation function.
     * @param x Input value
     * @return max(0, x)
     */
    double relu(double x) const;

    /**
     * @brief Forward pass through the layer.
     * 
     * Computes: output[i] = ReLU(sum_j(weights[i][j] * input[j]) + biases[i])
     * 
     * @param input Input vector of size input_size
     * @return Output vector of size output_size (cached in last_output)
     */
    std::vector<double> forward(const std::vector<double>& input);

    /**
     * @brief Update weights using gradient descent.
     * 
     * @param input Input that produced the activation
     * @param gradient Gradient from next layer (same size as output)
     * @param learning_rate Step size for weight updates
     */
    void update_weights(const std::vector<double>& input, 
                       const std::vector<double>& gradient, 
                       double learning_rate);

    /**
     * @brief Compute gradient for the previous layer during backpropagation.
     * 
     * @param next_layer_gradient Gradient from the next layer
     * @return Gradient for the previous layer
     */
    std::vector<double> compute_gradient(const std::vector<double>& next_layer_gradient);

    /**
     * @brief Apply random mutations to weights and biases.
     * 
     * @param mutation_rate Probability of mutating each parameter
     * @param gen Random number generator
     * @param dis Uniform distribution for decisions
     */
    void mutate(double mutation_rate, std::mt19937& gen, std::uniform_real_distribution<double>& dis);

    // Accessors
    const std::vector<std::vector<double>>& get_weights() const { return weights; }
    const std::vector<double>& get_biases() const { return biases; }
    const std::vector<double>& get_last_output() const { return last_output; }
    
    void set_weights(const std::vector<std::vector<double>>& new_weights) { weights = new_weights; }
    void set_biases(const std::vector<double>& new_biases) { biases = new_biases; }
};

/**
 * @brief Feedforward neural network with multiple layers.
 * 
 * Supports:
 * - Forward propagation
 * - Backpropagation training
 * - Target network copying (for DQN)
 * - Genetic operations (mutation)
 * - Genome serialization/deserialization
 */
class NeuralNetwork {
private:
    std::vector<Layer> layers;               ///< Network layers
    std::vector<std::vector<double>> layer_inputs; ///< Cached inputs to each layer

public:
    /**
     * @brief Construct a new Neural Network.
     * 
     * @param layer_sizes Vector specifying the size of each layer.
     *                  First element is input size, last is output size.
     *                  Example: {12, 32, 16, 4} = input=12, two hidden layers, output=4
     * @param gen Random number generator
     * @param dis Distribution for initialization
     */
    NeuralNetwork(const std::vector<int>& layer_sizes, std::mt19937& gen, std::uniform_real_distribution<double>& dis);

    /**
     * @brief Forward pass through the entire network.
     * 
     * @param input Input vector (must match input layer size)
     * @return Output vector (matches output layer size)
     */
    std::vector<double> forward(const std::vector<double>& input);
    
    /**
     * @brief Forward pass reusing output vector (reduces allocation).
     * 
     * @param input Input vector (must match input layer size)
     * @param output Pre-allocated output vector to fill
     */
    void forward(const std::vector<double>& input, std::vector<double>& output);

    /**
     * @brief Train the network using backpropagation with MSE loss.
     * 
     * Computes gradients using:
     * - MSE loss: L = (predicted - target)²
     * - Gradient: dL/dy = 2 * (predicted - target)
     * 
     * @param input Input state
     * @param target Desired output (Q-values)
     * @param learning_rate Step size for weight updates
     */
    void train(const std::vector<double>& input, 
               const std::vector<double>& target, 
               double learning_rate);

    /**
     * @brief Copy weights from another network (for target network updates).
     * 
     * Used in DQN to maintain a separate target network that updates
     * less frequently than the main network, improving training stability.
     * 
     * @param other Source network to copy from
     */
    void copy_weights_from(const NeuralNetwork& other);

    /**
     * @brief Apply mutation to all layers.
     * 
     * @param mutation_rate Probability of mutating each parameter
     * @param gen Random number generator
     * @param dis Distribution for decisions
     */
    void mutate(double mutation_rate, std::mt19937& gen, std::uniform_real_distribution<double>& dis);

    // Visualization helpers
    double get_layer_output(int layer_index, int neuron_index) const;
    double get_weight(int layer_index, int neuron_index, int next_neuron_index) const;

    /**
     * @brief Serialize network weights and biases into a flat vector.
     * 
     * Useful for genetic algorithms where the entire network is treated
     * as a genome that can be crossed over and mutated.
     * 
     * @return Flattened genome vector
     */
    std::vector<double> get_genome() const;

    /**
     * @brief Reconstruct network from a genome vector.
     * 
     * @param genome Flattened vector containing all weights and biases
     */
    void set_genome(const std::vector<double>& genome);
};

#endif // NEURAL_NETWORK_HPP
