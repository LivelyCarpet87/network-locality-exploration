import matplotlib.pyplot as plt
import numpy as np
from numpy import nan
import sqlite3

con = sqlite3.connect("database_path.db")
cur = con.cursor()
connections = cur.execute("SELECT SRC, DST, INFO_DIST FROM a_dbv_tau").fetchall()

TOTAL_COUNT = len(connections)
MAX_NET_DIST = cur.execute("SELECT MAX(NET_DIST) FROM a_dbv_tau").fetchone()[0]

WEIGHT_SCALE = 2

Z = [[np.NaN for i in range(MAX_NET_DIST)] for j in range(MAX_NET_DIST*WEIGHT_SCALE)]

for INFO_DIST_step_count in range(0,MAX_NET_DIST*WEIGHT_SCALE):
    for NET_DIST_step_count in range(0,MAX_NET_DIST):
        INFO_DIST_range_start = INFO_DIST_step_count / WEIGHT_SCALE
        INFO_DIST_range_end = INFO_DIST_range_start + 1/WEIGHT_SCALE
        NET_DIST_range_start = NET_DIST_step_count
        NET_DIST_range_end = NET_DIST_range_start + 1
        
        query = cur.execute("""
                            SELECT COUNT(g.INFO_DIST)
                            FROM g_dbv_tau g
                            INNER JOIN a_dbv_tau a ON
                            g.SRC = a.SRC AND g.DST = a.DST
                            WHERE g.INFO_DIST > ? AND g.INFO_DIST <= ? AND a.NET_DIST >= ? AND a.NET_DIST < ?
                            """, 
                            [INFO_DIST_range_start,INFO_DIST_range_end,NET_DIST_range_start,NET_DIST_range_end])
        
        # query = cur.execute("""
        #                     SELECT COUNT(g.INFO_DIST)
        #                     FROM g_dbv_tau g
        #                     INNER JOIN nl_dbv_tau a ON
        #                     g.SRC = a.SRC AND g.DST = a.DST
        #                     WHERE g.INFO_DIST > ? AND g.INFO_DIST <= ? AND a.INFO_DIST >= ? AND a.INFO_DIST < ?
        #                     """, 
        #                     [INFO_DIST_range_start,INFO_DIST_range_end,NET_DIST_range_start,NET_DIST_range_end])
        
        # query = cur.execute("""
        #                     SELECT COUNT(l.INFO_DIST)
        #                     FROM a_dbv_tau WHERE INFO_DIST > ? AND INFO_DIST <= ? AND NET_DIST >= ? AND NET_DIST < ?
        #                     """, 
        #                     [INFO_DIST_range_start,INFO_DIST_range_end,NET_DIST_range_start,NET_DIST_range_end])
        
        count = query.fetchone()[0]
        if count != 0:
            Z[INFO_DIST_step_count][NET_DIST_step_count] = count/TOTAL_COUNT

max_network_distance = MAX_NET_DIST
network_distance_resolution = 1
network_distances_entry_count = int(max_network_distance / network_distance_resolution) + 1

max_info_distance = MAX_NET_DIST
info_distance_resolution = 0.5
info_distances_entry_count = int(max_info_distance / info_distance_resolution) + 1

X, Y = np.meshgrid(np.linspace(0, max_network_distance, network_distances_entry_count),np.linspace(0, (info_distances_entry_count-1)*info_distance_resolution, info_distances_entry_count))

# plot
fig, ax = plt.subplots()

im = ax.pcolormesh(X, Y, Z, shading="flat", norm="log")
ax.set_xlabel('Network Distance')
ax.set_ylabel('Information Distance')
ax.set_title('Frequency Of Connections By Information And Network Distance')

plt.colorbar(im,ax=ax)

plt.show()
