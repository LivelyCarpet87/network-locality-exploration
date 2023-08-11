#!/usr/bin/python3
import pandas as pd
import re
#import networkx as nx
from igraph import Graph
import numpy as np
import sqlite3

DEFAULT_UNWEIGHTED_NETWORK_WEIGHT = 1

def parse_separated_values(filename_in:str, delimiter=",", remove_header=True, unweighted = False):
    lines = open(filename_in).read().splitlines()
    if remove_header:
        lines.pop(0)
    
    def parse_line(line):
        ret = line.split(delimiter)
        def remove_empty_strings(item):
            return not (type(item) is str and len(item) == 0)
        
        ret = list(filter(remove_empty_strings, ret))
        if unweighted:
            ret.append(DEFAULT_UNWEIGHTED_NETWORK_WEIGHT)
        return ret
    
    data_map = map(parse_line, lines)
    data_frame = pd.DataFrame(data_map, columns=["From", "To", "Weight"])
    return data_frame

def parse_via_regex(filename_in:str, pattern:str, order=[0,1,2], unweighted=False):
    contents = open(filename_in).read()
    return parse_content_via_regex(contents, pattern, order, unweighted)

def parse_content_via_regex(contents, pattern:str, order=[0,1,2], unweighted=False, absolute=True):
    data_extract_str = re.findall(pattern, contents, flags=re.M)
    def cast_weight_to_float(row):
        ret = []
        ret.append(np.int64(row[ order[0] ]))
        ret.append(np.int64(row[ order[1] ]))
        if not unweighted:
            weight = 0
            try:
                weight = np.float64(row[ order[2] ])
            except ValueError:
                print(f"Illegal Non-Float Weight Detected {row[ order[2] ]}")
                return
            if absolute:
                weight = abs(weight)
            ret.append(weight)
        else:
            ret.append(DEFAULT_UNWEIGHTED_NETWORK_WEIGHT)
        return ret
    
    data_map = map(cast_weight_to_float,data_extract_str)
    data_frame = pd.DataFrame(data_map, columns=["From", "To", "Weight"])
    return data_frame
    
def create_graph(dataframe, directional=False):
    G = Graph.DataFrame(dataframe, directed=directional)
    return G

def parse_via_regex_sqlite(filename_in:str, pattern:str, order=[0,1,2], unweighted=False):
    contents = open(filename_in).read()
    return parse_content_via_regex_sqlite(contents, pattern, order, unweighted)

def parse_content_via_regex_sqlite(contents, pattern:str, order=[0,1,2], unweighted=False, absolute=True):
    con = sqlite3.connect(":memory:")
    #con = sqlite3.connect("./debug.db")

    cur = con.cursor()
    cur.execute("CREATE TABLE edgelist(source INT, destination INT, weight FLOAT)")
    con.commit()

    data_extract_str = re.findall(pattern, contents, flags=re.M)
    for row in data_extract_str:
        ret = []
        ret.append(int(row[ order[0] ]))
        ret.append(int(row[ order[1] ]))
        if not unweighted:
            weight = 0
            try:
                weight = np.float64(row[ order[2] ])
            except ValueError:
                print(f"Illegal Non-Float Weight Detected {row[ order[2] ]}")
                return
            if absolute:
                weight = abs(weight)
            ret.append(weight)
        else:
            ret.append(DEFAULT_UNWEIGHTED_NETWORK_WEIGHT)
        cur.execute("INSERT INTO edgelist VALUES(?, ?, ?)", ret)
    con.commit()
    return con