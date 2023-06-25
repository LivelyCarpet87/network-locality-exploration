#!/usr/bin/python3
import helpers.format as f
import helpers.visualize as v
import helpers.metrics as m

df = f.parse_via_regex("datasets/Economics/world_trade.paj", r"(\d+)\s+(\d+)\s+(\d+)")
np_arr = df.to_numpy()
G = f.create_graph(np_arr)
#v.graph(np_arr)
m.sparsity(G)
m.betweenness_centrality(G)