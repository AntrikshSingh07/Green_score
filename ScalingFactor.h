// File: ScalingFactor.h
#ifndef SCALING_FACTOR_H
#define SCALING_FACTOR_H

#include <string>
#include <unordered_map>

class ScalingFactor {
private:
    std::unordered_map<std::string, double> factors;

public:
    bool loadFromFile(const std::string& filepath);
    double getFactor(const std::string& sector) const;
};

#endif // SCALING_FACTOR_H