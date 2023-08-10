import networkx as nx
import random
import matplotlib.pyplot as plt
import helpers.metrics as m
import pandas as pd
from tqdm import tqdm

SIZE = 1000

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
    new_edges.append(edge)

#G = nx.Graph()
#G.add_edges_from(new_edges)
#nx.draw(G)
#plt.show()

df = pd.DataFrame(new_edges)
print(m.calculate_geodesic_k(df,100, 10))
print(m.calculate_geodesic_tau(df,100, 75))