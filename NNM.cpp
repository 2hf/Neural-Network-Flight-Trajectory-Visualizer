#include <Windows.h>
#include <iostream>
#include <iomanip>
#include "CSVReader.hpp"
#include "NeuralNetwork.hpp"
#include "Simulator.hpp"
#include "FlightWindow.hpp"

int main() {
    try {
        const std::string csvPath = "data/flight_sample.csv";

        auto samples = CSVReader::readFlightCSV(csvPath);
        if (samples.size() < 10) {
            std::cerr << "Not enough samples.\n";
            return 1;
        }
        std::cout << "Building matrix\n";

        auto Xraw = Simulator::buildInputMatrix(samples);
        auto Yraw = Simulator::buildTargetMatrix(samples);

        auto inputStats = NeuralNetwork::fitNormalization(Xraw);
        auto targetStats = NeuralNetwork::fitNormalization(Yraw);

        std::vector<std::vector<double>> Xnorm, Ynorm;
        for (const auto& row : Xraw) Xnorm.push_back(NeuralNetwork::normalize(row, inputStats));
        for (const auto& row : Yraw) Ynorm.push_back(NeuralNetwork::normalize(row, targetStats));
       
        std::cout << "Training start:\n";

        NeuralNetwork nn(9, 32, 24, 6, 0.12);
        nn.train(Xnorm, Ynorm, 7000);

        size_t seedIndex = samples.size() / 2;
        FlightSample seed = samples[seedIndex];

        auto predictedPath = Simulator::simulateUntilLanding(
            nn,
            seed,
            inputStats,
            targetStats,
            400,
            1.0
        );

        const FlightSample& last = predictedPath.back();

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "\nSimulation start:\n";
        std::cout << "Lat: " << seed.latitude
            << ", Lon: " << seed.longitude
            << ", Alt: " << seed.altitude
            << ", Pitch: " << seed.pitchDeg
            << ", Yaw: " << seed.yawDeg
            << ", Speed: " << seed.speed << "\n";

        std::cout << "\nPredicted landing point:\n";
        std::cout << "Lat: " << last.latitude
            << ", Lon: " << last.longitude
            << ", Alt: " << last.altitude << "\n";

        std::cout << "\nPredicted path:\n";
        for (const auto& p : predictedPath) {
            std::cout << "t=" << std::setw(6) << p.timeSec
                << " lat=" << p.latitude
                << " lon=" << p.longitude
                << " alt=" << p.altitude
                << " pitch=" << p.pitchDeg
                << " yaw=" << p.yawDeg
                << " speed=" << p.speed
                << "\n";
        }

        FlightWindow::Show(predictedPath);
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

  
    
    while (!GetAsyncKeyState(VK_ESCAPE))
		Sleep(100);

    return 0;
}