# Overview
The code for this project is split primarily into two sections. The computational section of the code is written in C++ with multithreading for increased performance (at least x100 speed on a laptop) and the visualization section of the code is written in Python3. Most tasks will be done by first running the C++ binary to generate results (stored in SQLite3) then visualized by a corresponding Python3 script.

# Setup
## Setting Up Python3 Environment
Python3 dependencies can be found under requirements.txt
`pip3 install -r requirements.txt`
## Compiling The C++ Binary
### Dependencies
The C++ binary relies on OpenMP for multithreading and SQLite3 for data persistence.
### Compiling
- For multithreaded build (recommended):
    - `cd cpp`
    - `make build`
- For debug build:
    - `cd cpp`
    - `make debug`
# Usage
The format of the command follows this format: `task.bin [EDGELIST SOURCE] [EDGELIST OPTIONS] [ACTION] [ACTION OPTIONS]`

## Edgelist Options
These options define the edgelist being processed. It can either be a generated Watts Strogatz network or loaded from a file.

### Generate a Watts Strogatz network
To generate, set `[EDGELIST SOURCE]` to `gen_watts_strogatz` and `[EDGELIST OPTIONS]` becomes `[SIZE] [AVG DEG] [REWIRING PROBABILITY]`.

Eg. To use a Watts Strogatz network with 1000 vertices, an average degree of 20, and a reqiring probability of 10%, the command would be `task.bin gen_watts_strogatz 1000 20 0.1 [ACTION] [ACTION OPTIONS]`.

### Load an edgelist from file
Set `[EDGELIST SOURCE]` to `load_file` and `[EDGELIST OPTIONS]` to `[FILEPATH] [WEIGHTED] [DIRECTIONAL]`.

Eg. To use a load an edgelist located at `./data/out.data` that is weighted (True => 1) but represents an undirected network (False => 0), the command would be `task.bin load_file ./data/out.data 1 0 [ACTION] [ACTION OPTIONS]`.

## Actions
### Convert To ~G
Set `[ACTION]` to `convert_g_tilda` and `[ACTION OPTIONS]` to the filepath where you want the output edgelist to be stored.
### Calculate Distances From Vertex To Other Vertices (With Limit k or Tau)
Set `[ACTION]` to `dtv_k` or `dtv_tau` and `[ACTION OPTIONS]` to `[SRC] [k]` or `[SRC] [tau]`. Eg: Distances from `Vertex 1` on a network loaded from a file with limit $k=10$ would be `task.bin load_file ./data/out.data 1 0 dtv_k 1 10`.
### Calculate Distances Between All Vertices (With Limit k or Tau)
Set `[ACTION]` to `dbv_k` or `dbv_tau` and `[ACTION OPTIONS]` to `[k]` or `[tau]`. Eg: Distances between vertices in a network loaded from a file wiht limit $tau=7.8$ would be `task.bin load_file ./data/out.data 1 0 dbv_tau 7.8`.
### Calculate Y-Neighborhood Size Avg, S_avg(gamma)
Set `[ACTION]` to `s_avg` and `[ACTION OPTIONS]` to `[GAMMA]`. Eg:
### Calculate Avg R-Neighborhood Reduction Rate, R_avg(L)
Set `[ACTION]` to `r_avg` and `[ACTION OPTIONS]` to `[L]`. Eg.

# Extending The Code
The files in this project are organized as follows:
- `/cpp` contains all the source C++ files for the binary and its Makefile
    - `edgelist.cpp` implementation of the Edgelist class used, its helper functions, and conversions
    - `funcs.cpp` calculation of the function v(x)=y and the approximation of its inverse x=w(y) via linear interpolation. The relevant constants such as $\epsilon$ are also defined here.
    - `network_metrics.cpp` calculation of metrics such as distances between vertices, gamma neighborhoods, S_avg, and L Reduction Neighborhoods
    - `utils.cpp` utility functions. Code for loading edgelists from file.
    - `main.cpp` central logic for generating the data visualized. Contains each task as a function.
    - `Makefile` compilation commands
- `/scripts` Contains the Python3 scripts used
    -  `/helpers` Contains the helper functions used in the visualization code

# Vestigial Code
This project was originally written completely in Python3. Due to performance limitations from not using libraries to do the heavy lifting, the code was then partially rewritten into SQLite3 query language. Nevertheless, the code was eventually completely rewritten in C++ to clean it up. As a result, some of the Pythonic code archived was not used in the final product, will not run in a reasonable time frame, and are kept for reference.