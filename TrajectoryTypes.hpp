#pragma once
#include <vector>
#include <cmath>

struct FlightSample {
    double timeSec = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    double pitchDeg = 0.0;
    double yawDeg = 0.0;
    double speed = 0.0;
    double altitudeSpeedException = 0.0;
    double angleSpeedException = 0.0;
};

struct NormalizationStats {
    std::vector<double> minVals;
    std::vector<double> maxVals;
};

inline double degToRad(double deg) {
    return deg * 3.14159265358979323846 / 180.0;
}

inline double radToDeg(double rad) {
    return rad * 180.0 / 3.14159265358979323846;
}

inline double wrapYawDeg(double yaw) {
    while (yaw < 0.0) yaw += 360.0;
    while (yaw >= 360.0) yaw -= 360.0;
    return yaw;
}

inline double shortestAngleDeltaDeg(double fromDeg, double toDeg) {
    double d = toDeg - fromDeg;
    while (d > 180.0) d -= 360.0;
    while (d < -180.0) d += 360.0;
    return d;
}

inline std::vector<double> sampleToFeatures(const FlightSample& s) {
    double yawRad = degToRad(s.yawDeg);
    return {
        s.latitude,
        s.longitude,
        s.altitude,
        s.pitchDeg,
        std::sin(yawRad),
        std::cos(yawRad),
        s.speed,
        s.altitudeSpeedException,
        s.angleSpeedException
    };
}

inline std::vector<double> buildDeltaTarget(const FlightSample& current, const FlightSample& next) {
    return {
        next.latitude - current.latitude,
        next.longitude - current.longitude,
        next.altitude - current.altitude,
        next.pitchDeg - current.pitchDeg,
        shortestAngleDeltaDeg(current.yawDeg, next.yawDeg),
        next.speed - current.speed
    };
}