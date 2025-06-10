// File: Company.h
#ifndef COMPANY_H
#define COMPANY_H

#include <string>

class Company {
public:
    using ID = int;

private:
    ID id;
    std::string name;
    std::string sector;
    double baseGreenScore;
    double adjustedGreenScore;

public:
    Company(ID id, const std::string& name, const std::string& sector, double baseScore);
    ID getID() const;
    const std::string& getName() const;
    const std::string& getSector() const;
    double getBaseGreenScore() const;
    double getAdjustedGreenScore() const;
    void setBaseGreenScore(double score);
    void setAdjustedGreenScore(double score);
};

#endif // COMPANY_H
