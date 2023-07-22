#import networkx as nx
import pandas as pd
from igraph import Graph
import sqlite3

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

def calculate_geodesic_tau(edgelist:pd.DataFrame, src:int, tau:float, directional=False):
    con = sqlite3.connect(":memory:")
    cur = con.cursor()
    cur.execute("CREATE TABLE edgelist(source INT, destination INT, weight FLOAT)")
    cur.executemany("INSERT INTO edgelist VALUES(?, ?, ?)", edgelist.to_dict('split')["data"])
    con.commit()

    vertex_count = edgelist.shape[0]
    INF = float("Inf")

    # Step 1: Initialize distances from src to all other vertices as INFINITE
    dist = [INF] * vertex_count
    dist[src] = 0

    frontier = []
    frontier.append(src)

    # Step 2: Relax all edges |V| - 1 times. A simple shortest 
    # path from src to any other vertex can have at most |V| - 1 
    # edges
    while len(frontier) > 0:
        print(frontier)
        vertex = frontier.pop(0)
        res = []
        if not directional:
            res = cur.execute("SELECT destination, weight FROM edgelist WHERE source = ? UNION SELECT source, weight FROM edgelist WHERE destination = ?", [vertex, vertex])
        else:
            res = cur.execute("SELECT destination, weight FROM edgelist WHERE source = ?", [vertex])
        edges = res.fetchall()
        for edge in edges:
            dest, weight = edge
            if abs(dist[vertex] + weight) <= tau and dist[vertex] + weight < dist[dest]:
                dist[dest] = dist[vertex] + weight
                frontier.append(dest)

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
            if dist[dest] != INF and abs(dist[dest] + weight) <= tau and dist[dest] + weight < dist[source]:
                raise ValueError
    edgelist.apply(verification_row_operation, axis=1)

    ret = pd.DataFrame(None, columns = ["Distance"])
    index = 0
    for distance in dist:
        if distance != INF:
            ret.loc[index] = [distance]
        index += 1
    ret.sort_values("Distance", inplace=True)
    con.close()
    return ret

def calculate_geodesic_k(edgelist:pd.DataFrame, src:int, k:int, directional=False, signed=False):
    con = sqlite3.connect(":memory:")
    cur = con.cursor()
    cur.execute("CREATE TABLE edgelist(source INT, destination INT, weight FLOAT)")
    cur.executemany("INSERT INTO edgelist VALUES(?, ?, ?)", edgelist.to_dict('split')["data"])
    con.commit()

    vertex_count = edgelist.shape[0]
    INF = float("Inf")

    # Step 1: Initialize distances from src to all other vertices as INFINITE, order as -1
    dist = pd.DataFrame({'Distance':pd.Series([], dtype='float'),'Order':pd.Series([], dtype='int')})
    for i in range(vertex_count):
        dist.loc[i] = [INF, -1]
    dist.loc[src] = [0, 0]

    frontier = []
    frontier.append(src)
    # Step 2: Relax all edges |V| - 1 times. A simple shortest 
    # path from src to any other vertex can have at most |V| - 1 
    # edges
    while len(frontier) > 0:
        print(frontier)
        vertex = frontier.pop(0)
        res = []
        if not directional:
            res = cur.execute("SELECT destination, weight FROM edgelist WHERE source = ? UNION SELECT source, weight FROM edgelist WHERE destination = ?", [vertex, vertex])
        else:
            res = cur.execute("SELECT destination, weight FROM edgelist WHERE source = ?", [vertex])
        edges = res.fetchall()
        for edge in edges:
            dest, weight = edge
            if dist.iat[vertex, 0] + weight < dist.iat[dest, 0] and dist.iat[dest, 1] < k:
                dist.iat[dest, 0] = dist.iat[vertex, 0] + weight
                dist.iat[dest, 1] = dist.iat[vertex, 1] + 1
                frontier.append(dest)

    dist.replace(INF, None, inplace=True)
    dist.dropna(inplace = True)
    dist.sort_values("Distance", inplace=True)
    return dist

