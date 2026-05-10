#include "../include/core/neural_network.hpp"
#include <cassert>
#include <iostream>

void test_layer_creation() {
    std::mt19937 gen(std::random_device{}());

    Layer layer(3, 2, gen);
    std::vector<double> input = {1.0, 2.0, 3.0};
    std::vector<double> output = layer.forward(input);

    assert(output.size() == 2);
    std::cout << "test_layer_creation passed\n";
}

void test_network_forward() {
    std::mt19937 gen(std::random_device{}());

    NeuralNetwork net({3, 4, 2}, gen);
    std::vector<double> input = {1.0, 2.0, 3.0};
    std::vector<double> output = net.forward(input);

    assert(output.size() == 2);
    std::cout << "test_network_forward passed\n";
}

void test_network_genome() {
    std::mt19937 gen(std::random_device{}());

    NeuralNetwork net({2, 3, 1}, gen);
    std::vector<double> genome = net.get_genome();

    assert(!genome.empty());

    NeuralNetwork net2({2, 3, 1}, gen);
    net2.set_genome(genome);

    std::vector<double> genome2 = net2.get_genome();
    assert(genome.size() == genome2.size());
    for (size_t i = 0; i < genome.size(); ++i) {
        assert(genome[i] == genome2[i]);
    }
    std::cout << "test_network_genome passed\n";
}

void test_network_copy_weights() {
    std::mt19937 gen(std::random_device{}());

    NeuralNetwork net({3, 4, 2}, gen);
    NeuralNetwork target({3, 4, 2}, gen);

    target.copy_weights_from(net);

    std::vector<double> input = {0.5, 0.5, 0.5};
    std::vector<double> output_net = net.forward(input);
    std::vector<double> output_target = target.forward(input);

    assert(output_net.size() == output_target.size());
    for (size_t i = 0; i < output_net.size(); ++i) {
        assert(std::abs(output_net[i] - output_target[i]) < 1e-6);
    }
    std::cout << "test_network_copy_weights passed\n";
}

void test_network_train() {
    std::mt19937 gen(std::random_device{}());

    NeuralNetwork net({2, 3, 1}, gen);
    std::vector<double> input = {0.5, 0.8};
    std::vector<double> output = net.forward(input);

    std::vector<double> target = {0.7};
    net.train(input, target, 0.1);

    std::vector<double> output2 = net.forward(input);
    bool changed = false;
    for (size_t i = 0; i < output.size(); ++i) {
        if (std::abs(output[i] - output2[i]) > 1e-6) {
            changed = true;
            break;
        }
    }
    assert(changed);
    std::cout << "test_network_train passed\n";
}

int main() {
    std::cout << "Running neural network tests...\n";
    test_layer_creation();
    test_network_forward();
    test_network_genome();
    test_network_copy_weights();
    test_network_train();
    std::cout << "All tests passed!\n";
    return 0;
}
