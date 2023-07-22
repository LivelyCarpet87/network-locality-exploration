#import networkx as nx
import time
import pandas as pd
from igraph import Graph
from multiprocessing import Pool, Queue
import queue

def sparsity(G:Graph):
    sparsity = G.density()
    return sparsity

def centrality(G:Graph):
    centrality_list = G.betweenness()
    centrality_arr = []
    vertex = 0
    for item in centrality_list:
        entry = []
        entry.append(vertex)
        entry.append(item)
        centrality_arr.append(entry)
        vertex += 1
    centrality_df = pd.DataFrame(centrality_arr,columns=["Node","Centrality"])
    centrality_df.sort_values("Centrality", inplace=True)
    return centrality_df

def calculate_geodesic_tau(edgelist:pd.DataFrame, src:int, tau:float, directional=False, signed=False):
    vertex_count = edgelist.shape[0]
    INF = float("Inf")

    # Step 1: Initialize distances from src to all other vertices as INFINITE
    dist = [INF] * vertex_count
    dist[src] = 0

    # Step 2: Relax all edges |V| - 1 times. A simple shortest 
    # path from src to any other vertex can have at most |V| - 1 
    # edges
    for i in range(vertex_count - 1):
        # Update dist value and parent index of the adjacent vertices
        # of the picked vertex. Consider only those vertices which are
        # still in queue
        def row_operation(row):
            changed = None
            source, dest, weight = row
            #print(source, dest, weight, abs(dist[source] + weight) <= tau, dist)
            if dist[source] != INF and abs(dist[source] + weight) <= tau and dist[source] + weight < dist[dest]:
                dist[dest] = dist[source] + weight
                changed = True
            if not directional:
                if signed and dist[dest] != INF and abs(dist[dest] - weight) <= tau and dist[dest] - weight < dist[source]:
                    dist[source] = dist[dest] - weight
                    changed = True
                elif not signed and dist[dest] != INF and abs(dist[dest] + weight) <= tau and dist[dest] + weight < dist[source]:
                    dist[source] = dist[dest] + weight
                    changed = True
            return changed
        res = edgelist.apply(row_operation, axis=1)
        if res.dropna().shape[0] == 0:
            break

    # Step 3: Check for negative-weight cycles. The above step 
    # guarantees shortest distances if graph doesn't contain 
    # negative weight cycle. If we get a shorter path, then there
    # is a cycle
    def verification_row_operation(row):
        source, dest, weight = row
        source, dest, weight = row
        #print(source, dest, weight, abs(dist[source] + weight) <= tau, dist)
        if dist[source] != INF and abs(dist[source] + weight) <= tau and dist[source] + weight < dist[dest]:
            raise ValueError
        if not directional:
            if signed and dist[dest] != INF and abs(dist[dest] - weight) <= tau and dist[dest] - weight < dist[source]:
                raise ValueError
            elif not signed and dist[dest] != INF and abs(dist[dest] + weight) <= tau and dist[dest] + weight < dist[source]:
                raise ValueError
    edgelist.apply(verification_row_operation, axis=1)

    ret = pd.DataFrame(None, columns = ["Distance"])
    index = 0
    for distance in dist:
        if distance != INF:
            ret.loc[index] = [distance]
        index += 1
    ret.sort_values("Distance", inplace=True)
    return ret

def calculate_geodesic_k(edgelist:pd.DataFrame, src:int, k:int, directional=False, signed=False):
    vertex_count = edgelist.shape[0]
    INF = float("Inf")

    # Step 1: Initialize distances from src to all other vertices as INFINITE, order as -1
    dist = pd.DataFrame({'Distance':pd.Series([], dtype='float'),'Order':pd.Series([], dtype='int')})
    for i in range(vertex_count):
        dist.loc[i] = [INF, -1]
    dist.loc[src] = [0, 0]

    iterations = k
    if k >= vertex_count:
        iterations = vertex_count - 1
    # Step 2: Relax all edges |V| - 1 times. A simple shortest 
    # path from src to any other vertex can have at most |V| - 1 
    # edges
    for i in range(iterations):
        # Update dist value and parent index of the adjacent vertices
        # of the picked vertex. Consider only those vertices which are
        # still in queue
        def row_operation(row):
            changed = None
            source, dest, weight = row
            if dist.iat[source, 0] != INF and dist.iat[source, 0] + weight < dist.iat[dest, 0] and dist.iat[dest, 1] < k:
                dist.iat[dest, 0] = dist.iat[source, 0] + weight
                dist.iat[dest, 1] = dist.iat[source, 1] + 1
                changed = True
            if not directional:
                if signed and dist.iat[dest, 0] != INF and dist.iat[dest, 0] - weight < dist.iat[source, 0] and dist.iat[dest, 1] < k:
                    dist.iat[source, 0] = dist.iat[dest, 0] - weight
                    dist.iat[source, 1] = dist.iat[dest, 1] + 1
                    changed = True
                elif not signed and dist.iat[dest, 0] != INF and dist.iat[dest, 0] + weight < dist.iat[source, 0] and dist.iat[dest, 1] < k:
                    dist.iat[source, 0] = dist.iat[dest, 0] + weight
                    dist.iat[source, 1] = dist.iat[dest, 1] + 1
                    changed = True
            return changed
        res = edgelist.apply(row_operation, axis=1)
        if res.dropna().shape[0] == 0:
            break
    dist.replace(INF, None, inplace=True)
    dist.dropna(inplace = True)
    dist.sort_values("Distance", inplace=True)
    return dist

def calculate_geodesic_tau_dijkstra(G:Graph, src:int, tau:float, directional=False):
    INF = float("Inf")
    dist = pd.DataFrame({'Distance':pd.Series([], dtype="float64"),'Order':pd.Series([], dtype='int64'),'Back Pointer':pd.Series([], dtype='int64')})
    for i in range(G.vcount()):
        dist.loc[i] = [INF, -1, -1]
    dist.loc[src] = [0, 0, -1]

    priorityQueue = Queue()
    priorityQueue.put(src)

    def dijkstra_step(G:Graph, dist, target, priorityQueue:Queue):
        adjacents = []
        if directional == False:
            adjacents = G.neighbors(target, mode="all")
        else:
            adjacents = G.neighbors(target, mode="out")

        print(adjacents)

        for node in adjacents:
            if node == target:
                continue
            eids = G.get_eids([(target, node)])
            distance = INF
            for eid in eids:
                weight = G.es[eid]["Weight"]
                if weight < distance:
                    distance = weight
            
            add_to_queue = False
            if dist.loc[node]["Distance"] == INF:
                add_to_queue = True
            new_distance = distance + dist.loc[target]["Distance"]

            if dist.loc[node]["Distance"] > new_distance:
                dist.loc[node]["Distance"] = new_distance
                dist.loc[node]["Order"] = dist.loc[target]["Order"] + 1
                dist.loc[node]["Back Pointer"] = target

            if add_to_queue:
                priorityQueue.put(node)
        return
    with Pool(processes=4) as pool:
        while True:
            target = -1
            try:
                target = priorityQueue.get(timeout=1)
            except queue.Empty:
                break
            pool.apply_async(dijkstra_step, (G,dist,target,priorityQueue,))
            print("Launching worker.")
        print(priorityQueue.qsize())
        pool.close()
        pool.join()
    return dist
