// File: GreenScoreCalculator.cpp
#include "GreenScoreCalculator.h"
#include <algorithm>

// Example composite score: lower emissions => higher score
static double normalize(double value, double minVal, double maxVal) {
    if (maxVal <= minVal) return 0.0;
    double norm = (value - minVal) / (maxVal - minVal);
    return std::clamp(1.0 - norm, 0.0, 1.0);
}

double GreenScoreCalculator::computeBaseScore(const Company& company,
                                              double factoryEmissions,
                                              double employeeFootprint,
                                              double vehicleEmissions) {
    // Placeholder logic: Lower metrics are better. Score is 0-100.
    // This needs to be defined by actual business logic.
    // For example, map emissions to a penalty.
    double score = 100.0;
    score -= (factoryEmissions / 10.0); // Arbitrary scaling
    score -= (employeeFootprint / 5.0);  // Arbitrary scaling
    score -= (vehicleEmissions / 20.0); // Arbitrary scaling

    // Clamp score to a defined range, e.g., 0-100 or -100 to 100
    // Assuming -100 to 100 as per Company.h comments initially
    if (score < -100.0) return -100.0;
    if (score > 100.0) return 100.0;
    return score;
}

double GreenScoreCalculator::applyScaling(double baseScore, double scalingFactor) {
    double scaledScore = baseScore * scalingFactor;
    // Clamp scaled score to the same range, e.g., -100 to 100
    if (scaledScore < -100.0) return -100.0;
    if (scaledScore > 100.0) return 100.0;
    return scaledScore;
}