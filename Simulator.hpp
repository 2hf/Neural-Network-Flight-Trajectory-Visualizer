#pragma once
#include "TrajectoryTypes.hpp"
#include "NeuralNetwork.hpp"
#include <vector>

class Simulator {
public:
    static std::vector<std::vector<double>> buildInputMatrix(const std::vector<FlightSample>& samples);
    static std::vector<std::vector<double>> buildTargetMatrix(const std::vector<FlightSample>& samples);

    static std::vector<FlightSample> simulateUntilLanding(
        NeuralNetwork& nn,
        const FlightSample& start,
        const NormalizationStats& inputStats,
        const NormalizationStats& targetStats,
        int maxSteps,
        double dtSeconds
    );
};