#!/usr/bin/python3
import helpers.format as f
import helpers.visualize as v
import helpers.metrics as m

df = f.parse_via_regex("datasets/Transport/DC.tmp", r"(\d+) (\d+)\n[\d.]+ ([\d.]+) [\d.]+")
G = f.create_graph(df)
#v.graph(np_arr)
m.sparsity(G)
m.centrality(G)
m.calculate_geodesic_k(df, 50, 30)