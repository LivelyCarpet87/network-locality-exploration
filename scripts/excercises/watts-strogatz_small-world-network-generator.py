import networkx as nx
import random
import matplotlib.pyplot as plt

edges = []
for i in range(0,100):
    new_edge = [i, (i+1)%100]
    edges.append(new_edge)

new_edges = []
for edge in edges:
    if random.randint(0,1) == 1:
        while edge[1] == edge[0] or edge[1] == (edge[0] + 1)%100:
            edge[1] = random.randint(0,99)
    new_edges.append(edge)
print(new_edges)

G = nx.Graph()
G.add_edges_from(new_edges)
nx.draw(G)
plt.show()