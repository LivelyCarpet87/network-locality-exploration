#pragma once

#include <map>

#include "edgelist.h"

namespace metrics {

    // Used to store a pair of information and network distance
    struct distance_pair {
        double info_distance;
        int net_distance;
    };

    // ID of the destination vertex
    typedef int dest_vertex;

    // Information and network distance pairs from a vertex to all other vertices found (within a limit if supplied)
    typedef std::map<
        dest_vertex,
        distance_pair
    > distance_to_vertices;

    // ID of the source (starting) vertex
    typedef int src_vertex;

    // Stores the information and network distance from each vertex (source) to all its connected vertices (dest)
    typedef std::map<
        src_vertex,
        distance_to_vertices
    > distance_btwn_vertices;

    /**
     * Prints the contents of a distance to vertices structure (one to all)
     * @param dtn The targeted distance_to_vertices struct to print
    */
    void print_distance_to_vertices(distance_to_vertices dtn);

    /**
     * Prints the contents of a distance btwn vertices structure (all to all)
     * @param dbn The targeted distance_btwn_vertices struct to print
    */
    void print_distance_to_vertices(distance_btwn_vertices dbv);

    /**
     * Calculates the shortest information distance (sum of edge weights) and network distance (count of edges) prioritized in that order within a maximum of k edges from src
     * @param edgelist The edgelist representing the network edgelist upon which calculations occur.
     * @param src The vertex that distances are measured from.
     * @param k The upper bound of network distance (count of edges traversed) that the function explores.
     * @return A distance_to_vertices struct (one to all) containing the results
    */
    distance_to_vertices geodesic_distance_k(edgelist &edgelist, src_vertex src, int k);

    /**
     * Calculates the shortest information distance (sum of edge weights) and network distance (count of edges) prioritized in that order within a maximum information distance of tau from src
     * @param edgelist The edgelist representing the network edgelist upon which calculations occur.
     * @param src The vertex that distances are measured from.
     * @param tau The upper bound of information distance (sum of edge weights) that the function explores.
     * @return A distance_to_vertices struct (one to all) containing the results
    */
    distance_to_vertices geodesic_distance_tau(edgelist &edgelist, src_vertex src, double tau);

    /**
     * Calculates the distance from all vertices (starting from vertex 0) in the given edgelist within the limit of k
     * @param edgelist The edgelist representing the network edgelist upon which calculations occur.
     * @param k The upper bound of network distance (count of edges traversed) that the function explores.
     * @return A distance_btwn_vertices struct (all to all) containing the results
    */
    distance_btwn_vertices cross_geodesic_distance_k(edgelist &edgelist, int k);

    /**
     * Calculates the distance from all vertices (starting from vertex 0) in the given edgelist within the limit of tau
     * @param edgelist The edgelist representing the network edgelist upon which calculations occur.
     * @param tau The upper bound of information distance (sum of edge weights) that the function explores.
     * @return A distance_btwn_vertices struct (all to all) containing the results
    */
    distance_btwn_vertices cross_geodesic_distance_tau(edgelist &edgelist, double tau);

    /**
     * Saves the contents of a distance_btwn_vertices struct in csv format
     * @param filename Filename (as path) of the output file (assumes file does not exist yet)
     * @param dbv The target distance_btwn_vertices struct to save
    */
    void save_distance_btwn_vertices_to_file(std::string filename, distance_btwn_vertices dbv);

    /**
     * Saves the contents of a distance_btwn_vertices struct in SQLite3 database format
     * @param filename Filename (as path) of the output database file
     * @param table_name The SQLite3 table to store the contents in (assumed to be empty)
     * @param dbv The target distance_btwn_vertices struct to save
    */
    void dbv_to_sqlite(std::string filepath, std::string table_name, distance_btwn_vertices dbv);

    /**
     * Calculates a N_tilda(GAMMA) neighborhood with a given set of negative laplacian and g_tilda edgelists, src vertex, and gamma
     * @param neg_laplacian_edgelist The negative laplacian edgelist
     * @param g_tilda_edglist The G_tilda edgelist
     * @param src The vertex ID of the source vertex the neighborhood is centered around
     * @param gamma The value of Gamma used to calculate the neighborhood
     * @return Returns a distance_to_vertices struct containing the distances to all the vertices within the neighborhood
    */
    metrics::distance_to_vertices n_tilda_gamma_neighborhood(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, int src, const double gamma);

    /**
     * Calculates S_avg(Gamma) for a given set of laplacian and g_tilda edgelists, and gamma
     * @param neg_laplacian_edgelist The negative laplacian edgelist
     * @param g_tilda_edglist The G_tilda edgelist
     * @param gamma The value of Gamma used to calculate the neighborhood
     * @return S_avg
    */
    double s_avg_gamma(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, const double gamma);

    /**
     * Calculates L_Neighborhood_Reduction_Rate(L) for a given set of laplacian and g_tilda edgelists, and L
     * @param neg_laplacian_edgelist The negative laplacian edgelist
     * @param g_tilda_edglist The G_tilda edgelist
     * @param L The value of L used to calculate the neighborhood
     * @param src The source vertex ID the neighborhood is centered around
     * @return L_neighborhood_reduction_rate
    */
    double L_neighborhood_reduction_rate(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, const int L, int src);

    /**
     * Calculates L_Neighborhood_Reduction_Rate_Avg(L) for a given set of laplacian and g_tilda edgelists, and L
     * @param neg_laplacian_edgelist The negative laplacian edgelist
     * @param g_tilda_edglist The G_tilda edgelist
     * @param L The value of L used to calculate the neighborhood
     * @return L_neighborhood_reduction_rate_avg
    */
    double L_neighborhood_reduction_rate_average(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, const int L);
};