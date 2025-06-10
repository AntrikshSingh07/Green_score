#include <iostream>
#include <fstream>      // For std::ifstream
#include <sstream>      // For std::istringstream
#include <vector>       // For std::vector
#include <map>          // For std::map (to map company names to IDs)
#include "Company.h"
#include "ScalingFactor.h"
#include "GreenScoreCalculator.h"
#include "Graph.h"
#include "InfluenceCalculator.h"
#include "MaxFlowCalculator.h"
#include <iomanip> // For std::setprecision

// Helper function to parse a CSV string into a vector of strings
std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }
    return fields;
}

int main() {
    ScalingFactor sf;
    // TODO: Ensure scaling_factors.json exists and is correctly formatted.
    // Example format for scaling_factors.json:
    // Transportation 1.1
    // Agriculture 0.9
    // IT 1.0
    if (!sf.loadFromFile("scaling_factors.json")) {
        std::cerr << "Failed to load scaling_factors.json. Please create it with sector-factor pairs per line.\n";
        // return EXIT_FAILURE; // Decide if this is a fatal error
    }

    Graph graph;
    std::map<std::string, Company::ID> companyNameToId;
    Company::ID nextCompanyID = 1;

    // Load Companies from companies.csv
    std::ifstream companiesFile("companies.csv");
    std::string line;
    if (companiesFile.is_open()) {
        std::getline(companiesFile, line); // Skip header row
        while (std::getline(companiesFile, line)) {
            if (line.empty() || line.rfind("//", 0) == 0) continue; // Skip empty or comment lines
            std::vector<std::string> fields = parseCSVLine(line);
            if (fields.size() >= 2) {
                std::string companyName = fields[0];
                std::string sector = fields[1];
                Company::ID currentID = nextCompanyID++;
                graph.addCompany(Company(currentID, companyName, sector, 0.0)); // Base score 0.0 initially
                companyNameToId[companyName] = currentID;
                // std::cout << "Loaded Company: " << companyName << " with ID: " << currentID << " and Sector: " << sector << std::endl;
            }
        }
        companiesFile.close();
    } else {
        std::cerr << "Failed to open companies.csv\n";
        return EXIT_FAILURE;
    }

    // Calculate and set base and adjusted green scores for each company
    // This part requires actual metrics for each company to be meaningful.
    // For now, we'll use placeholder metrics as before, but iterate through loaded companies.
    for (auto const& [name, id] : companyNameToId) {
        Company* c = graph.getCompany(id);
        if (c) {
            // Placeholder metrics for base score calculation. 
            // In a real scenario, these would come from a data source related to the company.
            double factoryEmissions = 50.0 + (rand() % 100); // Example random data
            double employeeFootprint = 20.0 + (rand() % 50); // Example random data
            double vehicleEmissions = 10.0 + (rand() % 40);  // Example random data

            double base = GreenScoreCalculator::computeBaseScore(*c, factoryEmissions, employeeFootprint, vehicleEmissions);
            double scaled = GreenScoreCalculator::applyScaling(base, sf.getFactor(c->getSector()));
            c->setBaseGreenScore(base);
            c->setAdjustedGreenScore(scaled);
            // std::cout << "Company: " << c->getName() << ", Base: " << base << ", Scaled: " << scaled << ", Sector: " << c->getSector() << ", Factor: " << sf.getFactor(c->getSector()) << std::endl;
        }
    }

    // Load Dependencies from edges_weights.csv
    std::ifstream edgesFile("edges_weights.csv");
    if (edgesFile.is_open()) {
        std::getline(edgesFile, line); // Skip header row
        while (std::getline(edgesFile, line)) {
            if (line.empty() || line.rfind("//", 0) == 0) continue; // Skip empty or comment lines
            std::vector<std::string> fields = parseCSVLine(line);
            if (fields.size() >= 3) {
                std::string fromName = fields[0];
                std::string toName = fields[1];
                double weight = 0.0;
                try {
                    weight = std::stod(fields[2]);
                } catch (const std::invalid_argument& ia) {
                    std::cerr << "Invalid weight in edges_weights.csv for line: " << line << std::endl;
                    continue;
                } catch (const std::out_of_range& oor) {
                    std::cerr << "Weight out of range in edges_weights.csv for line: " << line << std::endl;
                    continue;
                }

                if (companyNameToId.count(fromName) && companyNameToId.count(toName)) {
                    graph.addDependency(companyNameToId[fromName], companyNameToId[toName], weight);
                    // std::cout << "Loaded Dependency: " << fromName << " -> " << toName << " with Weight: " << weight << std::endl;
                } else {
                    // std::cerr << "Warning: Company name not found for dependency: " << fromName << " or " << toName << std::endl;
                }
            }
        }
        edgesFile.close();
    } else {
        std::cerr << "Failed to open edges_weights.csv\n";
        return EXIT_FAILURE;
    }

    // Example: Define dependencies and handle collisions (if any, based on your logic)
    // graph.addDependency(companyNameToId["TRA_Co_1"], companyNameToId["ENE_Co_1"], 0.5);
    // graph.ignoreEdge(companyNameToId["ENE_Co_1"], companyNameToId["TRA_Co_1"]); // Example ignore

    // Toggle: update a company's adjusted green score (e.g., TRA_Co_1)
    if (companyNameToId.count("TRA_Co_1")) {
        Company::ID companyToToggleID = companyNameToId["TRA_Co_1"];
        Company* compToToggle = graph.getCompany(companyToToggleID);
        if (compToToggle) {
            double newAdjustedScore = 75.0; // Example new score
            std::cout << "\nToggling " << compToToggle->getName() << " (ID: " << companyToToggleID << ") adjusted score to: " << newAdjustedScore << std::endl;
            compToToggle->setAdjustedGreenScore(newAdjustedScore);

            // Propagate influence from the toggled company
            std::cout << "Calculating influence from " << compToToggle->getName() << ":\n";
            auto influences = InfluenceCalculator::computeInfluence(graph, companyToToggleID);
            if (influences.empty()) {
                std::cout << "  No direct influences calculated or start node had no outgoing paths/valid score.\n";
            }
            for (const auto& [influencedCompanyID, influenceValue] : influences) {
                Company* influencedComp = graph.getCompany(influencedCompanyID);
                if (influencedComp) {
                    std::cout << "  Company '" << influencedComp->getName() << "' (ID: " << influencedCompanyID 
                              << ") is influenced to an extent of: " << influenceValue << std::endl;
                }
            }
        } else {
            std::cerr << "Could not find TRA_Co_1 to toggle its score." << std::endl;
        }
    } else {
        std::cout << "TRA_Co_1 not found in the loaded companies. Cannot run toggle example.\n";
    }

    // Demonstrate the maximum flow calculation between two companies
    std::cout << "\\n\\n--- Maximum Green Score Flow Demonstration ---\\n";
    
    // Choose two companies for the flow calculation
    std::string sourceCompanyName = "ENE_Co_1"; 
    std::string sinkCompanyName = "RET_Co_3";   
    
    // Check if both companies exist
    if (companyNameToId.count(sourceCompanyName) && companyNameToId.count(sinkCompanyName)) {
        Graph::ID sourceID = companyNameToId[sourceCompanyName];
        Graph::ID sinkID = companyNameToId[sinkCompanyName];
        
        std::cout << "Calculating maximum green score flow from '" << sourceCompanyName 
                  << "' (" << sourceID << ") to '" << sinkCompanyName << "' (" << sinkID << ")\\n";
        
        try {
            MaxFlowCalculator flowCalc;
            flowCalc.calculateMaxGreenScoreFlow(graph, sourceID, sinkID);
            
            double maxFlow = flowCalc.getTotalMaxFlow();
            const auto& paths = flowCalc.getFlowPaths();
            
            std::cout << "Total Maximum Green Score Flow: " << std::fixed << std::setprecision(5) << maxFlow << std::endl;
            
            std::cout << "Paths contributing to the max flow:" << std::endl;
            if (paths.empty() && maxFlow > 1e-9) {
                 std::cout << "  Note: Max flow is positive, but no distinct paths were recorded (possibly direct source-sink or complex scenario not fully decomposed by current path tracking)." << std::endl;
            } else if (paths.empty()) {
                 std::cout << "  No paths found contributing to the flow." << std::endl;
            }
            for (const auto& p : paths) {
                std::cout << "  Flow: " << std::fixed << std::setprecision(5) << p.flowValue << ", Path: ";
                for (size_t i = 0; i < p.path.size(); ++i) {
                    // To get company name from ID for display, we'd ideally use a reverse map or query graph
                    // For now, just printing IDs. The JSON output will have names.
                    const Company* c = graph.getCompany(p.path[i]);
                    if (c) {
                        std::cout << c->getName();
                    } else {
                        std::cout << "ID_" << p.path[i];
                    }
                    if (i < p.path.size() - 1) std::cout << " -> ";
                }
                std::cout << std::endl;
            }

            // Write results to JSON
            flowCalc.persistResults(graph, "max_flow_results.json");
            std::cout << "Max flow results written to max_flow_results.json" << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error in maximum flow calculation: " << e.what() << std::endl;
        }
    } else {
        std::cout << "Source or sink company not found. Cannot run maximum flow example.\n";
    }

    return 0;
}