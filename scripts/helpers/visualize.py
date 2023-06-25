#!/usr/bin/python3
import networkx as nx
import pandas as pd
import matplotlib.pyplot as plt

def graph(np_array):
    G = nx.Graph()
    G.add_weighted_edges_from(np_array)
    nx.draw(G, with_labels=True, font_weight='bold')
    plt.show()

