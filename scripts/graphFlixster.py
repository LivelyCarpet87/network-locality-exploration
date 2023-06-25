#!/usr/bin/python3
import helpers.format as f
import helpers.visualize as v

df = f.parse_separated_values("datasets/Social/flixster/out.flixster", delimiter=" ", unweighted=True)
v.graph(df.to_numpy())