import pandas as pd
import networkx as nx
import matplotlib.pyplot as plt
import json  # Ensure json is imported

def load_data(edges_file='edges_small.csv', sector_file='companies_small.csv'):
    try:
        # Load the edges with weights
        edges_df = pd.read_csv(edges_file)
        print(f"Successfully loaded {edges_file} with {len(edges_df)} rows")
        print(f"Columns in {edges_file}: {edges_df.columns.tolist()}")
        
        # Load the companies data
        companies_df = pd.read_csv(sector_file)
        print(f"Successfully loaded {sector_file} with {len(companies_df)} rows")
        print(f"Columns in {sector_file}: {companies_df.columns.tolist()}")
        
        # Handle index column if it exists
        if '' in companies_df.columns or 'Unnamed: 0' in companies_df.columns:
            if '' in companies_df.columns:
                companies_df = companies_df.drop('', axis=1)
            if 'Unnamed: 0' in companies_df.columns:
                companies_df = companies_df.drop('Unnamed: 0', axis=1)
                
        # Validate that required columns exist
        if 'Company' not in companies_df.columns:
            print(f"Warning: 'Company' column not found in {sector_file}")
            # If not found, try to use the first column as the company name
            companies_df = companies_df.rename(columns={companies_df.columns[0]: 'Company'})
            print("Using first column as 'Company' instead")
            
        if 'Sector' not in companies_df.columns:
            print(f"Warning: 'Sector' column not found in {sector_file}")
            if len(companies_df.columns) > 1:
                companies_df = companies_df.rename(columns={companies_df.columns[1]: 'Sector'})
                print("Using second column as 'Sector' instead")
            else:
                # If no sector column available, create a default one
                companies_df['Sector'] = 'Unknown'
        
        required_edge_cols = ['Company1', 'Company2', 'Weight']
        for i, col_name in enumerate(required_edge_cols):
            if col_name not in edges_df.columns:
                print(f"Warning: '{col_name}' column not found in {edges_file}")
                if i < len(edges_df.columns):
                    edges_df = edges_df.rename(columns={edges_df.columns[i]: col_name})
                    print(f"Using column {i+1} as '{col_name}' instead")
        
        # Ensure weight is numeric
        try:
            edges_df['Weight'] = pd.to_numeric(edges_df['Weight'])
        except:
            print("Warning: Some weights are not numeric. Converting to numeric where possible.")
            edges_df['Weight'] = pd.to_numeric(edges_df['Weight'], errors='coerce')
            # Fill NaN values with a default weight
            edges_df['Weight'].fillna(0.01, inplace=True)
        
        return edges_df, companies_df
    
    except Exception as e:
        print(f"Error loading data: {e}")
        raise

def create_graph(edges_df, companies_df):
    # Create a new graph
    G = nx.DiGraph()  # Using directed graph to match the flow analysis
    
    # Add nodes (companies)
    for _, company in companies_df.iterrows():
        # Use the company name as the node ID
        company_name = company['Company']
        G.add_node(company_name, sector=company['Sector'])
    
    # Add edges with weights
    for _, edge in edges_df.iterrows():
        # Use the actual column names
        G.add_edge(edge['Company1'], edge['Company2'], weight=float(edge['Weight']))
    
    return G

def visualize_graph(G, output_file='network_visualization.png', title='Company Relationship Network'):
    # Create a layout for the nodes
    layout = nx.spring_layout(G, seed=42)
    
    # Get edge weights for line thickness
    edge_weights = [G[u][v]['weight'] for u, v in G.edges()]
    
    # Normalize edge weights for better visualization
    if edge_weights:  # Check if the list is not empty
        max_weight = max(edge_weights)
        if max_weight > 0:  # Check if max_weight is not zero
            edge_weights = [3 * w / max_weight for w in edge_weights]
        else:
            edge_weights = [1.0 for _ in edge_weights]
    else:
        edge_weights = []
    
    # Create a mapping of sectors to colors
    sectors = set(nx.get_node_attributes(G, 'sector').values())
    sector_colors = {}
    color_map = plt.cm.get_cmap('tab20', len(sectors))
    
    for i, sector in enumerate(sectors):
        sector_colors[sector] = color_map(i)
    
    # Get node colors based on sector
    node_colors = [sector_colors[G.nodes[node]['sector']] for node in G.nodes()]
    
    # Draw the graph
    plt.figure(figsize=(14, 12))
    
    # Draw nodes with colors based on sector
    nx.draw_networkx_nodes(G, layout, node_color=node_colors, node_size=300, alpha=0.8)
    
    # Draw edges with weights as width
    nx.draw_networkx_edges(G, layout, width=edge_weights, edge_color='gray', alpha=0.5, 
                          arrowsize=10, connectionstyle='arc3,rad=0.1')
    
    # Draw labels (make them smaller and more readable)
    nx.draw_networkx_labels(G, layout, font_size=8, font_family='sans-serif')
    
    # Add a legend for sectors
    legend_patches = [plt.Line2D([0], [0], marker='o', color='w', 
                               markerfacecolor=sector_colors[sector], 
                               markersize=10, label=sector) 
                    for sector in sectors]
    
    plt.legend(handles=legend_patches, title="Sectors", loc='best', fontsize='small')
    
    plt.title(title)
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(output_file, dpi=300)
    plt.show()

def visualize_max_flow(G, flow_paths, source, sink, output_file='max_flow_visualization.png'):
    """
    Visualize the maximum flow paths in the graph
    
    Parameters:
    G (networkx.DiGraph): The graph
    flow_paths (list): List of path dictionaries with 'path' and 'flow' keys
    source (str): Source node ID
    sink (str): Sink node ID
    output_file (str): Output file name
    """
    print(f"--- Debug: visualize_max_flow for {source} -> {sink} ---")
    print(f"Graph G has {G.number_of_nodes()} nodes and {G.number_of_edges()} edges.")

    if source not in G:
        print(f"ERROR: Source node '{source}' is not in the graph G. Cannot visualize max flow.")
        # Optionally, draw the graph without source/sink highlights or return
        plt.figure(figsize=(14, 12))
        nx.draw_networkx_nodes(G, nx.spring_layout(G, seed=42), node_size=300, alpha=0.8)
        nx.draw_networkx_edges(G, nx.spring_layout(G, seed=42), width=0.5, edge_color='lightgray', alpha=0.3)
        nx.draw_networkx_labels(G, nx.spring_layout(G, seed=42), font_size=8)
        plt.title(f"Graph (Source '{source}' not found)")
        plt.savefig(output_file, dpi=300)
        plt.show()
        return
    if sink not in G:
        print(f"ERROR: Sink node '{sink}' is not in the graph G. Cannot visualize max flow.")
        # Optionally, draw the graph without source/sink highlights or return
        plt.figure(figsize=(14, 12))
        nx.draw_networkx_nodes(G, nx.spring_layout(G, seed=42), node_size=300, alpha=0.8)
        nx.draw_networkx_edges(G, nx.spring_layout(G, seed=42), width=0.5, edge_color='lightgray', alpha=0.3)
        nx.draw_networkx_labels(G, nx.spring_layout(G, seed=42), font_size=8)
        plt.title(f"Graph (Sink '{sink}' not found)")
        plt.savefig(output_file, dpi=300)
        plt.show()
        return
    
    print(f"Source node '{source}' and Sink node '{sink}' are present in G.")
    print(f"Received {len(flow_paths)} flow paths from JSON.")

    # Create a layout for the nodes
    layout = nx.spring_layout(G, seed=42)
    
    # Create a mapping of sectors to colors
    sectors = set(nx.get_node_attributes(G, 'sector').values())
    sector_colors = {}
    # Ensure color_map is generated even if sectors is empty, though unlikely for a populated graph
    cmap_name = 'tab20' if len(sectors) <= 20 else 'viridis' # Choose appropriate cmap
    color_map = plt.cm.get_cmap(cmap_name, max(1, len(sectors))) # Avoid division by zero if len(sectors) is 0
    
    for i, sector in enumerate(sectors):
        sector_colors[sector] = color_map(i)
    
    # Get node colors based on sector, with a default for missing sectors
    node_colors = [sector_colors.get(G.nodes[node].get('sector', 'Unknown'), 'gray') for node in G.nodes()]
    
    # Create the figure
    plt.figure(figsize=(14, 12))
    
    # Draw regular edges (gray and thin)
    nx.draw_networkx_edges(G, layout, edgelist=list(G.edges()), 
                          width=0.5, edge_color='lightgray', alpha=0.3,
                          connectionstyle='arc3,rad=0.1')
    
    # Collect all valid flow edges that exist in G
    valid_flow_edges = set()
    path_issues_count = 0
    if flow_paths: # Only process if there are paths
        for i, path_info in enumerate(flow_paths):
            path_nodes = path_info.get('path')
            path_flow = path_info.get('flow')

            if not path_nodes:
                print(f"Debug: Path {i} is missing 'path' data or is empty.")
                path_issues_count +=1
                continue
            if path_flow is None: # 0 is a valid flow
                print(f"Debug: Path {i} (nodes: {path_nodes}) is missing 'flow' data.")
                path_issues_count +=1
                continue

            current_path_nodes_valid = True
            for node_name in path_nodes:
                if node_name not in G:
                    print(f"Debug: Node '{node_name}' in path {i} (flow: {path_flow}) is NOT in graph G.")
                    current_path_nodes_valid = False
                    path_issues_count +=1
                    break 
            if not current_path_nodes_valid:
                continue # Skip this path as it contains nodes not in G

            # If all nodes in path are valid, check and add edges
            for j in range(len(path_nodes) - 1):
                u, v = path_nodes[j], path_nodes[j+1]
                if G.has_edge(u, v):
                    valid_flow_edges.add((u, v))
                else:
                    print(f"Debug: Edge ({u}, {v}) from path {i} (nodes: {path_nodes}, flow: {path_flow}) does NOT exist in graph G (though nodes {u} and {v} exist).")
                    path_issues_count +=1
    else:
        print("Debug: No flow paths provided in 'flow_paths' list.")

    if path_issues_count > 0:
        print(f"Debug: Encountered issues (missing data, nodes/edges not in G) for {path_issues_count} segments/paths.")

    print(f"Debug: Identified {len(valid_flow_edges)} unique, valid flow edges to highlight.")
    
    # Map flow values to edge colors and widths for valid_flow_edges
    if valid_flow_edges:
        # Sort to ensure consistent list order for widths/colors if that matters for some backend/version
        # For NetworkX drawing, the order of edgelist, width, and edge_color lists must correspond.
        sorted_valid_flow_edges = sorted(list(valid_flow_edges))

        edge_colors_list = []
        edge_widths_list = []
        
        for u, v in sorted_valid_flow_edges:
            max_flow_on_this_edge = 0.0 # Initialize
            for path_info in flow_paths: # Iterate all original paths
                p_nodes = path_info.get('path', [])
                p_flow = path_info.get('flow', 0.0)
                for i in range(len(p_nodes) - 1):
                    if p_nodes[i] == u and p_nodes[i+1] == v:
                        max_flow_on_this_edge = max(max_flow_on_this_edge, p_flow)
                        break # Edge found in this path
            
            edge_colors_list.append('blue') # All flow paths are blue
            # Ensure a minimum visible width, and scale based on flow
            edge_widths_list.append(max(0.5, 1.0 + 5.0 * max_flow_on_this_edge)) 
        
        print(f"Debug: Drawing {len(sorted_valid_flow_edges)} flow edges.")
        nx.draw_networkx_edges(G, layout, edgelist=sorted_valid_flow_edges, 
                              width=edge_widths_list, edge_color=edge_colors_list, alpha=0.7,
                              connectionstyle='arc3,rad=0.1', arrowsize=15)
    else:
        print("Debug: No valid flow edges to draw (either no paths, or paths contain nodes/edges not in G).")
    
    # Draw nodes
    nx.draw_networkx_nodes(G, layout, node_color=node_colors, node_size=300, alpha=0.8)

    # Highlight source and sink nodes
    nx.draw_networkx_nodes(G, layout, nodelist=[source], node_color='green', node_size=500, edgecolors='black', linewidths=2)
    nx.draw_networkx_nodes(G, layout, nodelist=[sink], node_color='red', node_size=500, edgecolors='black', linewidths=2)

    # Draw labels
    nx.draw_networkx_labels(G, layout, font_size=8, font_family='sans-serif')
    
    # Add a legend for sectors and flow
    legend_patches = [plt.Line2D([0], [0], marker='o', color='w', 
                               markerfacecolor=sector_colors.get(sector, 'gray'), # Use .get for safety
                               markersize=10, label=sector) 
                    for sector in sectors]
    if valid_flow_edges: # Only add flow path to legend if they are drawn
        legend_patches.append(plt.Line2D([0], [0], linestyle='-', color='blue', linewidth=2, label='Max Flow Path'))
    legend_patches.append(plt.Line2D([0], [0], marker='o', color='w', markerfacecolor='green', markersize=10, markeredgecolor='black', label='Source Node'))
    legend_patches.append(plt.Line2D([0], [0], marker='o', color='w', markerfacecolor='red', markersize=10, markeredgecolor='black', label='Sink Node'))

    plt.legend(handles=legend_patches, title="Legend", loc='best', fontsize='small')
    
    plt.title(f"Maximum Flow from {source} to {sink}")
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(output_file, dpi=300)
    print(f"Debug: Max flow visualization saved to {output_file}")
    plt.show()
    print(f"--- Debug: visualize_max_flow finished ---")

def main():
    # Determine which dataset to use
    # dataset_choice = input("Enter 'small' for small dataset or '50' for 50-company dataset: ").strip().lower()
    dataset_choice = 'small' # Default to small for now, can be changed or prompted

    if dataset_choice == '50':
        edges_file = 'edges_weights_50.csv'
        sector_file = 'sector_mapping_50.csv'
        network_output_file = '50_network_visualization.png'
        max_flow_output_file = '50_max_flow_visualization.png'
    else:
        edges_file = 'edges_small.csv'
        sector_file = 'companies_small.csv'
        network_output_file = 'small_network_visualization.png'
        max_flow_output_file = 'small_max_flow_visualization.png'

    try:
        edges_df, companies_df = load_data(edges_file, sector_file)
        G = create_graph(edges_df, companies_df)
        visualize_graph(G, output_file=network_output_file, title=f"Company Network ({dataset_choice} dataset)")

        # Load max flow results
        try:
            with open('max_flow_results.json', 'r') as f:
                max_flow_data = json.load(f)
            
            source_node = max_flow_data.get('source')
            sink_node = max_flow_data.get('sink')
            flow_paths = max_flow_data.get('paths', [])
            max_flow_value = max_flow_data.get('max_flow', 0)

            if source_node and sink_node and flow_paths:
                print(f"Visualizing max flow from {source_node} to {sink_node}, Max Flow: {max_flow_value}")
                visualize_max_flow(G, flow_paths, source_node, sink_node, output_file=max_flow_output_file)
            else:
                print("Max flow data is incomplete or not found in max_flow_results.json. Skipping max flow visualization.")

        except FileNotFoundError:
            print("max_flow_results.json not found. Skipping max flow visualization.")
        except json.JSONDecodeError:
            print("Error decoding max_flow_results.json. Skipping max flow visualization.")
        except Exception as e:
            print(f"Error processing max flow data: {e}. Skipping max flow visualization.")

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == '__main__':
    main()