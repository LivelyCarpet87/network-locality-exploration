#!/usr/bin/python3
import networkx as nx
import matplotlib.pyplot as plt

def graph(G):
    nx.draw(G, with_labels=True, font_weight='bold')
    plt.show()

