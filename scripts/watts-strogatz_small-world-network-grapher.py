import matplotlib.pyplot as plt
import helpers.metrics as m
import sqlite3
import helpers.visualize as v

#con = sqlite3.connect(":memory:")
DB_PATH = "./output.db"
con = sqlite3.connect(DB_PATH)
cur = con.cursor()
# Graph
v.graph_conditional_probability(con)
#"""
#m.calculate_geodesic_k_sqlite(con,20,20)
#v.graph_information_neighborhood(con)


con.commit()
