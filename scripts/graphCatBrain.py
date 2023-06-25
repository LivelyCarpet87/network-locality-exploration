import networkx as nx
import matplotlib.pyplot as plt

G = nx.read_graphml('datasets/Biology/mixed.species_brain_1.graphml')
nx.draw(G)
plt.show()