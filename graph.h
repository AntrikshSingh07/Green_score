// File: Graph.h
#ifndef GRAPH_H
#define GRAPH_H

#include "Company.h"
#include <unordered_map>
#include <vector>
#include <utility>
#include <unordered_set>

struct EdgePairHash {
    size_t operator()(const std::pair<Company::ID, Company::ID>& p) const noexcept {
        return std::hash<long long>()(
            (static_cast<long long>(p.first) << 32) | static_cast<unsigned long long>(p.second));
    }
};

class Graph {
public:
    using ID = Company::ID;

private:
    std::unordered_map<ID, Company> companies;
    std::unordered_map<ID, std::vector<std::pair<ID, double>>> adj;
    std::unordered_set<std::pair<ID,ID>, EdgePairHash> ignoredEdges;

public:
    void addCompany(const Company& company);
    void addDependency(ID from, ID to, double weight);
    void ignoreEdge(ID from, ID to);
    Company* getCompany(ID id);    const Company* getCompany(ID id) const;
    const std::unordered_map<ID, std::vector<std::pair<ID, double>>>& getAdjacency() const;
    const std::unordered_set<std::pair<ID,ID>, EdgePairHash>& getIgnoredEdges() const;
    bool isEdgeIgnored(ID from, ID to) const;
};

#endif 