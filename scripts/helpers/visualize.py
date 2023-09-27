#!/usr/bin/python3
import math
import igraph as ig
import matplotlib.pyplot as plt
import sqlite3
import matplotlib as mpl
import matplotlib.patches as mpatches
colormap = mpl.colormaps['cool']
import numpy as np
from numpy import nan

def graph(G:ig.Graph):
    G["title"] = "Graph Title"
    fig, ax = plt.subplots(figsize=(WEIGHT_SCALE,WEIGHT_SCALE))

    layout = G.layout_kamada_kawai()
    ig.plot(G,target=ax,layout=layout)
    plt.show()

def graph_information_neighborhood(database_path, src):
    con = sqlite3.connect(database_path)
    cur = con.cursor()
    valid_edge_query = cur.execute( """
                                SELECT SRC, DST, WEIGHT FROM a_edgelist 
                                WHERE 
                                    SRC IN (SELECT DST FROM a_dbv_k WHERE SRC = ?) 
                                    AND 
                                    DST IN (SELECT DST FROM a_dbv_k WHERE SRC = ?)
                                """, [src, src])

    def translate_edge(row, cur):
        SRC_old, DST_old, weight = row
        return (SRC_old, DST_old, weight)
    reduced_edgelist = [translate_edge(item, cur) for item in valid_edge_query.fetchall()]
    
    G = ig.Graph.TupleList(reduced_edgelist, edge_attrs="weight")
    max_INFO_DIST = cur.execute("SELECT MAX(INFO_DIST) FROM a_dbv_k WHERE SRC = ?", [src]).fetchone()[0]
    max_NET_DIST = cur.execute("SELECT MAX(NET_DIST) FROM a_dbv_k WHERE SRC = ?", [src]).fetchone()[0]

    def vertex_size_lookup(name, cur:sqlite3.Cursor, max_NET_DIST):
        if name == src:
            return 1
        res = cur.execute("SELECT NET_DIST FROM a_dbv_k WHERE DST = ? AND SRC = ?", [name,src]).fetchone()[0]
        return 0.7 - (res * 0.4 / max_NET_DIST)
    
    def vertex_shade_lookup(name, cur:sqlite3.Cursor,max_INFO_DIST):
        res = cur.execute("SELECT INFO_DIST FROM a_dbv_k WHERE DST = ? AND SRC = ?", [name, src]).fetchone()[0]
        return colormap(res/max_INFO_DIST)

    fig, ax = plt.subplots(figsize=(40,40))
    layout = G.layout_kamada_kawai()
    #print(reduced_edgelist)
    ig.plot(G,
            target=ax,
            layout=layout,
            vertex_size=[vertex_size_lookup(vertex["name"], cur, max_NET_DIST) for vertex in G.vs],
            vertex_color=[vertex_shade_lookup(vertex["name"], cur, max_INFO_DIST) for vertex in G.vs],
            edge_width=0.1
            )

    fig.colorbar(mappable=mpl.cm.ScalarMappable(norm=mpl.colors.Normalize(vmin=0, vmax=max_INFO_DIST), cmap=colormap, ), cmap=colormap, ax=ax, orientation='horizontal', label="Information INFO_DIST") 
    
    plt.show()