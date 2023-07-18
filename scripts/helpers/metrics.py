#import networkx as nx
import pandas as pd
from igraph import Graph

def sparsity(G:Graph):
    sparsity = G.density()
    return sparsity

def centrality(G:Graph):
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
    return centrality_df

def calculate_geodesic_tau(edgelist:pd.DataFrame, src:int, tau:float, directional=False, signed=False):
    vertex_count = edgelist.shape[0]
    INF = float("Inf")

    # Step 1: Initialize distances from src to all other vertices as INFINITE
    dist = [INF] * vertex_count
    dist[src] = 0

    # Step 2: Relax all edges |V| - 1 times. A simple shortest 
    # path from src to any other vertex can have at most |V| - 1 
    # edges
    for i in range(vertex_count - 1):
        # Update dist value and parent index of the adjacent vertices
        # of the picked vertex. Consider only those vertices which are
        # still in queue
        def row_operation(row):
            source, dest, weight = row
            #print(source, dest, weight, abs(dist[source] + weight) <= tau, dist)
            if dist[source] != INF and abs(dist[source] + weight) <= tau and dist[source] + weight < dist[dest]:
                dist[dest] = dist[source] + weight
                break_const = 0
            if not directional:
                if signed and dist[dest] != INF and abs(dist[dest] - weight) <= tau and dist[dest] - weight < dist[source]:
                    dist[source] = dist[dest] - weight
                    break_const = 0
                elif not signed and dist[dest] != INF and abs(dist[dest] + weight) <= tau and dist[dest] + weight < dist[source]:
                    dist[source] = dist[dest] + weight
                    break_const = 0
        edgelist.apply(row_operation, axis=1)
        """
        if break_const == 1:
            break_const += 1
        if break_const == 2:
            break
        """

    # Step 3: Check for negative-weight cycles. The above step 
    # guarantees shortest distances if graph doesn't contain 
    # negative weight cycle. If we get a shorter path, then there
    # is a cycle
    def verification_row_operation(row):
        source, dest, weight = row
        source, dest, weight = row
        #print(source, dest, weight, abs(dist[source] + weight) <= tau, dist)
        if dist[source] != INF and abs(dist[source] + weight) <= tau and dist[source] + weight < dist[dest]:
            raise ValueError
        if not directional:
            if signed and dist[dest] != INF and abs(dist[dest] - weight) <= tau and dist[dest] - weight < dist[source]:
                raise ValueError
            elif not signed and dist[dest] != INF and abs(dist[dest] + weight) <= tau and dist[dest] + weight < dist[source]:
                raise ValueError
    edgelist.apply(verification_row_operation, axis=1)

    ret = pd.DataFrame(None, columns = ["Distance"])
    index = 0
    for distance in dist:
        if distance != INF:
            ret.loc[index] = [distance]
        index += 1
    ret.sort_values("Distance", inplace=True)
    return ret

def calculate_geodesic_k(edgelist:pd.DataFrame, src:int, k:int, directional=False, signed=False):
    vertex_count = edgelist.shape[0]
    INF = float("Inf")

    # Step 1: Initialize distances from src to all other vertices as INFINITE, order as -1
    dist = pd.DataFrame({'Distance':pd.Series([], dtype='float'),'Order':pd.Series([], dtype='int')})
    for i in range(vertex_count):
        dist.loc[i] = [INF, -1]
    dist.loc[src] = [0, 0]

    iterations = k
    if k >= vertex_count:
        iterations = vertex_count - 1
    # Step 2: Relax all edges |V| - 1 times. A simple shortest 
    # path from src to any other vertex can have at most |V| - 1 
    # edges
    for i in range(iterations):
        # Update dist value and parent index of the adjacent vertices
        # of the picked vertex. Consider only those vertices which are
        # still in queue
        def row_operation(row):
            source, dest, weight = row
            if dist.iat[source, 0] != INF and dist.iat[source, 0] + weight < dist.iat[dest, 0] and dist.iat[dest, 1] < k:
                dist.iat[dest, 0] = dist.iat[source, 0] + weight
                dist.iat[dest, 1] = dist.iat[source, 1] + 1
                break_const = 0
            if not directional:
                if signed and dist.iat[dest, 0] != INF and dist.iat[dest, 0] - weight < dist.iat[source, 0] and dist.iat[dest, 1] < k:
                    dist.iat[source, 0] = dist.iat[dest, 0] - weight
                    dist.iat[source, 1] = dist.iat[dest, 1] + 1
                    break_const = 0
                elif not signed and dist.iat[dest, 0] != INF and dist.iat[dest, 0] + weight < dist.iat[source, 0] and dist.iat[dest, 1] < k:
                    dist.iat[source, 0] = dist.iat[dest, 0] + weight
                    dist.iat[source, 1] = dist.iat[dest, 1] + 1
                    break_const = 0
        edgelist.apply(row_operation, axis=1)
        """
        if break_const == 1:
            break_const += 1
        if break_const == 2:
            break
        """
    dist.replace(INF, None, inplace=True)
    dist.dropna(inplace = True)
    dist.sort_values("Distance", inplace=True)
    return dist