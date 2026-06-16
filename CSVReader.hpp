#pragma once
#include "TrajectoryTypes.hpp"
#include <string>
#include <vector>

class CSVReader {
public:
    static std::vector<FlightSample> readFlightCSV(const std::string& path);
};