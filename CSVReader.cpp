#include "CSVReader.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<FlightSample> CSVReader::readFlightCSV(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open CSV: " + path);
    }

    std::vector<FlightSample> samples;
    std::string line;

    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cell;
        FlightSample s;

        std::getline(ss, cell, ','); s.timeSec = std::stod(cell);
        std::getline(ss, cell, ','); s.latitude = std::stod(cell);
        std::getline(ss, cell, ','); s.longitude = std::stod(cell);
        std::getline(ss, cell, ','); s.altitude = std::stod(cell);
        std::getline(ss, cell, ','); s.pitchDeg = std::stod(cell);
        std::getline(ss, cell, ','); s.yawDeg = std::stod(cell);
        std::getline(ss, cell, ','); s.speed = std::stod(cell);
        std::getline(ss, cell, ','); s.altitudeSpeedException = std::stod(cell);
        std::getline(ss, cell, ','); s.angleSpeedException = std::stod(cell);

        samples.push_back(s);
    }

    return samples;
}