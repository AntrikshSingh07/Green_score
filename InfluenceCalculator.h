// File: InfluenceCalculator.h
#ifndef INFLUENCE_CALCULATOR_H
#define INFLUENCE_CALCULATOR_H

#include "Graph.h"
#include <unordered_map>

class InfluenceCalculator {
public:
    static std::unordered_map<Graph::ID, double>
    computeInfluence(const Graph& graph, Graph::ID startID);
};

#endif