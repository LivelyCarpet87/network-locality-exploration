#!/usr/bin/python3
import math
import igraph as ig
import matplotlib.pyplot as plt
import sqlite3
import matplotlib as mpl
import matplotlib.patches as mpatches
colormap = mpl.colormaps['cool']
import numpy as np

def graph(G:ig.Graph):
    G["title"] = "Graph Title"
    fig, ax = plt.subplots(figsize=(20,20))

    layout = G.layout_kamada_kawai()
    ig.plot(G,target=ax,layout=layout)
    plt.show()

def graph_information_neighborhood(con:sqlite3.Connection):
    cur = con.cursor()
    valid_edge_query = cur.execute( """
                                SELECT source, destination, weight FROM edgelist 
                                WHERE 
                                    source IN (SELECT destination FROM geodesic_distances) 
                                    AND 
                                    destination IN (SELECT destination FROM geodesic_distances)
                                """)

    def translate_edge(row, cur):
        source_old, destination_old, weight = row
        source_new = cur.execute("SELECT rowid FROM geodesic_distances WHERE destination = ?", [source_old]).fetchone()[0] - 1
        destination_new = cur.execute("SELECT rowid FROM geodesic_distances WHERE destination = ?", [destination_old]).fetchone()[0] - 1
        return (source_new, destination_new, weight)
    reduced_edgelist = [translate_edge(item, cur) for item in valid_edge_query.fetchall()]
    
    G = ig.Graph.TupleList(reduced_edgelist, edge_attrs="weight")
    max_distance = cur.execute("SELECT MAX(distance) FROM geodesic_distances").fetchone()[0]

    def vertex_size_lookup(name, cur:sqlite3.Cursor):
        max = cur.execute("SELECT MAX(degree) FROM geodesic_distances").fetchone()[0]
        res = cur.execute("SELECT degree FROM geodesic_distances WHERE rowid = ?", [int(name+1)]).fetchone()[0]
        return 0.5 - (res * 0.4 / max)
    
    def vertex_shade_lookup(name, cur:sqlite3.Cursor,max_distance):
        res = cur.execute("SELECT distance FROM geodesic_distances WHERE rowid = ?", [int(name+1)]).fetchone()[0]
        return colormap(res/max_distance)

    fig, ax = plt.subplots(figsize=(40,40))
    layout = G.layout_kamada_kawai()
    print(reduced_edgelist)
    ig.plot(G,
            target=ax,
            layout=layout,
            vertex_size=[vertex_size_lookup(vertex["name"], cur) for vertex in G.vs],
            vertex_color=[vertex_shade_lookup(vertex["name"], cur, max_distance) for vertex in G.vs],
            edge_width=0.1
            )

    fig.colorbar(mappable=mpl.cm.ScalarMappable(norm=mpl.colors.Normalize(vmin=0, vmax=max_distance), cmap=colormap, ), cmap=colormap, ax=ax, orientation='horizontal', label="Information Distance") 
    
    plt.show()

def graph_conditional_probability(con:sqlite3.Connection):
    cur = con.cursor()
    max_distance = cur.execute("SELECT MAX(distance) FROM geodesic_distances_cross").fetchone()[0]
    max_order = cur.execute("SELECT MAX(degree) FROM geodesic_distances_cross").fetchone()[0]
    m = [[0 for i in range(max_order+1)] for j in range(max_order+1)]
    distance_step = max_distance/(max_order+1)
    for distance_step_count in range(0,max_order+1):
        for order_step_count in range(0,max_order+1):
            distance_range_start = distance_step*distance_step_count
            distance_range_end = distance_range_start + distance_step
            order_range_start = order_step_count
            order_range_end = order_range_start + 1
            
            query = cur.execute("SELECT source FROM geodesic_distances_cross WHERE distance > ? AND distance <= ? AND degree >= ? AND degree < ?", 
                                [distance_range_start,distance_range_end,order_range_start,order_range_end])
            count = len(query.fetchall())
            m[distance_step_count][order_step_count] = count
    print(m)
    npm = np.matrix(m)
    plt.matshow(npm)
    plt.show()
    