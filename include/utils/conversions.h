#pragma once
#include <cmath> // For log10, pow



// dBm to Watts
inline double dbmToWatts(double dbm) {
    return std::pow(10.0, (dbm - 30.0) / 10.0);
}

// Watts to dBm
inline double wattsToDbm(double watts) {
    if (watts <= 0) return -HUGE_VAL; // Or throw an error
    return 10.0 * std::log10(watts) + 30.0;
}

// dB to linear
inline double dbToLinear(double db) {
    return std::pow(10.0, db / 10.0);
}

// Linear to dB
inline double linearToDb(double linear_val) {
    if (linear_val <= 0) return -HUGE_VAL; // Or throw an error
    return 10.0 * std::log10(linear_val);
}

