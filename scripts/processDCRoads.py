#!/usr/bin/python3
import helpers.format as f
import helpers.visualize as v
import helpers.metrics as m

df = f.parse_via_regex("datasets/Transport/DC.tmp", r"(\d+) (\d+)\n[\d.]+ ([\d.]+) [\d.]+")
G = f.create_graph(df)
#v.graph(np_arr)
#m.sparsity(G)
#m.centrality(G)
#print(df)
#print(m.calculate_geodesic_k(df, 50, 39379))
print(m.calculate_geodesic_tau(df, 50, 39379*40))

results = G.get_shortest_paths(50, to=5422, weights=G.es["Weight"], output="epath")
if len(results[0]) > 0:
    # Add up the weights across all edges on the shortest path
    distance = 0
    for e in results[0]:
        distance += G.es[e]["Weight"]
    print("Shortest weighted distance is: ", distance, len(results[0]))
else:
    print("End node could not be reached!")
