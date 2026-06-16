#include "NeuralNetwork.hpp"
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <random>
#include <algorithm>

NeuralNetwork::NeuralNetwork(int inputSize, int hidden1Size, int hidden2Size, int outputSize, double learningRate)
    : inSize(inputSize), h1Size(hidden1Size), h2Size(hidden2Size), outSize(outputSize), lr(learningRate) {
    initialize();
}

double NeuralNetwork::sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

double NeuralNetwork::sigmoidDerivativeFromOutput(double y) {
    return y * (1.0 - y);
}

double NeuralNetwork::clampValue(double x, double lo, double hi) {
    return std::max(lo, std::min(x, hi));
}

void NeuralNetwork::initialize() {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(-0.7, 0.7);

    W1.assign(h1Size, std::vector<double>(inSize));
    W2.assign(h2Size, std::vector<double>(h1Size));
    W3.assign(outSize, std::vector<double>(h2Size));
    B1.assign(h1Size, 0.0);
    B2.assign(h2Size, 0.0);
    B3.assign(outSize, 0.0);

    for (auto& row : W1) for (auto& w : row) w = dist(rng);
    for (auto& row : W2) for (auto& w : row) w = dist(rng);
    for (auto& row : W3) for (auto& w : row) w = dist(rng);
    for (auto& b : B1) b = dist(rng);
    for (auto& b : B2) b = dist(rng);
    for (auto& b : B3) b = dist(rng);
}

std::vector<double> NeuralNetwork::predict(const std::vector<double>& input) const {
    std::vector<double> a1(h1Size), a2(h2Size), out(outSize);

    for (int i = 0; i < h1Size; ++i) {
        double z = B1[i];
        for (int j = 0; j < inSize; ++j) z += W1[i][j] * input[j];
        a1[i] = sigmoid(z);
    }

    for (int i = 0; i < h2Size; ++i) {
        double z = B2[i];
        for (int j = 0; j < h1Size; ++j) z += W2[i][j] * a1[j];
        a2[i] = sigmoid(z);
    }

    for (int i = 0; i < outSize; ++i) {
        double z = B3[i];
        for (int j = 0; j < h2Size; ++j) z += W3[i][j] * a2[j];
        out[i] = sigmoid(z);
    }

    return out;
}

void NeuralNetwork::train(const std::vector<std::vector<double>>& X,
    const std::vector<std::vector<double>>& Y,
    int epochs) {
    if (X.empty() || Y.empty() || X.size() != Y.size()) {
        throw std::runtime_error("Invalid training data.");
    }

    for (int epoch = 0; epoch < epochs; ++epoch) {
        double totalLoss = 0.0;

        for (size_t n = 0; n < X.size(); ++n) {
            const auto& x = X[n];
            const auto& y = Y[n];

            std::vector<double> a1(h1Size), a2(h2Size), out(outSize);

            for (int i = 0; i < h1Size; ++i) {
                double z = B1[i];
                for (int j = 0; j < inSize; ++j) z += W1[i][j] * x[j];
                a1[i] = sigmoid(z);
            }

            for (int i = 0; i < h2Size; ++i) {
                double z = B2[i];
                for (int j = 0; j < h1Size; ++j) z += W2[i][j] * a1[j];
                a2[i] = sigmoid(z);
            }

            for (int i = 0; i < outSize; ++i) {
                double z = B3[i];
                for (int j = 0; j < h2Size; ++j) z += W3[i][j] * a2[j];
                out[i] = sigmoid(z);
            }

            std::vector<double> delta3(outSize);
            for (int i = 0; i < outSize; ++i) {
                double error = y[i] - out[i];
                totalLoss += error * error;
                delta3[i] = error * sigmoidDerivativeFromOutput(out[i]);
            }

            std::vector<double> delta2(h2Size, 0.0);
            for (int i = 0; i < h2Size; ++i) {
                double sum = 0.0;
                for (int j = 0; j < outSize; ++j) sum += W3[j][i] * delta3[j];
                delta2[i] = sum * sigmoidDerivativeFromOutput(a2[i]);
            }

            std::vector<double> delta1(h1Size, 0.0);
            for (int i = 0; i < h1Size; ++i) {
                double sum = 0.0;
                for (int j = 0; j < h2Size; ++j) sum += W2[j][i] * delta2[j];
                delta1[i] = sum * sigmoidDerivativeFromOutput(a1[i]);
            }

            for (int i = 0; i < outSize; ++i) {
                for (int j = 0; j < h2Size; ++j) W3[i][j] += lr * delta3[i] * a2[j];
                B3[i] += lr * delta3[i];
            }

            for (int i = 0; i < h2Size; ++i) {
                for (int j = 0; j < h1Size; ++j) W2[i][j] += lr * delta2[i] * a1[j];
                B2[i] += lr * delta2[i];
            }

            for (int i = 0; i < h1Size; ++i) {
                for (int j = 0; j < inSize; ++j) W1[i][j] += lr * delta1[i] * x[j];
                B1[i] += lr * delta1[i];
            }
        }

        if (epoch % 500 == 0) {
            std::cout << "Epoch " << epoch << " MSE: " << totalLoss / X.size() << "\n";
        }
    }
}

NormalizationStats NeuralNetwork::fitNormalization(const std::vector<std::vector<double>>& data) {
    if (data.empty()) throw std::runtime_error("No data for normalization.");

    NormalizationStats stats;
    stats.minVals = data[0];
    stats.maxVals = data[0];

    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            stats.minVals[i] = std::min(stats.minVals[i], row[i]);
            stats.maxVals[i] = std::max(stats.maxVals[i], row[i]);
        }
    }

    return stats;
}

std::vector<double> NeuralNetwork::normalize(const std::vector<double>& row, const NormalizationStats& stats) {
    std::vector<double> out(row.size(), 0.0);
    for (size_t i = 0; i < row.size(); ++i) {
        double range = stats.maxVals[i] - stats.minVals[i];
        if (range == 0.0) out[i] = 0.0;
        else out[i] = (row[i] - stats.minVals[i]) / range;
        out[i] = clampValue(out[i], 0.0, 1.0);
    }
    return out;
}

std::vector<double> NeuralNetwork::denormalize(const std::vector<double>& row, const NormalizationStats& stats) {
    std::vector<double> out(row.size(), 0.0);
    for (size_t i = 0; i < row.size(); ++i) {
        out[i] = stats.minVals[i] + row[i] * (stats.maxVals[i] - stats.minVals[i]);
    }
    return out;
}