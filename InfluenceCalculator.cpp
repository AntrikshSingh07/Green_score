// File: InfluenceCalculator.cpp
#include "InfluenceCalculator.h"
#include "Company.h" // For Company::ID and accessing company scores
#include <queue>      // For BFS-like traversal
#include <unordered_set> // To keep track of visited nodes
#include <iostream>   // For potential debug output

std::unordered_map<Graph::ID, double>
InfluenceCalculator::computeInfluence(const Graph& graph, Graph::ID startNodeID) {
    std::unordered_map<Graph::ID, double> influences; // Stores companyID -> influence value

    const Company* startCompany = graph.getCompany(startNodeID);
    if (!startCompany) {
        // std::cerr << "Error: Start company with ID " << startNodeID << " not found.\n";
        return influences; // Return empty map if start company doesn't exist
    }

    // The initial "influence" is the (adjusted) green score of the start company.
    // Or it could be a *change* in score if the model is about deltas.
    // For now, let's use its current adjusted score.
    double initialInfluence = startCompany->getAdjustedGreenScore();

    std::queue<std::pair<Graph::ID, double>> q; // Stores {companyID, currentInfluenceStrength}
    std::unordered_set<Graph::ID> visited;

    q.push({startNodeID, initialInfluence});
    visited.insert(startNodeID);
    influences[startNodeID] = initialInfluence; // A company influences itself fully

    while (!q.empty()) {
        Graph::ID u_id = q.front().first;
        double u_influence = q.front().second;
        q.pop();

        const auto& adj = graph.getAdjacency();
        if (adj.count(u_id)) { // Check if u_id has outgoing dependencies
            for (const auto& edge : adj.at(u_id)) {
                Graph::ID v_id = edge.first;
                double weight = edge.second; // Dependency weight

                if (graph.isEdgeIgnored(u_id, v_id)) {
                    continue; // Skip ignored edges
                }

                double propagatedInfluence = u_influence * weight;                // For each company, we'll take the maximum influence from any incoming path
                // This represents the strongest dependency relationship
                if (visited.find(v_id) == visited.end()) {
                    visited.insert(v_id);
                    influences[v_id] = propagatedInfluence;
                    q.push({v_id, propagatedInfluence});
                } else {
                    // Take the strongest influence (maximum)
                    // This assumes that what matters is the strongest dependency, not the sum
                    if (propagatedInfluence > influences[v_id]) {
                        influences[v_id] = propagatedInfluence;
                        // Re-queue with the stronger influence
                        q.push({v_id, propagatedInfluence});
                    }
                }
            }
        }
    }

    return influences;
}