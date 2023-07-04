#import networkx as nx
import pandas as pd

def sparsity(G):
    sparsity = G.density()
    print(f"Sparsity: {sparsity}")
    return sparsity

def centrality(G):
    centrality_list = G.betweenness()
    centrality_arr = []
    vertex = 0
    for item in centrality_list:
        entry = []
        entry.append(vertex)
        entry.append(item)
        centrality_arr.append(entry)
        vertex += 1
    centrality_df = pd.DataFrame(centrality_arr,columns=["Node","Centrality"])
    centrality_df.sort_values("Centrality", inplace=True)
    print(f"Centrality:")
    print(centrality_df)
    return centrality_df