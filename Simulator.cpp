#include "Simulator.hpp"
#include <algorithm>
#include <cmath>

std::vector<std::vector<double>> Simulator::buildInputMatrix(const std::vector<FlightSample>& samples) {
    std::vector<std::vector<double>> X;
    for (size_t i = 0; i + 1 < samples.size(); ++i) {
        X.push_back(sampleToFeatures(samples[i]));
    }
    return X;
}

std::vector<std::vector<double>> Simulator::buildTargetMatrix(const std::vector<FlightSample>& samples) {
    std::vector<std::vector<double>> Y;
    for (size_t i = 0; i + 1 < samples.size(); ++i) {
        Y.push_back(buildDeltaTarget(samples[i], samples[i + 1]));
    }
    return Y;
}

std::vector<FlightSample> Simulator::simulateUntilLanding(
    NeuralNetwork& nn,
    const FlightSample& start,
    const NormalizationStats& inputStats,
    const NormalizationStats& targetStats,
    int maxSteps,
    double dtSeconds
) {
    std::vector<FlightSample> path;
    FlightSample current = start;
    path.push_back(current);

    for (int step = 0; step < maxSteps; ++step) {
        auto x = sampleToFeatures(current);
        auto xNorm = NeuralNetwork::normalize(x, inputStats);
        auto predNorm = nn.predict(xNorm);
        auto delta = NeuralNetwork::denormalize(predNorm, targetStats);

        FlightSample next = current;
        next.timeSec += dtSeconds;
        next.latitude += delta[0];
        next.longitude += delta[1];
        next.altitude += delta[2];
        next.pitchDeg += delta[3];
        next.yawDeg = wrapYawDeg(next.yawDeg + delta[4]);
        next.speed += delta[5];

        next.altitude = std::max(0.0, next.altitude);
        next.pitchDeg = std::clamp(next.pitchDeg, -89.0, 89.0);
        next.speed = std::max(0.0, next.speed);

        if (next.altitude > 3000.0 && std::abs(next.pitchDeg) > 20.0) {
            next.speed *= 0.97;
        }
        if (std::abs(next.pitchDeg) > 35.0 || next.angleSpeedException > 0.5) {
            next.speed *= 0.95;
        }
        if (next.altitudeSpeedException > 0.5 && next.altitude > 1500.0) {
            next.speed *= 0.92;
        }

        if (next.altitude <= 0.5) {
            next.altitude = 0.0;
            path.push_back(next);
            break;
        }

        path.push_back(next);
        current = next;
    }

    return path;
}