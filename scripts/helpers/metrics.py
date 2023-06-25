import networkx as nx
import pandas as pd

def sparsity(G):
    sparsity = nx.density(G)
    print(f"Sparsity: {sparsity}")
    return sparsity

def betweenness_centrality(G):
    centrality_dict = nx.betweenness_centrality(G)
    centrality_arr = []
    for key in centrality_dict:
        entry = []
        entry.append(key)
        entry.append(centrality_dict[key])
        centrality_arr.append(entry)
    centrality_df = pd.DataFrame(centrality_arr,columns=["Node","Centrality"])
    centrality_df.sort_values("Node", inplace=True)
    print(f"Centrality:")
    print(centrality_df)
    return centrality_df