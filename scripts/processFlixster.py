#!/usr/bin/python3
import helpers.format as f
import helpers.visualize as v
import helpers.metrics as m

df = f.parse_separated_values("datasets/Social/flixster/out.flixster", delimiter=" ", unweighted=True)
np_arr = df.to_numpy()
G = f.create_graph(np_arr)
#v.graph(G)
m.sparsity(G)
m.betweenness_centrality(G)