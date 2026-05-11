#include "neural_network.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstring>

Layer::Layer(int input_size, int output_size, std::mt19937& gen, bool use_relu)
    : input_size(input_size), output_size(output_size), use_relu(use_relu),
      w_stride(static_cast<size_t>(input_size)) {

    size_t nw = static_cast<size_t>(output_size) * static_cast<size_t>(input_size);
    weights.resize(nw);
    biases.resize(output_size, 0.0);
    last_output.resize(output_size);
    last_input.resize(input_size);
    w_data = weights.data();

    double weight_range = std::sqrt(6.0 / (input_size + output_size));
    std::uniform_real_distribution<double> weight_dist(-weight_range, weight_range);
    for (size_t i = 0; i < nw; ++i)
        weights[i] = weight_dist(gen);

    m_weights.resize(nw, 0.0);
    v_weights.resize(nw, 0.0);
    m_biases.resize(output_size, 0.0);
    v_biases.resize(output_size, 0.0);
    adam_t = 0;
}

void Layer::forward(const std::vector<double>& input, std::vector<double>& output) {
    last_input = input;
    output.resize(output_size);
    const double* w = w_data;

    for (int i = 0; i < output_size; ++i) {
        double sum = biases[i];
        for (int j = 0; j < input_size; ++j)
            sum += w[i * w_stride + j] * input[j];
        output[i] = use_relu ? relu(sum) : sum;
    }
    last_output = output;
}

void Layer::update_weights(const std::vector<double>& input,
                           const std::vector<double>& gradient,
                           double learning_rate) {
    constexpr double beta1 = 0.9;
    constexpr double beta2 = 0.999;
    constexpr double eps = 1e-8;
    adam_t++;
    double lr = std::clamp(learning_rate, 1e-6, 1.0);
    double* w = weights.data();

    double b1t = 1.0 - std::pow(beta1, adam_t);
    double b2t = 1.0 - std::pow(beta2, adam_t);

    for (int i = 0; i < output_size; ++i) {
        double grad = gradient[i];
        if (std::isnan(grad) || std::isinf(grad)) continue;

        if (use_relu)
            grad *= (last_output[i] > 0) ? 1.0 : 0.0;

        grad = std::clamp(grad, -5.0, 5.0);

        m_biases[i] = beta1 * m_biases[i] + (1.0 - beta1) * grad;
        v_biases[i] = beta2 * v_biases[i] + (1.0 - beta2) * grad * grad;
        biases[i] -= lr * (m_biases[i] / b1t) / (std::sqrt(v_biases[i] / b2t) + eps);

        size_t row_off = static_cast<size_t>(i) * w_stride;
        for (int j = 0; j < input_size; ++j) {
            double iv = input[j];
            if (std::isnan(iv) || std::isinf(iv)) continue;

            double wg = grad * iv;
            wg = std::clamp(wg, -5.0, 5.0);

            size_t idx = row_off + static_cast<size_t>(j);
            m_weights[idx] = beta1 * m_weights[idx] + (1.0 - beta1) * wg;
            v_weights[idx] = beta2 * v_weights[idx] + (1.0 - beta2) * wg * wg;
            w[idx] -= lr * (m_weights[idx] / b1t) / (std::sqrt(v_weights[idx] / b2t) + eps);
        }
    }
}

std::vector<double> Layer::compute_gradient(const std::vector<double>& next_layer_gradient) {
    std::vector<double> gradient(input_size, 0.0);
    const double* w = w_data;

    for (int j = 0; j < input_size; ++j) {
        double sum = 0.0;
        for (int i = 0; i < output_size; ++i) {
            double ad = use_relu ? ((last_output[i] > 0) ? 1.0 : 0.0) : 1.0;
            sum += next_layer_gradient[i] * w[i * w_stride + j] * ad;
        }
        gradient[j] = sum;
    }
    return gradient;
}

void Layer::mutate(double mutation_rate, std::mt19937& gen, std::uniform_real_distribution<double>& dis) {
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    for (auto& w : weights) {
        if (dis(gen) < mutation_rate)
            w += dist(gen) * 0.1;
    }
    for (auto& b : biases) {
        if (dis(gen) < mutation_rate)
            b += dist(gen) * 0.1;
    }
}

void Layer::get_genome(std::vector<double>& out) const {
    size_t pos = out.size();
    out.resize(pos + weights.size() + biases.size());
    std::memcpy(&out[pos], weights.data(), weights.size() * sizeof(double));
    std::memcpy(&out[pos + weights.size()], biases.data(), biases.size() * sizeof(double));
}

bool Layer::set_genome(const std::vector<double>& genome, size_t& offset) {
    size_t nw = static_cast<size_t>(output_size) * static_cast<size_t>(input_size);
    size_t nb = static_cast<size_t>(output_size);
    if (offset + nw + nb > genome.size()) return false;

    std::memcpy(weights.data(), &genome[offset], nw * sizeof(double));
    offset += nw;
    std::memcpy(biases.data(), &genome[offset], nb * sizeof(double));
    offset += nb;
    return true;
}

void Layer::copy_weights_and_biases(const Layer& other) {
    std::memcpy(weights.data(), other.weights.data(), weights.size() * sizeof(double));
    std::memcpy(biases.data(), other.biases.data(), biases.size() * sizeof(double));
}

void Layer::set_weights_array(const double* w, const double* b) {
    std::memcpy(weights.data(), w, weights.size() * sizeof(double));
    std::memcpy(biases.data(), b, biases.size() * sizeof(double));
}

// ============================================================================
// NeuralNetwork Implementation
// ============================================================================

NeuralNetwork::NeuralNetwork(const std::vector<int>& layer_sizes, std::mt19937& gen) {
    for (size_t i = 0; i < layer_sizes.size() - 1; ++i) {
        bool use_relu = (i < layer_sizes.size() - 2);
        layers.emplace_back(layer_sizes[i], layer_sizes[i + 1], gen, use_relu);
    }
    if (!layers.empty())
        fwd_workspace_.reserve(layers.back().get_output_size());
}

void NeuralNetwork::forward(const std::vector<double>& input, std::vector<double>& output) {
    if (layers.empty()) { output.clear(); return; }

    size_t n = layers.size();
    layers[0].forward(input, fwd_workspace_);

    for (size_t i = 1; i < n - 1; ++i) {
        std::vector<double> tmp;
        layers[i].forward(fwd_workspace_, tmp);
        fwd_workspace_ = std::move(tmp);
    }

    if (n > 1) {
        layers[n - 1].forward(fwd_workspace_, output);
    } else {
        output = fwd_workspace_;
    }
}

void NeuralNetwork::train(const std::vector<double>& input,
                           const std::vector<double>& target,
                           double learning_rate) {
    if (layers.empty()) return;

    // Forward pass using workspace
    forward(input, fwd_workspace_);

    for (double val : fwd_workspace_) {
        if (std::isnan(val) || std::isinf(val)) return;
    }

    size_t os = fwd_workspace_.size();
    std::vector<double> gradient(os);
    double norm = 0.0;
    for (size_t i = 0; i < os; ++i) {
        double g = 2.0 * (fwd_workspace_[i] - target[i]);
        if (std::isnan(g) || std::isinf(g)) return;
        gradient[i] = g;
        norm += g * g;
    }
    norm = std::sqrt(norm);
    constexpr double MAX_GRAD_NORM = 10.0;
    if (norm > MAX_GRAD_NORM) {
        double scale = MAX_GRAD_NORM / norm;
        for (auto& g : gradient) g *= scale;
    }

    for (int i = static_cast<int>(layers.size()) - 1; i >= 0; --i) {
        layers[i].update_weights(layers[i].get_last_input(), gradient, learning_rate);
        if (i > 0)
            gradient = layers[i].compute_gradient(gradient);
    }
}

void NeuralNetwork::copy_weights_from(const NeuralNetwork& other) {
    for (size_t i = 0; i < layers.size(); ++i)
        layers[i].copy_weights_and_biases(other.layers[i]);
}

void NeuralNetwork::mutate(double mutation_rate, std::mt19937& gen, std::uniform_real_distribution<double>& dis) {
    for (auto& layer : layers)
        layer.mutate(mutation_rate, gen, dis);
}

void NeuralNetwork::get_genome(std::vector<double>& out) const {
    for (const auto& layer : layers)
        layer.get_genome(out);
}

bool NeuralNetwork::set_genome(const std::vector<double>& genome) {
    size_t offset = 0;
    for (auto& layer : layers) {
        if (!layer.set_genome(genome, offset))
            return false;
    }
    return true;
}
