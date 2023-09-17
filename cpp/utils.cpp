#include <string>
#include <iostream>
#include <fstream>

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