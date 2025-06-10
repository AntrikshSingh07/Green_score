#include "MaxFlowCalculator.h"
#include <iostream> // For cerr
#include <set>      // For collecting unique nodes
#include <algorithm> // Required for std::reverse
#include <vector> // Required for std::vector
#include <limits> // Required for std::numeric_limits

MaxFlowCalculator::MaxFlowCalculator() 
    : sourceID_(0), sinkID_(0), totalMaxFlow_(0.0) {}

std::string MaxFlowCalculator::getCompanyNameFromId(Graph::ID id, const Graph& graph) const {
    const Company* company = graph.getCompany(id);
    if (company) {
        return company->getName();
    }
    return "ID_" + std::to_string(id); // Fallback if name not found
}

void MaxFlowCalculator::prepareFlowNetwork(
    const Graph& graph,
    std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>>& capacities,
    std::vector<Graph::ID>& allNodesInNetwork) {
    
    capacities.clear();
    allNodesInNetwork.clear();
    std::set<Graph::ID> nodeSet; // To collect unique nodes

    const Company* sourceCompany = graph.getCompany(sourceID_);
    if (!sourceCompany) {
        throw std::runtime_error("Source company ID not found in graph for prepareFlowNetwork.");
    }
    double sourceGreenScore = sourceCompany->getAdjustedGreenScore();
    if (sourceGreenScore <= 1e-9) { // Using a small epsilon for zero or negative
        // std::cerr << "Warning: Source company green score is " << sourceGreenScore 
        //           << ". Using a default scaling factor of 0.1 for capacities." << std::endl;
        sourceGreenScore = 0.1; 
    }

    const auto& adjMap = graph.getAdjacency();
    for (const auto& pair_u : adjMap) {
        Graph::ID u = pair_u.first;
        nodeSet.insert(u);
        for (const auto& edge : pair_u.second) {
            Graph::ID v = edge.first;
            double weight = edge.second;
            nodeSet.insert(v);

            if (graph.isEdgeIgnored(u, v)) continue;
            if (v == sourceID_) continue; // No edges into source
            if (u == sinkID_) continue;   // No edges out of sink
            // Also, typically, no edges out of source directly to sink if there are intermediate paths,
            // but the problem doesn't state this, so we allow u=sourceID and v=sinkID if not caught by above.
            // However, the primary filtering is no flow *into* source and no flow *out of* sink.

            capacities[u][v] = sourceGreenScore * weight;
        }
    }
    // Ensure source and sink are in the node set, even if they have no outgoing/incoming edges after filtering
    nodeSet.insert(sourceID_);
    nodeSet.insert(sinkID_);
    
    allNodesInNetwork.assign(nodeSet.begin(), nodeSet.end());
}

bool MaxFlowCalculator::bfs(
    const std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>>& capacities,
    const std::vector<Graph::ID>& allNodesInNetwork,
    std::unordered_map<Graph::ID, Graph::ID>& parent) const {
    
    parent.clear();
    std::queue<Graph::ID> q;
    std::unordered_set<Graph::ID> visited; // Changed from std::set for consistency with typical BFS

    q.push(sourceID_);
    visited.insert(sourceID_);
    parent[sourceID_] = sourceID_; // Mark source as its own parent or use a sentinel

    bool sinkFound = false;
    while (!q.empty() && !sinkFound) {
        Graph::ID u = q.front();
        q.pop();

        for (Graph::ID v : allNodesInNetwork) {
            if (visited.count(v)) {
                continue;
            }

            // Check forward edge u -> v
            double cap_uv = 0.0;
            if (capacities.count(u) && capacities.at(u).count(v)) {
                cap_uv = capacities.at(u).at(v);
            }
            double flow_uv = 0.0;
            if (flow_.count(u) && flow_.at(u).count(v)) {
                flow_uv = flow_.at(u).at(v);
            }
            if (cap_uv - flow_uv > 1e-9) { // If residual capacity u->v > 0
                parent[v] = u;
                visited.insert(v);
                q.push(v);
                if (v == sinkID_) {
                    sinkFound = true;
                    break;
                }
                continue; // Found path to v via forward edge
            }

            // Check backward edge v -> u (residual capacity is flow_vu, allowing path u -> v)
            // This means there was flow from v to u, that we can "push back"
            double flow_vu = 0.0;
            if (flow_.count(v) && flow_.at(v).count(u)) {
                 flow_vu = flow_.at(v).at(u);
            }
            // We need to ensure v->u was a valid edge in the original capacity graph for flow_vu to be meaningful
            // in the context of capacities (though flow_ map handles this implicitly by skew symmetry if updated correctly)
            // The condition is simply if flow_vu > 0, it means we can send flow from u to v via this "reverse" mechanism.
            if (flow_vu > 1e-9) { // If residual capacity u->v (via reverse of v->u) > 0
                parent[v] = u;
                visited.insert(v);
                q.push(v);
                if (v == sinkID_) {
                    sinkFound = true;
                    break;
                }
            }
        } // end for v
    } // end while
    return sinkFound;
}

void MaxFlowCalculator::fordFulkerson(
    const Graph& graph, // graph parameter is not strictly needed here if allNodesInNetwork is comprehensive
    const std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>>& capacities,
    const std::vector<Graph::ID>& allNodesInNetwork) {
    
    flow_.clear(); // Initialize all flows to 0 implicitly
    totalMaxFlow_ = 0.0;
    flowPaths_.clear();

    std::unordered_map<Graph::ID, Graph::ID> parentPath;
    while (bfs(capacities, allNodesInNetwork, parentPath)) {
        double pathFlow = std::numeric_limits<double>::max();
        std::vector<Graph::ID> currentPathSegment;

        // Find bottleneck capacity (pathFlow)
        for (Graph::ID v_curr = sinkID_; v_curr != sourceID_; v_curr = parentPath.at(v_curr)) {
            Graph::ID u_prev = parentPath.at(v_curr);
            currentPathSegment.push_back(v_curr); // Build path segment in reverse

            double residual_capacity_edge = 0;
            // Check if (u_prev, v_curr) was used as a forward edge
            double cap_uv = 0.0;
            if (capacities.count(u_prev) && capacities.at(u_prev).count(v_curr)) {
                cap_uv = capacities.at(u_prev).at(v_curr);
            }
            double flow_uv = 0.0;
            if (flow_.count(u_prev) && flow_.at(u_prev).count(v_curr)) {
                flow_uv = flow_.at(u_prev).at(v_curr);
            }

            if (cap_uv - flow_uv > 1e-9) { // Edge u_prev -> v_curr used forward
                 residual_capacity_edge = cap_uv - flow_uv;
            } else { // Edge u_prev -> v_curr must have been used backward (original v_curr -> u_prev)
                double flow_vu = 0.0; // Flow on original v_curr -> u_prev
                if (flow_.count(v_curr) && flow_.at(v_curr).count(u_prev)) {
                    flow_vu = flow_.at(v_curr).at(u_prev);
                }
                residual_capacity_edge = flow_vu;
            }
            pathFlow = std::min(pathFlow, residual_capacity_edge);
        }
        currentPathSegment.push_back(sourceID_);
        std::reverse(currentPathSegment.begin(), currentPathSegment.end());

        // Augment flow and update residual capacities
        for (Graph::ID v_curr = sinkID_; v_curr != sourceID_; v_curr = parentPath.at(v_curr)) {
            Graph::ID u_prev = parentPath.at(v_curr);
            
            // Add pathFlow to flow_[u_prev][v_curr]
            if (!flow_.count(u_prev)) flow_[u_prev] = {};
            flow_[u_prev][v_curr] += pathFlow;

            // Subtract pathFlow from flow_[v_curr][u_prev] (skew symmetry)
            if (!flow_.count(v_curr)) flow_[v_curr] = {};
            flow_[v_curr][u_prev] -= pathFlow;
        }
        
        totalMaxFlow_ += pathFlow;
        if (pathFlow > 1e-9) { // Only add paths that carry flow
             flowPaths_.push_back({currentPathSegment, pathFlow});
        }
    }
}

void MaxFlowCalculator::calculateMaxGreenScoreFlow(
    const Graph& graph,
    Graph::ID sourceID,
    Graph::ID sinkID) {
    
    sourceID_ = sourceID;
    sinkID_ = sinkID;

    if (sourceID_ == sinkID_) {
        totalMaxFlow_ = 0.0;
        flowPaths_.clear();
        // std::cerr << "Warning: Source and Sink are the same. Max flow is 0." << std::endl;
        return;
    }
    
    std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>> capacities;
    std::vector<Graph::ID> allNodesInNetwork;

    try {
        prepareFlowNetwork(graph, capacities, allNodesInNetwork);
    } catch (const std::runtime_error& e) {
        std::cerr << "Error in prepareFlowNetwork: " << e.what() << std::endl;
        totalMaxFlow_ = 0;
        flowPaths_.clear();
        return;
    }
    
    if (allNodesInNetwork.empty() || capacities.empty()) {
         // std::cerr << "Warning: Flow network is empty after preparation. Max flow is 0." << std::endl;
         totalMaxFlow_ = 0.0;
         flowPaths_.clear();
         return;
    }
    
    // Check if source and sink are part of the prepared network
    bool sourceInNetwork = false;
    bool sinkInNetwork = false;
    for(Graph::ID node : allNodesInNetwork) {
        if(node == sourceID_) sourceInNetwork = true;
        if(node == sinkID_) sinkInNetwork = true;
    }
    if(!sourceInNetwork || !sinkInNetwork) {
        // std::cerr << "Warning: Source or Sink not in the prepared flow network. Max flow is 0." << std::endl;
        totalMaxFlow_ = 0.0;
        flowPaths_.clear();
        return;
    }


    fordFulkerson(graph, capacities, allNodesInNetwork);
}

void MaxFlowCalculator::writeResultsToJson(const Graph& graph, const std::string& filename) const {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing max flow results." << std::endl;
        return;
    }

    outFile << "{\n";
    outFile << "  \"source\": \"" << getCompanyNameFromId(sourceID_, graph) << "\",\n";
    outFile << "  \"sink\": \"" << getCompanyNameFromId(sinkID_, graph) << "\",\n";
    outFile << "  \"max_flow\": " << std::fixed << std::setprecision(5) << totalMaxFlow_ << ",\n";
    outFile << "  \"paths\": [\n";

    for (size_t i = 0; i < flowPaths_.size(); ++i) {
        const auto& flowPath = flowPaths_[i];
        outFile << "    {\n";
        outFile << "      \"path\": [";
        for (size_t j = 0; j < flowPath.path.size(); ++j) {
            outFile << "\"" << getCompanyNameFromId(flowPath.path[j], graph) << "\"";
            if (j < flowPath.path.size() - 1) {
                outFile << ", ";
            }
        }
        outFile << "],\n";
        outFile << "      \"flow\": " << std::fixed << std::setprecision(5) << flowPath.flowValue << "\n";
        outFile << "    }";
        if (i < flowPaths_.size() - 1) {
            outFile << ",\n";
        } else {
            outFile << "\n";
        }
    }
    outFile << "  ]\n";
    outFile << "}\n";
    outFile.close();
}

void MaxFlowCalculator::persistResults(const Graph& graph, const std::string& filename) const {
    writeResultsToJson(graph, filename);
}
