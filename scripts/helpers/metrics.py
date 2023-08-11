#import networkx as nx
from typing import Tuple
import pandas as pd
from igraph import Graph
import sqlite3
import numpy as np

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

    # Step 1: Initialize distances from src to all other vertices as INFINITE
    dist = [np.nan] * vertex_count
    dist[src] = 0

    frontier = []
    frontier.append(src)

    # Step 2: Relax all edges |V| - 1 times. A simple shortest 
    # path from src to any other vertex can have at most |V| - 1 
    # edges
    while len(frontier) > 0:
        vertex = frontier.pop(0)
        res = []
        if not directional:
            res = cur.execute("SELECT destination, weight FROM edgelist WHERE source = ? UNION SELECT source, weight FROM edgelist WHERE destination = ?", [vertex, vertex])
        else:
            res = cur.execute("SELECT destination, weight FROM edgelist WHERE source = ?", [vertex])
        edges = res.fetchall()
        for edge in edges:
            dest, weight = edge
            if (np.isnan(dist[dest]) or dist[vertex] + weight < dist[dest]) and abs(dist[vertex] + weight) <= tau:
                dist[dest] = dist[vertex] + weight
                frontier.append(dest)

    ret = pd.DataFrame(None, columns = ["Distance"])
    index = 0
    for distance in dist:
        if not np.isnan(distance):
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

    # Step 1: Initialize distances from src to all other vertices as INFINITE, order as -1
    dist = pd.DataFrame({'Distance':pd.Series([], dtype='float'),'Order':pd.Series([], dtype='int')})
    for i in range(vertex_count):
        dist.loc[i] = [np.nan, -1]
    dist.loc[src] = [0, 0]

    frontier = []
    frontier.append(src)
    # Step 2: Relax all edges |V| - 1 times. A simple shortest 
    # path from src to any other vertex can have at most |V| - 1 
    # edges
    while len(frontier) > 0:
        vertex = frontier.pop(0)
        res = []
        if not directional:
            res = cur.execute("SELECT destination, weight FROM edgelist WHERE source = ? UNION SELECT source, weight FROM edgelist WHERE destination = ?", [vertex, vertex])
        else:
            res = cur.execute("SELECT destination, weight FROM edgelist WHERE source = ?", [vertex])
        edges = res.fetchall()
        for edge in edges:
            dest, weight = edge
            if (np.isnan(dist.iat[dest, 0]) or dist.iat[vertex, 0] + weight < dist.iat[dest, 0]) and dist.iat[vertex, 1] < k:
                dist.iat[dest, 0] = dist.iat[vertex, 0] + weight
                dist.iat[dest, 1] = dist.iat[vertex, 1] + 1
                frontier.append(dest)
            elif dist.iat[vertex, 0] + weight == dist.iat[dest, 0] and dist.iat[dest, 1] > dist.iat[vertex, 1] + 1 and dist.iat[vertex, 1] < k:
                dist.iat[dest, 1] = dist.iat[vertex, 1] + 1
                frontier.append(dest)

    dist.dropna(inplace = True)
    dist.sort_values("Distance", inplace=True)
    return dist

def convert_to_g_sqlite(con:sqlite3.Connection, undirected=True):
    cur = con.cursor()
    cur.execute("CREATE TABLE neg_laplacian_edgelist(source INT, destination INT, weight FLOAT)")
    con.commit()

    res = cur.execute("SELECT source, destination, max(weight) FROM edgelist GROUP BY source,destination WHERE source != destination")
    edges = res.fetchall()
    for edge in edges:
        source, destination, weight = edge
        cur.execute("INSERT INTO neg_laplacian_edgelist VALUES(?, ?, ?)", source, destination, -weight)
    
    if undirected:
        res = cur.execute("SELECT source FROM edgelist UNION SELECT destination FROM edgelist")
        sources = res.fetchall()
        for row in sources:
            source = row[0]
            res = cur.execute("INSERT INTO neg_laplacian_edgelist VALUES(?, ?, (SELECT sum(weight) FROM edgelist WHERE source = ? OR destination = ?))", [source, source, source, source])
    else:
        res = cur.execute("INSERT INTO neg_laplacian_edgelist VALUES(SELECT source, source, sum(weight) FROM edgelist GROUP BY source)")
