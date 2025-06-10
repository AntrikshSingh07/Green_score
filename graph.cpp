#include "Graph.h"
#include <utility> // for std::pair
#include <functional> // for std::hash

void Graph::addCompany(const Company& company) {
    companies.emplace(company.getID(), company);
}

void Graph::addDependency(Company::ID from, Company::ID to, double weight) {
    adj[from].push_back({to, weight});
}

void Graph::ignoreEdge(Company::ID from, Company::ID to) {
    ignoredEdges.insert({from, to});
}

Company* Graph::getCompany(Company::ID id) {
    auto it = companies.find(id);
    return (it != companies.end()) ? &it->second : nullptr;
}

const Company* Graph::getCompany(Company::ID id) const {
    auto it = companies.find(id);
    return (it != companies.end()) ? &it->second : nullptr;
}

const std::unordered_map<Graph::ID, std::vector<std::pair<Graph::ID, double>>>&
Graph::getAdjacency() const {
    return adj;
}

const std::unordered_set<std::pair<Graph::ID, Graph::ID>, EdgePairHash>&
Graph::getIgnoredEdges() const {
    return ignoredEdges;
}

bool Graph::isEdgeIgnored(Company::ID from, Company::ID to) const {
    return ignoredEdges.count({from, to});
}