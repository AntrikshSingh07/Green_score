#ifndef MAX_FLOW_CALCULATOR_H
#define MAX_FLOW_CALCULATOR_H

#include "Graph.h"
#include <vector>
#include <unordered_map>
#include <limits>
#include <string>
#include <fstream>   // For writeResultsToJson
#include <iomanip>   // For std::fixed, std::setprecision
#include <algorithm> // For std::reverse, std::min
#include <queue>     // For BFS
#include <set>       // For visited set in BFS (can use unordered_set for potential perf improvement)
#include <stdexcept> // For std::runtime_error

struct FlowPath {
    std::vector<Graph::ID> path; // Stores company IDs
    double flowValue;
};

class MaxFlowCalculator {
public:
    MaxFlowCalculator(); // Constructor

    // Calculate the maximum flow from source to sink
    // Populates internal members: totalMaxFlow_, flowPaths_, sourceID_, sinkID_
    void calculateMaxGreenScoreFlow(
        const Graph& graph,
        Graph::ID sourceID,
        Graph::ID sinkID);

    // Get the total maximum flow value
    double getTotalMaxFlow() const { return totalMaxFlow_; }

    // Get the paths contributing to the max flow
    const std::vector<FlowPath>& getFlowPaths() const { return flowPaths_; }

    // Write the results to a JSON file
    void persistResults(const Graph& graph, const std::string& filename = "max_flow_results.json") const;

private:
    // Ford-Fulkerson/Edmonds-Karp algorithm implementation
    void fordFulkerson(
        const Graph& graph, // For accessing all nodes if needed by BFS
        const std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>>& capacities,
        const std::vector<Graph::ID>& allNodesInNetwork); // All nodes involved in flow

    // Helper for BFS to find augmenting paths in the residual graph
    bool bfs(
        const std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>>& capacities,
        const std::vector<Graph::ID>& allNodesInNetwork, // All nodes for iteration
        std::unordered_map<Graph::ID, Graph::ID>& parent) const;

    // Convert the weighted Graph to a flow network, populates capacities and allNodesInNetwork
    void prepareFlowNetwork(
        const Graph& graph,
        std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>>& capacities,
        std::vector<Graph::ID>& allNodesInNetwork);

    // Helper to write results to JSON
    void writeResultsToJson(
        const Graph& graph,
        const std::string& filename) const;

    // Helper to get company name from ID
    std::string getCompanyNameFromId(Graph::ID id, const Graph& graph) const;

    Graph::ID sourceID_;
    Graph::ID sinkID_;
    double totalMaxFlow_;
    std::vector<FlowPath> flowPaths_;
    
    // Represents the current flow on edge (u,v). flow_[u][v] = -flow_[v][u]
    std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>> flow_; 
};

#endif // MAX_FLOW_CALCULATOR_H
