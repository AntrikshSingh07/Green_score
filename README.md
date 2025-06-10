# Green Score Project

## Overview
This project implements a system for calculating and analyzing "green scores" for companies based on their environmental impact. It models companies and their dependencies as a directed weighted graph, where:
- Nodes represent companies.
- Directed edges represent dependencies (e.g., company A supplies/influences company B).
- Edge weights indicate the strength or extent of this dependency/influence.

The system can:
1.  Calculate base and adjusted green scores for companies.
2.  Propagate the influence of a company's green score change throughout the network.
3.  Calculate the maximum "green score flow" between a designated source and sink company using the Ford-Fulkerson (Edmonds-Karp) algorithm.
4.  Visualize the company network, dependencies, and max flow paths.

## Core Concepts

### 1. Companies and Green Scores
- Each company is represented by the `Company` class (ID, name, sector).
- **Base Green Score**: Calculated from environmental metrics (e.g., emissions).
- **Adjusted Green Score**: Base score adjusted by sector-specific scaling factors.

### 2. Graph Representation
- The `Graph` class uses a directed graph (`std::unordered_map<ID, std::vector<std::pair<ID, double>>>`) to store companies and weighted dependencies.
- Supports ignoring specific edges to prevent cycles or unwanted influences.

### 3. Green Score Calculation
- `GreenScoreCalculator` computes scores based on metrics like factory emissions, employee carbon footprint, and vehicle emissions.
- Scores are normalized (-100 to 100). Higher scores are better.
- `ScalingFactor` class loads sector-specific multipliers from `scaling_factors.json`.

### 4. Influence Propagation
- `InfluenceCalculator` determines how a change in one company's score affects others.
- Uses a modified BFS; influence attenuates based on dependency weights. The strongest path of influence is considered if multiple paths exist.

### 5. Maximum Green Score Flow
- `MaxFlowCalculator` implements the Edmonds-Karp algorithm to find the maximum possible green score influence that can flow from a source to a sink company.
- It identifies all paths contributing to this max flow and their individual flow values.
- The results, including paths and flow values, are saved to `max_flow_results.json`.

### 6. Visualization
- `visualize.py` (Python script) uses `pandas`, `networkx`, and `matplotlib` for:
    - Visualizing the overall company network (companies color-coded by sector, edge thickness representing dependency strength).
    - Visualizing the maximum flow paths read from `max_flow_results.json`, highlighting the source, sink, and flow routes.

## File Structure

### Core C++ Files
- **`company.h/.cpp`**: `Company` class.
- **`graph.h/.cpp`**: `Graph` class for the dependency network.
- **`GreenScoreCalculator.h/.cpp`**: Calculates green scores.
- **`ScalingFactor.h/.cpp`**: Manages sector-specific scaling factors from `scaling_factors.json`.
- **`InfluenceCalculator.h/.cpp`**: Calculates influence propagation.
- **`MaxFlowCalculator.h/.cpp`**: Implements the Edmonds-Karp algorithm for max flow.
- **`main.cpp`**: Main application logic: loads data, builds graph, demonstrates calculations, and saves max flow results.

### Data Files
- **`companies.csv`**: Original dataset listing companies and their sectors.
- **`edges_weights.csv`**: Original dataset defining dependencies and weights.
- **`companies_small.csv`**: A smaller, sample dataset for companies and sectors. Used by default by `visualize.py`.
- **`edges_small.csv`**: A smaller, sample dataset for dependencies and weights. Used by default by `visualize.py`.
- **`sector_mapping_50.csv`**: A dataset with 50 companies and their sector mappings.
- **`edges_weights_50.csv`**: A dataset with dependencies and weights for the 50 companies.
- **`scaling_factors.json`**: Configuration for sector-specific scaling factors.
- **`max_flow_results.json`**: Output from the C++ application, containing source, sink, total max flow, and the paths contributing to it. Used by `visualize.py` for max flow visualization.

### Visualization & Execution
- **`visualize.py`**: Python script for network and max flow visualization.
- **`run.bat`**: Windows batch file to compile the C++ project (creating `green_score_analysis.exe`) and then run it.
- **`network_visualization.png` / `small_network_visualization.png`**: Example output image of the general network graph.
- **`max_flow_visualization.png` / `small_max_flow_visualization.png`**: Example output image of the max flow graph.

## Key Algorithms

### 1. Base Green Score Calculation
```cpp
double GreenScoreCalculator::computeBaseScore(const Company& company,
                                             double factoryEmissions,
                                             double employeeFootprint,
                                             double vehicleEmissions) {
    // Calculate score based on environmental metrics
    double score = 100.0;
    score -= (factoryEmissions / 10.0);
    score -= (employeeFootprint / 5.0);
    score -= (vehicleEmissions / 20.0);

    // Clamp score to defined range
    if (score < -100.0) return -100.0;
    if (score > 100.0) return 100.0;
    return score;
}
```

### 2. Influence Propagation Algorithm
The project uses a modified Breadth-First Search (BFS) algorithm to propagate influence through the network:

1. Start with the company whose score was changed
2. For each outgoing dependency, calculate propagated influence by multiplying the source influence by the dependency weight
3. For each affected company, keep the strongest influence (maximum)
4. Continue propagating influence until all reachable companies are processed

```cpp
while (!q.empty()) {
    Graph::ID u_id = q.front().first;
    double u_influence = q.front().second;
    q.pop();

    for (const auto& edge : adj.at(u_id)) {
        Graph::ID v_id = edge.first;
        double weight = edge.second;

        // Skip ignored edges
        if (graph.isEdgeIgnored(u_id, v_id)) continue;

        // Calculate propagated influence
        double propagatedInfluence = u_influence * weight;

        // Update influence value
        if (visited.find(v_id) == visited.end()) {
            visited.insert(v_id);
            influences[v_id] = propagatedInfluence;
            q.push({v_id, propagatedInfluence});
        } else if (propagatedInfluence > influences[v_id]) {
            // Take strongest influence path
            influences[v_id] = propagatedInfluence;
            q.push({v_id, propagatedInfluence});
        }
    }
}
```

### 3. Maximum Flow Algorithm (Edmonds-Karp)
The `MaxFlowCalculator` uses the Edmonds-Karp algorithm (a BFS-based Ford-Fulkerson method):
1.  The dependency graph is treated as a flow network. Edge capacities can be derived from their weights (representing influence potential).
2.  BFS is used to find the shortest augmenting path in the residual graph from source to sink.
3.  The bottleneck capacity of this path is determined.
4.  Flow is augmented along this path, and residual capacities are updated.
5.  Steps 2-4 are repeated until no more augmenting paths can be found.
6.  The sum of flows from all augmenting paths gives the maximum flow.
7.  The C++ application saves the source, sink, total max flow, and each contributing path with its flow value to `max_flow_results.json`.

```cpp
double MaxFlowCalculator::fordFulkerson(
    const std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>>& capacities,
    const std::unordered_map<Graph::ID, std::vector<Graph::ID>>& adjacencyList,
    Graph::ID sourceID,
    Graph::ID sinkID,
    std::unordered_map<Graph::ID, std::unordered_map<Graph::ID, double>>& flow,
    std::vector<FlowPath>& flowPaths) {
    
    // Initialize flows to 0
    for (const auto& [u, neighbors] : adjacencyList) {
        for (const auto& v : neighbors) {
            flow[u][v] = 0.0;
        }
    }
    
    double maxFlow = 0.0;
    std::unordered_map<Graph::ID, Graph::ID> parent;
    
    // While there is an augmenting path
    while (bfs(capacities, adjacencyList, flow, sourceID, sinkID, parent)) {
        // Find maximum flow on this path
        double pathFlow = std::numeric_limits<double>::max();
        
        // Store the path
        std::vector<Graph::ID> currentPath;
        currentPath.push_back(sinkID);
        
        // Find minimum capacity in the path
        for (Graph::ID v = sinkID; v != sourceID; v = parent[v]) {
            Graph::ID u = parent[v];
            pathFlow = std::min(pathFlow, capacities.at(u).at(v) - flow.at(u).at(v));
            currentPath.push_back(u);
        }
        
        std::reverse(currentPath.begin(), currentPath.end());
        
        // Store path and flow information
        FlowPath flowPath;
        flowPath.path = currentPath;
        flowPath.flowValue = pathFlow;
        flowPaths.push_back(flowPath);
        
        // Update residual capacities
        for (Graph::ID v = sinkID; v != sourceID; v = parent[v]) {
            Graph::ID u = parent[v];
            flow[u][v] += pathFlow;
            flow[v][u] -= pathFlow;  // For residual capacity
        }
        
        maxFlow += pathFlow;
    }
    
    return maxFlow;
}
```

### 4. JSON Parsing for Scaling Factors
The project includes a simple JSON parser to load sector-specific scaling factors:

```cpp
bool ScalingFactor::loadFromFile(const std::string& filepath) {
    // Read JSON file and parse key-value pairs
    // Store scaling factors in a map for lookup by sector
}
```

### 5. CSV Parsing for Company and Dependency Data
```cpp
std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }
    return fields;
}
```

## Design Patterns

### 1. Singleton-like Static Methods
The project uses static methods in `GreenScoreCalculator` and `InfluenceCalculator` classes to provide utility functions without requiring instance creation.

### 2. Factory Methods
Though not explicitly implemented as the Factory pattern, the project uses factory-like methods to create and configure objects.

### 3. Graph-based Data Structure
The project leverages graph theory concepts to model complex company relationships and dependencies.

## Extension Points

### 1. Enhanced Green Score Calculation
The current implementation uses a simple calculation based on three metrics. This could be extended to include:
- More sophisticated environmental metrics
- Industry-specific metrics
- Time-series data for tracking changes over time

### 2. Advanced Influence Models
The influence calculation could be enhanced to:
- Support different propagation models (e.g., sum, average, weighted sum)
- Account for bidirectional dependencies
- Model more complex interactions between companies

### 3. Cycle Detection
The current implementation relies on manual specification of ignored edges. This could be extended with automatic cycle detection.

### 4. Interactive Visualization
The visualization component could be enhanced to:
- Provide interactive exploration
- Highlight influence paths
- Allow what-if scenario modeling

## Usage Example

1. Define companies and their sectors in `companies.csv`
2. Define dependencies between companies in `edges_weights.csv`
3. Configure sector scaling factors in `scaling_factors.json`
4. Compile and run the project
5. View the network visualization using `visualize.py`

## Usage Examples

### Basic Usage
```cpp
// Load company data and build dependency graph
// ...
// Calculate influence from a company with toggled score
Company::ID companyID = 1; // Example ID
auto influences = InfluenceCalculator::computeInfluence(graph, companyID);
```

### Maximum Flow Analysis
```cpp
// Define source and sink companies
Graph::ID sourceID = companyNameToId["TRA_Co_1"]; // Source company
Graph::ID sinkID = companyNameToId["ENE_Co_5"];   // Sink company

// Calculate maximum green score flow
MaxFlowCalculator flowCalc;
std::vector<FlowPath> flowPaths = flowCalc.calculateMaxGreenScoreFlow(graph, sourceID, sinkID);

// Get total maximum flow
double maxFlow = flowCalc.getTotalMaxFlow();

// Access individual flow paths
for (const auto& path : flowPaths) {
    // path.flowValue contains the flow amount
    // path.path contains the company IDs in the path
}
```

### Visualization
```python
# Import modules
import pandas as pd
import networkx as nx
import matplotlib.pyplot as plt

# Load data
edges_df = pd.read_csv('edges_weights.csv')
companies_df = pd.read_csv('companies.csv')

# Create graph
G = nx.Graph()
# Add nodes and edges
# ...

# Visualize
nx.draw(G, with_labels=True)
plt.show()
```

## How to Run the Simulation

This project involves a C++ backend for calculations and a Python script for visualization.

### Prerequisites
1.  **C++ Compiler**: A C++ compiler that supports C++17 (like g++ or MSVC).
2.  **Python Environment**: Python 3.x installed.
3.  **Python Libraries**: Install required libraries using pip:
    ```bash
    pip install pandas networkx matplotlib
    ```

### Steps to Run

**1. Compile and Run the C++ Application:**
   - Open a terminal or command prompt.
   - Navigate to the project's root directory.
   - Execute the `run.bat` script:
     ```batch
     run.bat
     ```
   - This script will:
     - Compile all `.cpp` files (from `company.cpp`, `graph.cpp`, `GreenScoreCalculator.cpp`, `InfluenceCalculator.cpp`, `MaxFlowCalculator.cpp`, `ScalingFactor.cpp`, and `main.cpp`).
     - Create an executable named `green_score_analysis.exe`.
     - Run `green_score_analysis.exe`.
   - The C++ application will perform its calculations (including influence and max flow) and, if max flow is calculated, it will generate/update the `max_flow_results.json` file in the project directory. This file contains the source, sink, total max flow, and the specific paths with their flow values.

**2. Run the Python Visualization Script:**
   - After the C++ application has run (and `max_flow_results.json` is potentially created/updated), you can visualize the network and the max flow paths.
   - In your terminal, ensure you are still in the project's root directory.
   - Run the `visualize.py` script:
     ```bash
     python visualize.py
     ```
   - **Default Behavior**:
     - The script will load data from `edges_small.csv` and `companies_small.csv` by default to build and display the general company network graph. This will be saved as `small_network_visualization.png`.
     - It will then attempt to load `max_flow_results.json`. If found, it will visualize the maximum flow paths described in the JSON file, highlighting the source, sink, and flow routes. This visualization will be saved as `small_max_flow_visualization.png`.
   - **Using Other Datasets (e.g., the 50-company dataset):**
     - To use `edges_weights_50.csv` and `sector_mapping_50.csv`, you need to modify the `load_data` function call within `visualize.py` or change its default parameters:
       ```python
       # In visualize.py, inside the main() function or at the top for defaults:
       # Option 1: Modify defaults in load_data definition
       # def load_data(edges_file='edges_weights_50.csv', sector_file='sector_mapping_50.csv'):
       
       # Option 2: Modify the call in main()
       # edges_df, companies_df = load_data(edges_file='edges_weights_50.csv', sector_file='sector_mapping_50.csv')
       ```
     - Similarly, you can change the output filenames in `visualize_graph` and `visualize_max_flow` calls if you want to save images with different names (e.g., `network_visualization_50.png`).

### Expected Output
- **Console Output**: Both the C++ application and Python script will print status messages, calculation results, and analysis information to the console.
- **`max_flow_results.json`**: Generated by the C++ application, detailing the max flow calculation.
- **Image Files**:
    - `small_network_visualization.png` (or other specified name): Shows the general company dependency network.
    - `small_max_flow_visualization.png` (or other specified name): Shows the network with max flow paths highlighted if `max_flow_results.json` was processed.

## Visualization Details (`visualize.py`)

The `visualize.py` script provides two main types of visualizations:

1.  **General Network Graph (`visualize_graph` function):**
    *   Displays all companies as nodes and their dependencies as directed edges.
    *   Nodes are color-coded based on their sector.
    *   Edge thickness can represent the weight/strength of the dependency.
    *   Uses `networkx.DiGraph` to represent the directed nature of dependencies.

2.  **Maximum Flow Visualization (`visualize_max_flow` function):**
    *   Reads data from `max_flow_results.json` (generated by the C++ backend).
    *   Highlights the source and sink nodes.
    *   Draws the paths that contribute to the maximum flow, often with different colors or thicker lines to distinguish them.
    *   Other network edges might be displayed faintly in the background for context.

**Example Python Snippet (Conceptual from `visualize.py`):**
```python
import pandas as pd
import networkx as nx
import matplotlib.pyplot as plt
import json

def load_data(edges_file='edges_small.csv', sector_file='companies_small.csv'):
    # ... loads CSVs into pandas DataFrames ...
    edges_df = pd.read_csv(edges_file)
    companies_df = pd.read_csv(sector_file)
    # ... (handling for 'Unnamed: 0' and column name validation) ...
    return edges_df, companies_df

def create_graph(edges_df, companies_df):
    G = nx.DiGraph() # Use Directed Graph
    # ... (adds nodes with sector attributes and edges with weights) ...
    return G

def visualize_graph(G, output_file='network_visualization.png', title='Company Network'):
    # ... (uses nx.draw_networkx_nodes, nx.draw_networkx_edges, etc. to plot) ...
    # ... (color-codes nodes by sector, adjusts edge width by weight) ...
    plt.savefig(output_file)
    plt.show()

def visualize_max_flow(G, flow_data, output_file='max_flow_visualization.png'):
    source = flow_data['source']
    sink = flow_data['sink']
    paths = flow_data['paths']
    # ... (similar to visualize_graph but highlights flow paths, source, sink) ...
    # ... (edges in flow paths might be colored differently or be thicker) ...
    plt.savefig(output_file)
    plt.show()

def main():
    edges_df, companies_df = load_data() # Uses defaults: edges_small.csv, companies_small.csv
    G = create_graph(edges_df, companies_df)
    visualize_graph(G, output_file='small_network_visualization.png', title='Small Company Network')

    try:
        with open('max_flow_results.json', 'r') as f:
            flow_data = json.load(f)
        if flow_data:
            visualize_max_flow(G, flow_data, output_file='small_max_flow_visualization.png')
    except FileNotFoundError:
        print("max_flow_results.json not found. Run C++ simulation to generate it.")
    except Exception as e:
        print(f"Error processing max_flow_results.json: {e}")

if __name__ == "__main__":
    main()
```

## Conclusion
This project provides a framework for analyzing environmental scores and their influence within a corporate network. The max flow analysis, in particular, offers insights into the critical pathways of influence. The combination of C++ for computation and Python for visualization makes it a versatile tool for such analyses. Future enhancements could include more complex scoring, dynamic network analysis, and more interactive visualizations.
