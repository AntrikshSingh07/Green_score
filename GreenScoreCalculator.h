// File: GreenScoreCalculator.h
#ifndef GREEN_SCORE_CALCULATOR_H
#define GREEN_SCORE_CALCULATOR_H

#include "Company.h"

class GreenScoreCalculator {
public:
    static double computeBaseScore(const Company& company,
                                   double factoryEmissions,
                                   double employeeFootprint,
                                   double vehicleEmissions);
    static double applyScaling(double baseScore, double scalingFactor);
};

#endif // GREEN_SCORE_CALCULATOR_H