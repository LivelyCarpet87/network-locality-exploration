#!/usr/bin/python3
import pandas as pd
import re
#import networkx as nx
from igraph import Graph

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
    print("Data Frame Preview:")
    print(data_frame)
    print()
    return data_frame

def parse_via_regex(filename_in:str, pattern:str, order=[0,1,2], unweighted=False):
    contents = open(filename_in).read()
    data_extract_str = re.findall(pattern, contents, flags=0)
    def cast_weight_to_float(row):
        ret = []
        ret.append(int(row[ order[0] ]))
        ret.append(int(row[ order[1] ]))
        if not unweighted:
            weight = 0
            try:
                weight = float(row[ order[2] ])
            except ValueError:
                print("Illegal Non-Float Weight Detected")
                return
            ret.append(weight)
        else:
            ret.append(DEFAULT_UNWEIGHTED_NETWORK_WEIGHT)
        return ret
    
    data_map = map(cast_weight_to_float,data_extract_str)
    data_frame = pd.DataFrame(data_map, columns=["From", "To", "Weight"])
    print("Data Frame Preview:")
    print(data_frame)
    print()
    return data_frame
    
def create_graph(dataframe, directional=False):
    G = Graph.DataFrame(dataframe, directed=directional)
    return G