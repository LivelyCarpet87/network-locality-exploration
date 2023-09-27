#include <string>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <random>

#include "utils.h"

#include "edgelist.h"

edgelist edgelist_from_file(bool weighted, std::string filepath){
    edgelist new_edgelist;

    std::ifstream file(filepath);
    std::string line;
    if (!file.is_open()){
        std::cerr << "FAILED TO OPEN FILE!\n";
        exit(1);
    }
    while (std::getline(file, line)) {
        if (weighted){
            int src;
            int dest;
            double weight;
            if (sscanf(line.c_str(), "%d %d %lf", &src, &dest, &weight) == 3){
                new_edgelist.insert_edge(src, dest, weight);
            }
        } else {
            int src;
            int dest;
            if (sscanf(line.c_str(), "%d %d", &src, &dest) == 2){
                new_edgelist.insert_edge(src, dest, 1);
            }
        }
    }
    return new_edgelist;
}

/**
 * Generates a Watts Strogatz small world network into the given edgelist object according to the input parameters.
 * @param edgelist The edgelist (presumed empty) that the Watts Strogatz network will be generated into.
 * @param SIZE The size (vertex count) of the generated network.
 * @param AVG_DEG The average number of edges (undirected network) connected to each vertex in the generated network. Must be even.
 * @param REWIRING_PROB The probability that each edge in the initial network is rewired. Represented as a double between 0 and 1 (inclusive, accurate to 1E-3).
*/
void generate_watts_strogatz_small_world_network(edgelist &edgelist, int SIZE, int AVG_DEG, double REWIRING_PROB){
    std::vector<edge> edges;
    // Set the edgelist to be undirected
    edgelist.set_directional(false);

    assert(SIZE > 0);
    assert(AVG_DEG > 0);
    assert(AVG_DEG % 2 == 0);
    assert(REWIRING_PROB >= 0);
    assert(REWIRING_PROB <= 1);

    // Initialize the random number generators
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib_weight(0, 10E4);
    std::uniform_int_distribution<> distrib_reroll(0, 1000);
    std::uniform_int_distribution<> distrib_reroll_target(0, SIZE);

    
    // Generate the initial edgelist (circular)
    for (int src = 0; src < SIZE; src ++){ // For each node
        for (int offset = 1; offset <= AVG_DEG/2; offset++){ // Target the next AVG_DEG/2 nodes (because half of AVG_DEG point in & half point out if it were directed)
            int dest = (src + offset) % SIZE; // Wrap around if exceeded largest node count
            double weight = distrib_weight(gen)/10E4; // Generate a random edge weight between 0 & 1 (distrib_weight is an int between 0 and 10E4)

            edgelist.insert_edge(src,dest,weight); // Add the edge
        }
    }
    
    // For each exist edge (see above for loop structure explanation)
    for (int src = 0; src < SIZE; src ++){
        for (int offset = 1; offset <= AVG_DEG/2; offset++){
            int dest_org = (src + offset) % SIZE; // This is the original destination of the edge
            double weight = distrib_weight(gen)/10E4; // Generate a new weight (avoid annoyance of retrieving and saving weight, no mathematical significance)

            if (distrib_reroll(gen)/(double)1000 <= REWIRING_PROB){ // Roll for chance to change edge destination
                // Select a new target for the edge
                int new_dest = distrib_reroll_target(gen);
                // While the target for the edge causes a duplicate edge (including former edge destination) or a self edge, reroll the target.
                std::vector<int> current_adjacent = edgelist.get_adjacent_vertices(src);
                while(std::find(current_adjacent.begin(), current_adjacent.end(), new_dest) != current_adjacent.end() || new_dest == src){
                    new_dest = distrib_reroll_target(gen);
                }

                // Once a new edge has been rolled, add the new edge and remove the old edge
                edgelist.insert_edge(src,new_dest,weight);
                edgelist.rm_edge(src,dest_org);
            }
        }
    }
}