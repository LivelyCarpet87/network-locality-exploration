#pragma once

#include "edgelist.h"
#include "network_metrics.h"

/**
 * Loads an edgelist in the "SRC DEST [WEIGHT]" line format from a file
 * @param weighted If the network represented by the edgelist is weighted
 * @param filepath The filepath of the document containing hte edgelist
 * @return An edgelist object
*/
edgelist edgelist_from_file(bool weighted, std::string filepath);

void generate_watts_strogatz_small_world_network(edgelist &edgelist, int SIZE, int AVG_DEG, double REWIRING_PROB);