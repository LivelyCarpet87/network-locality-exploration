#import networkx as nx
from typing import Tuple
import pandas as pd
from igraph import Graph
import sqlite3
import numpy as np
from tqdm import tqdm
from multiprocessing import Process

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
            if (np.isnan(dist.iat[dest, 0]) or dist.iat[vertex, 0] + weight < dist.iat[dest, 0]) and abs(dist.iat[vertex, 0] + weight) <= tau:
                dist.iat[dest, 0] = dist.iat[vertex, 0] + weight
                dist.iat[dest, 1] = dist.iat[vertex, 1] + 1
                frontier.append(dest)
            elif dist.iat[vertex, 0] + weight == dist.iat[dest, 0] and dist.iat[dest, 1] > dist.iat[vertex, 1] + 1 and abs(dist.iat[vertex, 0] + weight) <= tau:
                dist.iat[dest, 1] = dist.iat[vertex, 1] + 1
                frontier.append(dest)

    dist.dropna(inplace = True)
    dist.sort_values("Distance", inplace=True)
    con.close()
    return dist

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

def calculate_geodesic_tau_sqlite(con:sqlite3.Connection, src:int, tau:float, directional=False):
    INF = float("INF")
    cur = con.cursor()

    # Step 1: Initialize distances from src to all other vertices as INFINITE
    cur.execute("CREATE TABLE IF NOT EXISTS geodesic_distances(destination INTEGER UNIQUE, distance FLOAT, degree INTEGER)")
    cur.execute("INSERT INTO geodesic_distances VALUES(?, ?, ?)",[src,0,0])
    con.commit()

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

            dest_query = cur.execute("SELECT distance, degree FROM geodesic_distances WHERE destination = ?", [dest])
            dest_res = dest_query.fetchone()
            dest_distance, dest_order = dest_res if dest_res else (INF, -1)

            vertex_query = cur.execute("SELECT distance, degree FROM geodesic_distances WHERE destination = ?", [vertex])
            vertex_res = vertex_query.fetchone()
            vertex_distance, vertex_order = vertex_res

            if vertex_distance + weight < dest_distance and abs(vertex_distance + weight) <= tau:
                dest_distance = vertex_distance + weight
                dest_order = vertex_order + 1
                cur.execute("INSERT INTO geodesic_distances(destination,distance,degree) VALUES(?, ?, ?) ON CONFLICT(destination) DO UPDATE SET distance=excluded.distance, degree=excluded.degree;",[dest,dest_distance,dest_order])
                if dest not in frontier:
                    frontier.append(dest)
            elif vertex_distance + weight == dest_distance and dest_order > vertex_order + 1 and abs(vertex_distance + weight) <= tau:
                dest_distance = vertex_distance + weight
                dest_order = vertex_order + 1
                cur.execute("UPDATE geodesic_distances SET degree = ? WHERE destination = ?",[dest_order, dest])
                if dest not in frontier:
                    frontier.append(dest)
    return con

def calculate_geodesic_k_sqlite(con:sqlite3.Connection, src:int, k:int, directional=False, signed=False):
    INF = float("INF")
    cur = con.cursor()

    # Step 1: Initialize distances from src to all other vertices as INFINITE
    cur.execute("CREATE TABLE IF NOT EXISTS geodesic_distances(destination INTEGER UNIQUE, distance FLOAT, degree INTEGER)")
    cur.execute("INSERT INTO geodesic_distances VALUES(?, ?, ?)",[src,0,0])
    con.commit()

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

            dest_query = cur.execute("SELECT distance, degree FROM geodesic_distances WHERE destination = ?", [dest])
            dest_res = dest_query.fetchone()
            dest_distance, dest_order = dest_res if dest_res else (INF, -1)

            vertex_query = cur.execute("SELECT distance, degree FROM geodesic_distances WHERE destination = ?", [vertex])
            vertex_res = vertex_query.fetchone()
            vertex_distance, vertex_order = vertex_res

            if vertex_distance + weight < dest_distance and vertex_order < k:
                dest_distance = vertex_distance + weight
                dest_order = vertex_order + 1
                cur.execute("INSERT INTO geodesic_distances(destination,distance,degree) VALUES(?, ?, ?) ON CONFLICT(destination) DO UPDATE SET distance=excluded.distance, degree=excluded.degree;",[dest,dest_distance,dest_order])
                if dest not in frontier:
                    frontier.append(dest)
            elif vertex_distance + weight == dest_distance and dest_order > vertex_order + 1 and vertex_order < k:
                dest_distance = vertex_distance + weight
                dest_order = vertex_order + 1
                cur.execute("UPDATE geodesic_distances SET degree = ? WHERE destination = ?",[dest_order, dest])
                if dest not in frontier:
                    frontier.append(dest)
    return con

def calculate_geodesic_k_sqlite_cross(con:sqlite3.Connection, k:int, directional=False, signed=False):
    INF = float("INF")
    cur = con.cursor()

    # Step 1: Initialize distances from src to all other vertices as INFINITE
    cur.execute("CREATE TABLE IF NOT EXISTS geodesic_distances_cross(source INTEGER, destination INTEGER, distance FLOAT, degree INTEGER, UNIQUE(source, destination) )")
    
    for row in tqdm(cur.execute("SELECT source FROM edgelist UNION SELECT destination FROM edgelist").fetchall()):
        src = row[0]
        cur.execute("INSERT INTO geodesic_distances_cross VALUES(?, ?, ?, ?)",[src,src,0,0])
        con.commit()

        frontier = []
        frontier.append(src)

        # Step 2: Relax all edges |V| - 1 times. A simple shortest 
        # path from src to any other vertex can have at most |V| - 1 
        # edges
        while len(frontier) > 0:
            vertex = frontier.pop(0)
            res = []
            res = cur.execute("SELECT destination, weight FROM edgelist WHERE source = ?", [vertex])
            edges = res.fetchall()

            vertex_query = cur.execute("SELECT distance, degree FROM geodesic_distances_cross WHERE destination = ? AND source = ?", [vertex, src])
            
            vertex_res = vertex_query.fetchone()
            vertex_distance, vertex_order = vertex_res
            
            for edge in edges:
                dest, weight = edge

                dest_query = None
                dest_query = cur.execute("SELECT MIN(distance), degree FROM geodesic_distances_cross WHERE destination = ? AND source = ?", [dest, src])
                dest_res = dest_query.fetchone()
                dest_distance, dest_order = dest_res if (dest_res is not None and dest_res[0] is not None) else (INF, -1)

                if vertex_distance + weight < dest_distance and vertex_order < k:
                    dest_distance = vertex_distance + weight
                    dest_order = vertex_order + 1
                    cur.execute("INSERT INTO geodesic_distances_cross(source, destination,distance,degree) VALUES(?, ?, ?, ?) ON CONFLICT(source,destination) DO UPDATE SET distance=excluded.distance, degree=excluded.degree;",[src,dest,dest_distance,dest_order])
                    con.commit()
                    if dest not in frontier:
                        frontier.append(dest)
                elif vertex_distance + weight == dest_distance and dest_order > vertex_order + 1 and vertex_order < k:
                    dest_distance = vertex_distance + weight
                    dest_order = vertex_order + 1
                    cur.execute("UPDATE geodesic_distances_cross SET degree = ? WHERE source = ? AND destination = ?",[dest_order, src, dest])
                    con.commit()
                    if dest not in frontier:
                        frontier.append(dest)
    return con

def generate_v_func():
    con_2 = sqlite3.connect("./constants.db")
    cur_2 = con_2.cursor()
    cur_2.execute("CREATE TABLE IF NOT EXISTS v_func(x FLOAT UNIQUE, y FLOAT AS (exp(pow(x,0.9)) * pow(1+x,1.2)) STORED)")
    for i in range(1,10**7):
        cur_2.execute("INSERT INTO v_func VALUES(?)",[i/10000])
    con_2.commit()


def convert_to_g_sqlite(con:sqlite3.Connection, undirected = True):
    cur = con.cursor()
    cur.execute("CREATE TABLE neg_laplacian_edgelist(source INT, destination INT, weight FLOAT)")
    con.commit()

    if undirected:
        cur.execute("INSERT INTO edgelist SELECT destination, source, weight FROM edgelist")

    # Take the negative of the max weight for each edge
    cur.execute("INSERT INTO neg_laplacian_edgelist SELECT source, destination, -max(weight) FROM edgelist WHERE source != destination GROUP BY source,destination")
    # Take the sum for each diagonal element
    cur.execute("INSERT INTO neg_laplacian_edgelist SELECT source, source, -sum(weight) FROM edgelist GROUP BY source")
    
    cur.execute("CREATE TABLE g_edgelist(source INT, destination INT, weight FLOAT)")

    MAX_WEIGHT = cur.execute("SELECT MAX(ABS(weight)) FROM neg_laplacian_edgelist").fetchone()[0]
    res = cur.execute("""
        SELECT q.s, q.d, MAX(ABS(weight)) 
        FROM (
        SELECT source AS s, destination AS d, weight FROM neg_laplacian_edgelist
        UNION ALL
        SELECT destination AS s, source AS d, weight FROM neg_laplacian_edgelist
        ) AS q
        GROUP BY q.s,q.d
        """)
    
    for entry in res.fetchall():
        s, d, w = entry
        y = MAX_WEIGHT / w
        #cur.execute("INSERT INTO neg_laplacian_edgelist SELECT source, source, -sum(weight) FROM edgelist GROUP BY source")
        print(s,d,y)