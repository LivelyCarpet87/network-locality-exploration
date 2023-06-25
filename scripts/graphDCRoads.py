#!/usr/bin/python3
import helpers.format as f
import helpers.visualize as v

df = f.parse_via_regex("datasets/Transport/DC.tmp", r"(\d+) (\d+)\n[\d.]+ ([\d.]+) [\d.]+")
v.graph(df.to_numpy())