import networkx as nx
import random
import matplotlib.pyplot as plt
import helpers.metrics as m
import pandas as pd
from tqdm import tqdm
import sqlite3
import helpers.visualize as v

SIZE = 1000

#con = sqlite3.connect(":memory:")
con = sqlite3.connect("./debug.db")

"""

cur = con.cursor()
cur.execute("CREATE TABLE edgelist(source INT, destination INT, weight FLOAT)")
con.commit()

edges = []
for j in range(1,21):
    for i in range(0,SIZE):
        new_edge = [i, (i+j)%SIZE, random.random()]
        edges.append(new_edge)

new_edges = []
for edge in edges:
    if random.randint(1,10) == 1:
        while edge[1] == edge[0] or edge[1] == (edge[0] + 1)%SIZE:
            edge[1] = random.randint(0,SIZE-1)
    cur.execute("INSERT INTO edgelist VALUES(?, ?, ?)", edge)
cur.execute("INSERT INTO edgelist SELECT destination, source, weight FROM edgelist")

con.commit()

"""
#m.generate_v_func()
m.convert_to_g_sqlite(con)
#m.calculate_geodesic_k_sqlite(con,1,5)
#v.graph_information_neighborhood(con)
#m.calculate_geodesic_k_sqlite_cross(con,SIZE)
#v.graph_conditional_probability(con)

con.commit()
