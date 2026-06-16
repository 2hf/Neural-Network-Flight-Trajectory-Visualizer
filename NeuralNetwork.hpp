#pragma once
#include <vector>
#include "TrajectoryTypes.hpp"

class NeuralNetwork {
public:
    NeuralNetwork(int inputSize, int hidden1Size, int hidden2Size, int outputSize, double learningRate);

    void train(const std::vector<std::vector<double>>& X,
        const std::vector<std::vector<double>>& Y,
        int epochs);

    std::vector<double> predict(const std::vector<double>& input) const;

    static NormalizationStats fitNormalization(const std::vector<std::vector<double>>& data);
    static std::vector<double> normalize(const std::vector<double>& row, const NormalizationStats& stats);
    static std::vector<double> denormalize(const std::vector<double>& row, const NormalizationStats& stats);

private:
    int inSize, h1Size, h2Size, outSize;
    double lr;

    std::vector<std::vector<double>> W1, W2, W3;
    std::vector<double> B1, B2, B3;

    static double sigmoid(double x);
    static double sigmoidDerivativeFromOutput(double y);
    static double clampValue(double x, double lo, double hi);

    void initialize();
};