import pandas as pd
import networkx as nx
import matplotlib.pyplot as plt
import sys

print("Python version:", sys.version)
print("Pandas version:", pd.__version__)
print("NetworkX version:", nx.__version__)
print("Matplotlib version:", plt.matplotlib.__version__)

# Test loading small dataset
try:
    edges_df = pd.read_csv('edges_small.csv')
    companies_df = pd.read_csv('companies_small.csv')
    print(f"Successfully loaded edges_small.csv with {len(edges_df)} rows")
    print(f"Successfully loaded companies_small.csv with {len(companies_df)} rows")
    print("Edges columns:", edges_df.columns.tolist())
    print("Companies columns:", companies_df.columns.tolist())
except Exception as e:
    print("Error:", str(e))

print("Test complete")
