#pragma once

#include <iostream>
#include <stdexcept>
#include <map>
#include <vector>

// Represents an edge (can be directed depending on edgelist)
struct edge {
    int src;
    int dest;
    double weight;
};

class edgelist {
    private:
        // Stores all the edges in the positive direction
        std::map<
            int,
            std::map<
                int, 
                std::vector<double>
            >
        > Edges;
        // Stores all the edges in reverse direction for faster lookup
        std::map<
            int,
            std::map<
                int, 
                std::vector<double>
            >
        > RevEdges;
        // Represents whether if the network represented by the edgelist is directional
        bool Directional;
    public:
        // Construct an empty default (undirected) edgelist
        edgelist();

        /**
         * Construct an empty edgelist with the given directionality
         * @param isDirectional Whether if the network represented by the edgelist is directional
        */
        edgelist(bool isDirectional);

        /**
         * Insert an edge into the edgelist (can cause duplicate edges)
         * @param src The source vertex ID the edge points from. Relevant in directed networks.
         * @param dest The destination vertex ID the edge points to. Relevant in directed networks.
         * @param weight The weight of the edge
        */
        void insert_edge(int src, int dest, double weight);

        /**
         * Removes an edge from the edgelist (only if it exists)
         * @param src The source vertex ID the edge points from. Relevant in directed networks.
         * @param dest The destination vertex ID the edge points to. Relevant in directed networks.
        */
        void rm_edge(int src, int dest);

        /**
         * Sets the directionality of the network represented by the edgelist
         * @param directional Whether if the network is directional
        */
        void set_directional(bool directional);

        // Returns the edgelist as a vector of edges
        std::vector<edge> get_edges();

        // Returns the edgelist as a vector of edges. Duplicates the edgelist to return edges in reverse direction if network is undirectional.
        std::vector<edge> get_edges_duplicate_on_undirectional();

        // Returns a vector of edges originating from the given vertex
        std::vector<edge> get_edges(int src);

        // Returns a vector of vertex IDs adjacent to the given vertex
        std::vector<int> get_adjacent_vertices(int src);

        // Returns the edgelsit as a string
        std::string to_string();

        /**
         * Returns a vector containing all the weights of all the edges from source to destination
         * @param src ID of the source vertex. Relevant in directed networks.
         * @param dest ID of the destination vertex. Relevant in directed networks.
         * @return A vector containing the weights of all the relevant edges.
        */
        std::vector<double> get_edge_weights(int src, int dest);

        // Returns the largest vertex ID in the edgelist
        int max_vertex();

        // Returns an edgelist object representing the negative laplacian of the edgelist
        edgelist take_neg_laplacian();

        // Returns an edgelist object representing the g_tilda edgelist assuming it is run on the negative laplacian edgelist
        edgelist neg_laplacian_to_g();

        /**
         * Saves the edgelist executed into an SQLite3 database
         * @param filepath Filepath of the database file
         * @param table_name The name of the table to save the edgelist into
        */
        void save_edgelist_to_sqlite(std::string filepath, std::string table_name);

        /**
         * Saves the edgelist into the given filepath as plaintext
         * @param filepath Filepath of the text output file
        */
        void save_edgelist_as_plaintext(std::string filepath);
};