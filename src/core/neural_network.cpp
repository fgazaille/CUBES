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
#include <numeric>  // for std::clamp

// ============================================================================
// Layer Implementation
// ============================================================================

Layer::Layer(int input_size, int output_size, std::mt19937& gen, bool use_relu)
    : input_size(input_size), output_size(output_size), use_relu(use_relu) {
    
    // Initialize weights matrix [output_size][input_size]
    weights.resize(output_size, std::vector<double>(input_size));
    
    // Xavier/Glorot initialization: keeps gradients stable across layers
    // Range = [-sqrt(6/(fan_in+fan_out)), sqrt(6/(fan_in+fan_out))]
    double weight_range = std::sqrt(6.0 / (input_size + output_size));
    std::uniform_real_distribution<double> weight_dist(-weight_range, weight_range);
    
    for (int i = 0; i < output_size; ++i) {
        for (int j = 0; j < input_size; ++j) {
            weights[i][j] = weight_dist(gen);
        }
    }
    
    // Initialize biases to zero (ReLU layers work better with zero biases initially)
    biases.resize(output_size, 0.0);
    
    // Initialize cached vectors
    last_output.resize(output_size, 0.0);
    last_input.resize(input_size, 0.0);

    // Initialize Adam optimizer state
    m_weights.resize(output_size, std::vector<double>(input_size, 0.0));
    v_weights.resize(output_size, std::vector<double>(input_size, 0.0));
    m_biases.resize(output_size, 0.0);
    v_biases.resize(output_size, 0.0);
    adam_t = 0;
}

std::vector<double> Layer::forward(const std::vector<double>& input) {
    last_input = input;
    last_output.resize(output_size);
    
    for (int i = 0; i < output_size; ++i) {
        double sum = biases[i];
        for (int j = 0; j < input_size; ++j) {
            sum += weights[i][j] * input[j];
        }
        // Apply ReLU only for hidden layers; output layer uses linear activation
        if (use_relu) {
            last_output[i] = relu(sum);
        } else {
            last_output[i] = sum;
        }
    }
    return last_output;
}

void Layer::update_weights(const std::vector<double>& input, 
                           const std::vector<double>& gradient, 
                           double learning_rate) {
    // Adam optimizer hyperparameters
    constexpr double beta1 = 0.9;
    constexpr double beta2 = 0.999;
    constexpr double eps = 1e-8;
    adam_t++;

    double lr = std::clamp(learning_rate, 1e-6, 1.0);
    
    for (int i = 0; i < output_size; ++i) {
        if (std::isnan(gradient[i]) || std::isinf(gradient[i])) {
            continue;
        }

        // Apply ReLU derivative: gradient *= ReLU'(z)  (only for hidden layers)
        double grad = gradient[i];
        if (use_relu) {
            double relu_deriv = (last_output[i] > 0) ? 1.0 : 0.0;
            grad *= relu_deriv;
        }

        // Clip gradient to prevent explosion
        grad = std::clamp(grad, -5.0, 5.0);

        // Update bias with Adam
        m_biases[i] = beta1 * m_biases[i] + (1.0 - beta1) * grad;
        v_biases[i] = beta2 * v_biases[i] + (1.0 - beta2) * grad * grad;
        double m_hat_b = m_biases[i] / (1.0 - std::pow(beta1, adam_t));
        double v_hat_b = v_biases[i] / (1.0 - std::pow(beta2, adam_t));
        biases[i] -= lr * m_hat_b / (std::sqrt(v_hat_b) + eps);

        for (int j = 0; j < input_size; ++j) {
            if (std::isnan(input[j]) || std::isinf(input[j])) {
                continue;
            }

            double w_grad = grad * input[j];
            w_grad = std::clamp(w_grad, -5.0, 5.0);

            // Update weight with Adam
            m_weights[i][j] = beta1 * m_weights[i][j] + (1.0 - beta1) * w_grad;
            v_weights[i][j] = beta2 * v_weights[i][j] + (1.0 - beta2) * w_grad * w_grad;
            double m_hat_w = m_weights[i][j] / (1.0 - std::pow(beta1, adam_t));
            double v_hat_w = v_weights[i][j] / (1.0 - std::pow(beta2, adam_t));
            weights[i][j] -= lr * m_hat_w / (std::sqrt(v_hat_w) + eps);
        }
    }
}

std::vector<double> Layer::compute_gradient(const std::vector<double>& next_layer_gradient) {
    // Backpropagate gradient to previous layer
    // gradient_j = sum_i(gradient_i * weight[i][j] * activation_derivative(output_i))
    // ReLU derivative: 1 if output > 0, else 0
    // Linear derivative: 1 (always, for output layer)
    std::vector<double> gradient(input_size, 0.0);
    
    for (int j = 0; j < input_size; ++j) {
        for (int i = 0; i < output_size; ++i) {
            double act_deriv = use_relu ? ((last_output[i] > 0) ? 1.0 : 0.0) : 1.0;
            gradient[j] += next_layer_gradient[i] * weights[i][j] * act_deriv;
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
                             std::mt19937& gen) {
    // Allocate space for layer inputs (used during backpropagation)
    layer_inputs.resize(layer_sizes.size());
    
    // Create layers: each layer connects layer[i] to layer[i+1]
    // Hidden layers use ReLU; the output layer uses linear activation
    for (size_t i = 0; i < layer_sizes.size() - 1; ++i) {
        bool use_relu = (i < layer_sizes.size() - 2);
        layers.push_back(Layer(layer_sizes[i], layer_sizes[i+1], gen, use_relu));
    }
    
    // Pre-allocate forward cache if we have layers
    if (!layers.empty()) {
        forward_cache_.resize(layer_sizes.back(), 0.0);  // output size
    }
}

std::vector<double> NeuralNetwork::forward(const std::vector<double>& input) {
    layer_inputs[0] = input;
    forward_cache_ = input;
    
    // Forward pass through each layer
    for (size_t i = 0; i < layers.size(); ++i) {
        forward_cache_ = layers[i].forward(forward_cache_);
        if (i < layers.size() - 1) {
            layer_inputs[i+1] = forward_cache_;  // Cache for backprop
        }
    }
    return forward_cache_;
}

void NeuralNetwork::forward(const std::vector<double>& input, std::vector<double>& output) {
    layer_inputs[0] = input;
    forward_cache_ = input;
    
    // Forward pass through each layer
    for (size_t i = 0; i < layers.size(); ++i) {
        forward_cache_ = layers[i].forward(forward_cache_);
        if (i < layers.size() - 1) {
            layer_inputs[i+1] = forward_cache_;
        }
    }
    output = forward_cache_;
}

void NeuralNetwork::train(const std::vector<double>& input, 
                           const std::vector<double>& target, 
                           double learning_rate) {
    // Forward pass
    std::vector<double> predicted = forward(input);
    
    // Validate predicted values
    for (double val : predicted) {
        if (std::isnan(val) || std::isinf(val)) {
            return;
        }
    }
    
    // Compute output gradient using MSE loss derivative:
    // L = (predicted - target)^2
    // dL/dy = 2 * (predicted - target)
    std::vector<double> output_gradient(predicted.size());
    for (size_t i = 0; i < predicted.size(); ++i) {
        output_gradient[i] = 2.0 * (predicted[i] - target[i]);
        
        if (std::isnan(output_gradient[i]) || std::isinf(output_gradient[i])) {
            return;
        }
    }
    
    // Global gradient norm clipping (prevents exploding gradients)
    double norm = 0.0;
    for (double g : output_gradient) norm += g * g;
    norm = std::sqrt(norm);
    constexpr double MAX_GRAD_NORM = 10.0;
    double scaling = (norm > MAX_GRAD_NORM) ? MAX_GRAD_NORM / norm : 1.0;
    
    std::vector<double> gradient = output_gradient;
    for (size_t i = 0; i < gradient.size(); ++i) {
        gradient[i] *= scaling;
    }
    
    // Backpropagate through layers (from output to input)
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
    // Pre-calculate size to avoid reallocations
    size_t total_size = 0;
    for (const auto& layer : layers) {
        const auto& weights = layer.get_weights();
        total_size += weights.size() * weights[0].size() + layer.get_biases().size();
    }
    
    std::vector<double> genome;
    genome.reserve(total_size);
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
        const auto& weights_ref = layer.get_weights();
        const auto& biases_ref = layer.get_biases();
        
        // Get dimensions
        size_t weight_rows = weights_ref.size();
        size_t weight_cols = weight_rows > 0 ? weights_ref[0].size() : 0;
        size_t bias_size = biases_ref.size();
        
        // Create new weights and biases
        std::vector<std::vector<double>> weights(weight_rows, std::vector<double>(weight_cols));
        std::vector<double> biases(bias_size);
        
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
