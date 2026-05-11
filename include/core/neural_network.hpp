#pragma once

#include <vector>
#include <random>
#include <cstddef>

class Layer {
private:
    int input_size;
    int output_size;
    std::vector<double> weights;
    std::vector<double> biases;
    std::vector<double> last_output;
    std::vector<double> last_input;
    bool use_relu;
    double* w_data = nullptr;

    // Adam optimizer state (flat storage)
    std::vector<double> m_weights;
    std::vector<double> v_weights;
    std::vector<double> m_biases;
    std::vector<double> v_biases;
    int adam_t = 0;

    size_t w_stride = 0;

public:
    Layer(int input_size, int output_size, std::mt19937& gen, bool use_relu = true);

    [[nodiscard]] constexpr double relu(double x) const noexcept {
        return std::max(0.0, x);
    }

    void forward(const std::vector<double>& input, std::vector<double>& output);

    [[nodiscard]] std::vector<double> forward(const std::vector<double>& input) {
        std::vector<double> out(output_size);
        forward(input, out);
        return out;
    }

    void update_weights(const std::vector<double>& input,
                       const std::vector<double>& gradient,
                       double learning_rate);

    [[nodiscard]] std::vector<double> compute_gradient(const std::vector<double>& next_layer_gradient);

    void mutate(double mutation_rate, std::mt19937& gen, std::uniform_real_distribution<double>& dis);

    void get_genome(std::vector<double>& out) const;
    [[nodiscard]] std::vector<double> get_genome() const {
        std::vector<double> out;
        get_genome(out);
        return out;
    }

    bool set_genome(const std::vector<double>& genome, size_t& offset);

    const std::vector<double>& get_last_output() const { return last_output; }
    const std::vector<double>& get_last_input() const { return last_input; }

    const double* get_weights_ptr() const { return weights.data(); }
    const double* get_biases_ptr() const { return biases.data(); }
    int get_input_size() const { return input_size; }
    int get_output_size() const { return output_size; }
    size_t get_w_stride() const { return w_stride; }
    bool get_use_relu() const { return use_relu; }

    size_t get_genome_size() const {
        return weights.size() + biases.size();
    }

    void copy_weights_and_biases(const Layer& other);
    void set_weights_array(const double* w, const double* b);
};

class NeuralNetwork {
private:
    std::vector<Layer> layers;

    // Pre-allocated workspace for forward/train
    std::vector<double> fwd_workspace_;

public:
    NeuralNetwork(const std::vector<int>& layer_sizes, std::mt19937& gen);

    void forward(const std::vector<double>& input, std::vector<double>& output);

    [[nodiscard]] std::vector<double> forward(const std::vector<double>& input) {
        std::vector<double> out;
        forward(input, out);
        return out;
    }

    void train(const std::vector<double>& input,
               const std::vector<double>& target,
               double learning_rate);

    void copy_weights_from(const NeuralNetwork& other);

    void mutate(double mutation_rate, std::mt19937& gen, std::uniform_real_distribution<double>& dis);

    [[nodiscard]] double get_layer_output(int layer_index, int neuron_index) const {
        return layers[layer_index].get_last_output()[neuron_index];
    }

    void get_genome(std::vector<double>& out) const;
    [[nodiscard]] std::vector<double> get_genome() const {
        std::vector<double> out;
        get_genome(out);
        return out;
    }

    bool set_genome(const std::vector<double>& genome);
};
