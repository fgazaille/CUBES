/**
 * @file neural_network.cpp
 * @brief Implementation of neural network classes.
 * 
 * Implements a feedforward neural network with:
 * - Xavier initialization for weights
 * - ReLU activation function
 * - Backpropagation with MSE loss
 * - Support for genetic operations (mutation)
 */

#include "neural_network.hpp"
#include <cmath>
#include <algorithm>

// ============================================================================
// Layer Implementation
// ============================================================================

Layer::Layer(int input_size, int output_size, std::mt19937& gen, std::uniform_real_distribution<double>& dis)
    : input_size(input_size), output_size(output_size) {
    
    // Initialize weights matrix [output_size][input_size]
    weights.resize(output_size, std::vector<double>(input_size));
    
    // Xavier initialization: keeps gradients stable across layers
    // Range = [-sqrt(6/(fan_in+fan_out)), sqrt(6/(fan_in+fan_out))]
    double weight_range = std::sqrt(6.0 / (input_size + output_size));
    std::uniform_real_distribution<double> weight_dist(-weight_range, weight_range);
    
    for (int i = 0; i < output_size; ++i) {
        for (int j = 0; j < input_size; ++j) {
            weights[i][j] = weight_dist(gen);
        }
    }
    
    // Initialize biases to small random values near zero
    biases.resize(output_size);
    for (int i = 0; i < output_size; ++i) {
        biases[i] = dis(gen) * 0.1 - 0.05;  // Range: [-0.05, 0.05]
    }
    
    // Initialize cached output vector
    last_output.resize(output_size, 0.0);
}

double Layer::relu(double x) const {
    return std::max(0.0, x);
}

std::vector<double> Layer::forward(const std::vector<double>& input) {
    last_output.resize(output_size);
    
    // Compute: output[i] = ReLU(sum_j(weights[i][j] * input[j]) + biases[i])
    for (int i = 0; i < output_size; ++i) {
        double sum = biases[i];
        for (int j = 0; j < input_size; ++j) {
            sum += weights[i][j] * input[j];
        }
        last_output[i] = relu(sum);
    }
    return last_output;
}

void Layer::update_weights(const std::vector<double>& input, 
                           const std::vector<double>& gradient, 
                           double learning_rate) {
    // Gradient descent: weight -= learning_rate * gradient
    // For each output neuron:
    //   bias[i] -= lr * gradient[i]
    //   weight[i][j] -= lr * gradient[i] * input[j]
    
    // Clamp learning rate to prevent explosion
    double lr = std::max(0.0001, std::min(1.0, learning_rate));
    
    for (int i = 0; i < output_size; ++i) {
        // Skip if gradient is invalid
        if (std::isnan(gradient[i]) || std::isinf(gradient[i])) {
            continue;
        }
        
        double update = lr * gradient[i];
        // Clamp update to prevent explosion
        update = std::max(-1.0, std::min(1.0, update));
        
        biases[i] -= update;
        // Clamp bias
        biases[i] = std::max(-10.0, std::min(10.0, biases[i]));
        
        for (int j = 0; j < input_size; ++j) {
            // Skip if input is invalid
            if (std::isnan(input[j]) || std::isinf(input[j])) {
                continue;
            }
            
            double w_update = update * input[j];
            w_update = std::max(-1.0, std::min(1.0, w_update));
            
            weights[i][j] -= w_update;
            // Clamp weight to prevent explosion
            weights[i][j] = std::max(-10.0, std::min(10.0, weights[i][j]));
        }
    }
}

std::vector<double> Layer::compute_gradient(const std::vector<double>& next_layer_gradient) {
    // Backpropagate gradient to previous layer
    // gradient_j = sum_i(gradient_i * weight[i][j] * ReLU_derivative(output_i))
    // ReLU derivative: 1 if output > 0, else 0
    std::vector<double> gradient(input_size, 0.0);
    
    for (int j = 0; j < input_size; ++j) {
        for (int i = 0; i < output_size; ++i) {
            double relu_derivative = (last_output[i] > 0) ? 1.0 : 0.0;
            gradient[j] += next_layer_gradient[i] * weights[i][j] * relu_derivative;
        }
    }
    return gradient;
}

void Layer::mutate(double mutation_rate, std::mt19937& gen, std::uniform_real_distribution<double>& dis) {
    // Randomly mutate weights and biases with probability = mutation_rate
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    
    for (int i = 0; i < output_size; ++i) {
        // Mutate weights
        for (int j = 0; j < input_size; ++j) {
            if (dis(gen) < mutation_rate) {
                weights[i][j] += dist(gen) * 0.1;  // Small change
            }
        }
        // Mutate bias
        if (dis(gen) < mutation_rate) {
            biases[i] += dist(gen) * 0.1;  // Small change
        }
    }
}

// ============================================================================
// NeuralNetwork Implementation
// ============================================================================

NeuralNetwork::NeuralNetwork(const std::vector<int>& layer_sizes, 
                             std::mt19937& gen, 
                             std::uniform_real_distribution<double>& dis) {
    // Allocate space for layer inputs (used during backpropagation)
    layer_inputs.resize(layer_sizes.size());
    
    // Create layers: each layer connects layer[i] to layer[i+1]
    for (size_t i = 0; i < layer_sizes.size() - 1; ++i) {
        layers.push_back(Layer(layer_sizes[i], layer_sizes[i+1], gen, dis));
    }
}

std::vector<double> NeuralNetwork::forward(const std::vector<double>& input) {
    layer_inputs[0] = input;
    std::vector<double> current_output = input;
    
    // Forward pass through each layer
    for (size_t i = 0; i < layers.size(); ++i) {
        current_output = layers[i].forward(current_output);
        if (i < layers.size() - 1) {
            layer_inputs[i+1] = current_output;  // Cache for backprop
        }
    }
    return current_output;
}

void NeuralNetwork::forward(const std::vector<double>& input, std::vector<double>& output) {
    layer_inputs[0] = input;
    std::vector<double> current_output = input;
    
    // Forward pass through each layer
    for (size_t i = 0; i < layers.size(); ++i) {
        current_output = layers[i].forward(current_output);
        if (i < layers.size() - 1) {
            layer_inputs[i+1] = current_output;
        }
    }
    output = std::move(current_output);
}

void NeuralNetwork::train(const std::vector<double>& input, 
                           const std::vector<double>& target, 
                           double learning_rate) {
    // Forward pass
    std::vector<double> predicted = forward(input);
    
    // Validate predicted values
    for (double val : predicted) {
        if (std::isnan(val) || std::isinf(val)) {
            return; // Skip training if forward pass produced invalid values
        }
    }
    
    // Compute output gradient using MSE loss derivative:
    // L = (predicted - target)^2
    // dL/dy = 2 * (predicted - target)
    std::vector<double> output_gradient(predicted.size());
    for (size_t i = 0; i < predicted.size(); ++i) {
        output_gradient[i] = 2.0 * (predicted[i] - target[i]);
        
        // Clamp gradient to prevent explosion
        if (std::isnan(output_gradient[i]) || std::isinf(output_gradient[i])) {
            return; // Skip training if gradient is invalid
        }
        output_gradient[i] = std::max(-10.0, std::min(10.0, output_gradient[i]));
    }
    
    // Backpropagate through layers (from output to input)
    std::vector<double> gradient = output_gradient;
    for (int i = layers.size() - 1; i >= 0; --i) {
        layers[i].update_weights(layer_inputs[i], gradient, learning_rate);
        if (i > 0) {
            gradient = layers[i].compute_gradient(gradient);
        }
    }
}

void NeuralNetwork::copy_weights_from(const NeuralNetwork& other) {
    // Copy weights for target network update (DQN)
    for (size_t i = 0; i < layers.size(); ++i) {
        layers[i].set_weights(other.layers[i].get_weights());
        layers[i].set_biases(other.layers[i].get_biases());
    }
}

void NeuralNetwork::mutate(double mutation_rate, std::mt19937& gen, std::uniform_real_distribution<double>& dis) {
    // Apply mutation to all layers
    for (auto& layer : layers) {
        layer.mutate(mutation_rate, gen, dis);
    }
}

double NeuralNetwork::get_layer_output(int layer_index, int neuron_index) const {
    return layers[layer_index].get_last_output()[neuron_index];
}

double NeuralNetwork::get_weight(int layer_index, int neuron_index, int next_neuron_index) const {
    // Returns: weight from neuron_index in current layer to next_neuron_index in next layer
    return layers[layer_index].get_weights()[next_neuron_index][neuron_index];
}

std::vector<double> NeuralNetwork::get_genome() const {
    // Flatten all weights and biases into a single vector (genome)
    std::vector<double> genome;
    for (const auto& layer : layers) {
        const auto& weights = layer.get_weights();
        const auto& biases = layer.get_biases();
        for (const auto& w : weights) {
            genome.insert(genome.end(), w.begin(), w.end());
        }
        genome.insert(genome.end(), biases.begin(), biases.end());
    }
    return genome;
}

void NeuralNetwork::set_genome(const std::vector<double>& genome) {
    // Reconstruct network from flattened genome
    size_t index = 0;
    for (auto& layer : layers) {
        std::vector<std::vector<double>> weights = layer.get_weights();
        std::vector<double> biases = layer.get_biases();
        
        // Read weights
        for (auto& w : weights) {
            for (auto& val : w) {
                val = genome[index++];
            }
        }
        // Read biases
        for (auto& b : biases) {
            b = genome[index++];
        }
        
        layer.set_weights(weights);
        layer.set_biases(biases);
    }
}
