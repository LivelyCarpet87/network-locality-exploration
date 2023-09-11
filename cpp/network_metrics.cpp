#include <queue>
#include <vector>
#include <limits>
#include <cmath>
#include <cfloat>
#include <iostream>
#include <fstream>
#include <sqlite3.h>
#include <omp.h>
#include <array>
#include <algorithm>

#include "edgelist.h"
#include "funcs.h"

#include "network_metrics.h"

const double MINIMAL_PERCENT_ROUNDING_ERR = 0.00001;

void metrics::print_distance_to_vertices(metrics::distance_to_vertices dtv){
    for (auto iter : dtv){
        std::cout << "-> " << iter.first << " = " << "INFO:" << iter.second.info_distance << " | " << "NET:" << iter.second.net_distance << "\n";
    }
}

void metrics::print_distance_to_vertices(metrics::distance_btwn_vertices dbv){
    for (auto iter : dbv){
        metrics::src_vertex from = iter.first;
        metrics::distance_to_vertices connections = iter.second;
        for (auto connection : connections){
            metrics::dest_vertex to = connection.first;
            metrics::distance_pair dp = connection.second;
            std::cout << from << " -> " << to << " = " << "INFO:" << dp.info_distance << " | " << "NET:" << dp.net_distance << "\n";
        }
    }
}

metrics::distance_to_vertices metrics::geodesic_distance_k(edgelist &edgelist, metrics::src_vertex src, int k){
    metrics::distance_to_vertices dtv; // Distance to vertex
    std::queue<int> priority_queue; // queue representing the frontier

    // Initialize the src vertex distances to zero
    dtv[src].info_distance = 0;
    dtv[src].net_distance = 0;

    priority_queue.push(src); // Add src vertex to frontier

    while (!priority_queue.empty()){ // While frontier not empty
        // Pop front of frontier to test adjacent endges
        metrics::src_vertex from = priority_queue.front();
        priority_queue.pop();

        // Preload the distances for the frontier vertex
        double from_info_distance = dtv.at(from).info_distance;
        int from_net_distance = dtv.at(from).net_distance;

        // Iterate through adjacent edges
        std::vector<edge> edges = edgelist.get_edges(from);
        for (struct edge edge: edges){
            // Get edge destination and weight
            metrics::src_vertex to = edge.dest;
            double weight = edge.weight;
            
            // initialize distances to infinity if no known distance exists or load existing distances
            double current_to_info_distance;
            int current_to_net_distance;
            if (dtv.find(to) != dtv.end()){
                current_to_info_distance = dtv.at(to).info_distance;
                current_to_net_distance = dtv.at(to).net_distance;
            } else {
                current_to_info_distance = INFINITY;
                current_to_net_distance = INT32_MAX;
            }

            // Calculate the achieved distances passing through the edge being tested
            double possible_to_info_distance = from_info_distance + weight;
            int possible_to_net_distance = from_net_distance + 1;

            // Test that passing through the edge meets the condition
            if (from_net_distance == k){
                continue;
            }

            // Update the data if passing the edge leads to a shorter distance (information > network)
            if (current_to_info_distance - possible_to_info_distance > possible_to_info_distance*MINIMAL_PERCENT_ROUNDING_ERR){
                metrics::distance_pair dtp = {
                    .info_distance = possible_to_info_distance,
                    .net_distance = possible_to_net_distance
                };
                dtv[to] = dtp;
                priority_queue.push(to); // If updated, add to the frontier

            } else if (std::abs(current_to_info_distance - possible_to_info_distance) < possible_to_info_distance*MINIMAL_PERCENT_ROUNDING_ERR && current_to_net_distance > possible_to_net_distance){
                dtv.at(to).net_distance = possible_to_net_distance;
                priority_queue.push(to);
            }
        }
    }

    return dtv;
}

metrics::distance_to_vertices metrics::geodesic_distance_tau(edgelist &edgelist, metrics::src_vertex src, double tau){
    metrics::distance_to_vertices dtv; // Distance to vertex
    std::queue<int> priority_queue; // queue representing the frontier

    // Initialize the src vertex distances to zero
    dtv[src].info_distance = 0;
    dtv[src].net_distance = 0;

    priority_queue.push(src); // Add src vertex to frontier

    while (!priority_queue.empty()){ // While frontier not empty
        // Pop front of frontier to test adjacent endges
        metrics::src_vertex from = priority_queue.front();
        priority_queue.pop();

        // Preload the distances for the frontier vertex
        double from_info_distance = dtv.at(from).info_distance;
        int from_net_distance = dtv.at(from).net_distance;

        // Iterate through adjacent edges
        std::vector<edge> edges = edgelist.get_edges(from);
        for (struct edge edge: edges){
            // Get edge destination and weight
            metrics::src_vertex to = edge.dest;
            double weight = edge.weight;
            
            // initialize distances to infinity if no known distance exists or load existing distances
            double current_to_info_distance;
            int current_to_net_distance;
            if (dtv.find(to) != dtv.end()){
                current_to_info_distance = dtv.at(to).info_distance;
                current_to_net_distance = dtv.at(to).net_distance;
            } else {
                current_to_info_distance = INFINITY;
                current_to_net_distance = INT32_MAX;
            }

            // Calculate the achieved distances passing through the edge being tested
            double possible_to_info_distance = from_info_distance + weight;
            int possible_to_net_distance = from_net_distance + 1;

            // Test that passing through the edge meets the condition
            if (possible_to_info_distance > tau){
                continue;
            }

            // Update the data if passing the edge leads to a shorter distance (information > network)
            if (current_to_info_distance - possible_to_info_distance > possible_to_info_distance*MINIMAL_PERCENT_ROUNDING_ERR){
                metrics::distance_pair dtp = {
                    .info_distance = possible_to_info_distance,
                    .net_distance = possible_to_net_distance
                };
                dtv[to] = dtp;
                priority_queue.push(to); // If updated, add to the frontier
            } else if (std::abs(current_to_info_distance - possible_to_info_distance) < possible_to_info_distance*MINIMAL_PERCENT_ROUNDING_ERR  && current_to_net_distance > possible_to_net_distance){
                dtv.at(to).net_distance = possible_to_net_distance;
                priority_queue.push(to);
            }
        }
    }

    return dtv;
}

metrics::distance_btwn_vertices metrics::cross_geodesic_distance_k(edgelist &edgelist, int k){
    metrics::distance_btwn_vertices dbn;
    int dim = edgelist.max_vertex();
    metrics::distance_to_vertices* res = new metrics::distance_to_vertices[dim+1];
    #pragma omp parallel for
    for (int src = 0; src <= dim; src++){
        res[src] = geodesic_distance_k(edgelist, src, k);
    }

    for (int src = 0; src <= dim; src++){
        dbn[src] = res[src];
    }
    delete[] res;
    return dbn;
}

metrics::distance_btwn_vertices metrics::cross_geodesic_distance_tau(edgelist &edgelist, double tau){
    metrics::distance_btwn_vertices dbn;
    int dim = edgelist.max_vertex();
    metrics::distance_to_vertices* res = new metrics::distance_to_vertices[dim+1];
    #pragma omp parallel for
    for (int src = 0; src <= dim; src++){
        res[src] = geodesic_distance_tau(edgelist, src, tau);
    }

    for (int src = 0; src <= dim; src++){
        dbn[src] = res[src];
    }
    delete[] res;
    return dbn;
}

void metrics::save_distance_btwn_vertices_to_file(std::string filepath, metrics::distance_btwn_vertices dbv){
    std::ofstream output_file;
    output_file.open(filepath);
    output_file << "FROM, TO, INFO_DISTANCE, NET_DISTANCE" << "\n";
    for (auto iter : dbv){
        metrics::src_vertex from = iter.first;
        metrics::distance_to_vertices connections = iter.second;
        for (auto connection : connections){
            metrics::dest_vertex to = connection.first;
            metrics::distance_pair dp = connection.second;
            output_file << from << ", " << to << ", " << dp.info_distance << ", " << dp.net_distance << "\n";
        }
    }
    output_file.close();
}

void metrics::dbv_to_sqlite(std::string filepath, std::string table_name, metrics::distance_btwn_vertices dbv){
    sqlite3 *db;
    if (sqlite3_open(filepath.c_str(), &db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    std::string sql_create_table_query = "CREATE TABLE IF NOT EXISTs " \
     + table_name + "("  \
      "SRC          INT     NOT NULL," \
      "DST          INT     NOT NULL," \
      "INFO_DIST    REAL    NOT NULL," \
      "NET_DIST     INT     NOT NULL    );";

    char *zErrMsg = 0;
    if( sqlite3_exec(db, sql_create_table_query.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(1);
    }

    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
    for (auto iter : dbv){
        metrics::src_vertex from = iter.first;
        metrics::distance_to_vertices connections = iter.second;
        for (auto connection : connections){
            metrics::dest_vertex to = connection.first;
            metrics::distance_pair dp = connection.second;
            std::string sql_insert_query = "INSERT INTO "+table_name+" (SRC,DST,INFO_DIST,NET_DIST) " \
            "VALUES (" + std::to_string(from) + ", " + std::to_string(to) + ", " + std::to_string(dp.info_distance) + ", " + std::to_string(dp.net_distance) + ")\n";
            if (sqlite3_exec(db, sql_insert_query.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK){
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }
        }
    }
    sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
    sqlite3_close(db);
}

metrics::distance_to_vertices metrics::n_tilda_gamma_neighborhood(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, int src, const double gamma){
    metrics::distance_to_vertices dtv; // Distance to vertex
    std::queue<int> priority_queue; // queue representing the frontier
    double MU;
    double KAPPA;

    // Initialize the src vertex distances to zero
    dtv[src].info_distance = 0;
    dtv[src].net_distance = 0;

    // Initialize MU & KAPPA
    MU = -INFINITY;
    KAPPA = -INFINITY;
    std::vector<edge> neg_laplacian_edges = neg_laplacian_edgelist.get_edges();
    for (edge edge: neg_laplacian_edges){
        double abs_weight = std::abs(edge.weight);

        if (abs_weight > KAPPA){
            KAPPA = abs_weight;
        }
        if ( (edge.src == src || edge.dest == src) && abs_weight > MU){
            MU = abs_weight;
        }
    }
    KAPPA *= funcs::v_func(funcs::EPSILON);

    // Threshold to stop using inverse approximation w_func and start using v_func
    double max_approx_x;
    //double max_approx_y;
    auto threshold = funcs::max_approximation_threshold_w(KAPPA/(gamma * MU));
    max_approx_x = threshold.first;
    //max_approx_y = threshold.second;

    #ifdef _DEBUG_N_tilda_gamma_neighborhood
    std::cout << "max_approx_y:" << max_approx_y <<" VS " << KAPPA/(gamma * MU) << "\n";
    #endif

    priority_queue.push(src); // Add src vertex to frontier

    while (!priority_queue.empty()){ // While frontier not empty
        // Pop front of frontier to test adjacent endges
        metrics::src_vertex from = priority_queue.front();
        priority_queue.pop();

        // Preload the distances for the frontier vertex
        double from_info_distance = dtv.at(from).info_distance;
        int from_net_distance = dtv.at(from).net_distance;

        // Iterate through adjacent edges
        std::vector<edge> edges = g_tilda_edgelist.get_edges(from);
        for (struct edge edge: edges){
            // Get edge destination and weight
            metrics::src_vertex to = edge.dest;
            double weight = edge.weight;
            
            // initialize distances to infinity if no known distance exists or load existing distances
            double current_to_info_distance;
            int current_to_net_distance;
            if (dtv.find(to) != dtv.end()){
                current_to_info_distance = dtv.at(to).info_distance;
                current_to_net_distance = dtv.at(to).net_distance;
            } else {
                current_to_info_distance = INFINITY;
                current_to_net_distance = INT32_MAX;
            }

            // Calculate the achieved distances passing through the edge being tested
            double possible_to_info_distance = from_info_distance + weight;
            int possible_to_net_distance = from_net_distance + 1;

            // Test that passing through the edge meets the condition
            if (possible_to_info_distance < max_approx_x){
                // Meets the smaller than approximation condition
            } else if (KAPPA / funcs::v_func(possible_to_info_distance) > gamma * MU) {
                // Meets the v_func inequality condition
            } else {
                continue;
            }

            // Update the data if passing the edge leads to a shorter distance (information > network)
            if (current_to_info_distance - possible_to_info_distance > possible_to_info_distance*MINIMAL_PERCENT_ROUNDING_ERR){
                metrics::distance_pair dtp = {
                    .info_distance = possible_to_info_distance,
                    .net_distance = possible_to_net_distance
                };
                dtv[to] = dtp;
                priority_queue.push(to); // If updated, add to the frontier
            } else if (std::abs(current_to_info_distance - possible_to_info_distance) < possible_to_info_distance*MINIMAL_PERCENT_ROUNDING_ERR  && current_to_net_distance > possible_to_net_distance){
                dtv.at(to).net_distance = possible_to_net_distance;
                priority_queue.push(to);
            }
        }
    }

    return dtv;
}

double metrics::s_avg_gamma(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, const double gamma){
    int dim = neg_laplacian_edgelist.max_vertex();
    metrics::distance_to_vertices* res = new metrics::distance_to_vertices[dim+1];
    #pragma omp parallel for
    for (int src = 0; src <= dim; src++){
        res[src] = n_tilda_gamma_neighborhood(neg_laplacian_edgelist, g_tilda_edgelist, src, gamma);
    }
    double total_size = 0;
    for (int src = 0; src <= dim; src++){
        total_size += res[src].size();
    }
    delete[] res;
    return total_size / (dim+1);
}

double metrics::L_neighborhood_reduction_rate(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, const int L, int src) {
    // Can be at most (L-1) edges away
    metrics::distance_to_vertices dtv = metrics::geodesic_distance_k(g_tilda_edgelist,src,L-1);

    std::vector<double> distances;
    for (auto iter : dtv){
        distances.push_back(iter.second.info_distance);
    }
    sort(distances.begin(), distances.end());

    // Get the max distance within L vertices
    double max_distance = distances.at(std::min(L-1,int(distances.size()-1)));

    // Initialize MU & KAPPA
    double MU = -INFINITY;
    double KAPPA = -INFINITY;
    std::vector<edge> neg_laplacian_edges = neg_laplacian_edgelist.get_edges();
    for (edge edge: neg_laplacian_edges){
        double abs_weight = std::abs(edge.weight);

        if (abs_weight > KAPPA){
            KAPPA = abs_weight;
        }
        if ( (edge.src == src || edge.dest == src) && abs_weight > MU){
            MU = abs_weight;
        }
    }
    if (MU == 0){
        return 0;
    }
    KAPPA *= funcs::v_func(funcs::EPSILON);
    double res = KAPPA / (funcs::v_func(max_distance) * MU);
    if (res == INFINITY){
        std::cerr << "L_neighborhood_reduction_rate resulted in infinity\n";
        std::cerr << funcs::v_func(max_distance) <<"\n";
        std::cerr << KAPPA << "/" << MU <<"\n";
        return 0;
    }
    return res;
}

double metrics::L_neighborhood_reduction_rate_average(edgelist &neg_laplacian_edgelist, edgelist &g_tilda_edgelist, const int L) {
    int dim = neg_laplacian_edgelist.max_vertex();
    double* res = new double[dim+1];
    #pragma omp parallel for
    for (int src = 0; src <= dim; src++){
        res[src] = L_neighborhood_reduction_rate(neg_laplacian_edgelist, g_tilda_edgelist, L, src);
    }
    long double total = 0;
    for (int src = 0; src <= dim; src++){
        total += res[src];
    }
    delete[] res;
    if (total == INFINITY){
        std::cerr << "Total Overflowed\n";
        exit(1);
    }
    return total / (dim+1);
}