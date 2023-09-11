#pragma once

#include <iostream>
#include <stdexcept>
#include <map>
#include <vector>

struct edge {
    int src;
    int dest;
    double weight;
};

class edgelist {
    private:
        std::map<
            int,
            std::map<
                int, 
                std::vector<double>
            >
        > Edges;
        std::map<
            int,
            std::map<
                int, 
                std::vector<double>
            >
        > RevEdges;
        bool Directional;
    public:
        //
        // constructor:
        //
        edgelist();

        edgelist(bool isDirectional);

        void update_edge(int src, int dest, double weight);

        void rm_edge(int src, int dest);

        void set_directional(bool directional);

        std::vector<edge> get_edges();

        std::vector<edge> get_edges_duplicate_on_undirectional();

        std::vector<edge> get_edges(int src);

        std::vector<int> get_adjacent_vertices(int src);

        std::string to_string();

        std::vector<double> get_edge_weights(int src, int dest);

        int max_vertex();

        edgelist take_neg_laplacian();

        edgelist neg_laplacian_to_g();

        void save_edgelist_to_sqlite(std::string filepath, std::string table_name);
};