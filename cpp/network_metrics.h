#pragma once

#include <map>

#include "edgelist.h"

namespace metrics {
    struct distance_pair {
        double info_distance;
        int net_distance;
    };

    typedef int dest_vertex;

    typedef std::map<
        dest_vertex,
        distance_pair
    > distance_to_vertices;

    typedef int src_vertex;

    typedef std::map<
        src_vertex,
        distance_to_vertices
    > distance_btwn_vertices;

    void print_distance_to_vertices(distance_to_vertices dtn);

    void print_distance_to_vertices(distance_btwn_vertices dbv);

    distance_to_vertices geodesic_distance_k(edgelist &edgelist, src_vertex src, int k);

    distance_to_vertices geodesic_distance_tau(edgelist &edgelist, src_vertex src, double tau);

    distance_btwn_vertices cross_geodesic_distance_k(edgelist &edgelist, int k);

    distance_btwn_vertices cross_geodesic_distance_tau(edgelist &edgelist, double tau);

    void save_distance_btwn_vertices_to_file(std::string filename, distance_btwn_vertices dbv);

    void dbv_to_sqlite(std::string filepath, std::string table_name, distance_btwn_vertices dbv);

    // std::vector<dest_vertex> info_neighborhood(edgelist &edgelist, double tau);

    metrics::distance_to_vertices n_tilda_gamma_neighborhood(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, int src, const double gamma);

    double s_avg_gamma(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, const double gamma);

    double L_neighborhood_reduction_rate(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, const int L, int src);

    double L_neighborhood_reduction_rate_average(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, const int L);
};