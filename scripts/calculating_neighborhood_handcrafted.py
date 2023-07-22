import pandas
import helpers.format as f
import helpers.visualize as v
import helpers.metrics as m
from igraph import Graph
import networkx as nx
import matplotlib.pyplot as plt

graph_1 = [ 
    [0, 5, 3], 
    [1, 7, 2], 
    [2, 5, 4], 
    [3, 9, 2], 
    [4, 5, 5], 
    [5, 9, 3], 
    [6, 3, 5], 
    [7, 9, 3], 
    [8, 9, 2], 
    [9, 0, 1]
]
df = pandas.DataFrame(graph_1, columns=["From", "To", "Weight"])
res = m.calculate_geodesic_tau(df,0,10)
print(res)
res = m.calculate_geodesic_k(df,0,3)
print(res)