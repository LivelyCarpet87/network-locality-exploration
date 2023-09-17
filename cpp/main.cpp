#include "edgelist.h"
#include "utils.h"
#include "network_metrics.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <random>
#include <omp.h>
#include <sqlite3.h>
#include <string>
#include <assert.h>

/* 
Manual test network:
adds the following edges to the edgelist passed into the network and sets it as undirected.
You can visualize the network using https://graphonline.ru/en/
    1-(1.2)-2
    2-(0.7)-3
    3-(0.9)-4
    4-(0.1)-5
    5-(1.6)-6
    6-(1.3)-7
    7-(0.85)-1
    1-(0.7)-5
    2-(0.3)-6
    3-(0.8)-7
    4-(0.8)-1
    5-(1.6)-8
*/
void manual_test_network(edgelist &edgelist){
    edgelist.insert_edge(1,2,1.2);
    edgelist.insert_edge(2,3,0.7);
    edgelist.insert_edge(3,4,0.9);
    edgelist.insert_edge(4,5,0.1);
    edgelist.insert_edge(5,6,1.6);
    edgelist.insert_edge(6,7,1.3);
    edgelist.insert_edge(7,1,0.85);
    edgelist.insert_edge(1,5,0.7);
    edgelist.insert_edge(2,6,0.3);
    edgelist.insert_edge(3,7,0.8);
    edgelist.insert_edge(4,1,0.8);
    edgelist.insert_edge(5,8,1.6);

    edgelist.set_directional(false);
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

/**
 * Generates a Watts Strogatz network with SIZE 1,000 AVG_DEG 20 and REWIRING_PROB of 10% and compute various distances.
 * Save the information and network distances between all vertex pairs for the original, neg-laplacian, and g-tilda edgelist into a SQLite3 database.
 * ---
 * Creates the following tables:
 * - `a_edgelist`: Edgelist representing the generated network
 * - `neg_laplacian_edgelist`: Edgelist representing the calculated Negative Laplacian
 * - `g_edgelist Edgelist`: representing the g_tilda network
 * - `a_dbv_tau`: Information and network distances between all vertex pairs for a_edgelist using limit tau = 1000
 * - `a_dbv_k`: Information and network distances between all vertex pairs for a_edgelist using limit k = 10
 * - `nl_dbv_tau`: Information and network distances between all vertex pairs for neg_laplacian_edgelist using limit tau = 1000
 * - `g_dbv_tau`: Information and network distances between all vertex pairs for g_edgelist using limit tau = 1000
 * - `g_dbv_k`: Information and network distances between all vertex pairs for g_edgelist using limit k = 10
 * @param OUTPUT_DB File path of the database to be stored into.
*/
void watts_strogatz_generation_task(std::string OUTPUT_DB) {
    edgelist a_edgelist;
    generate_watts_strogatz_small_world_network(a_edgelist,1000,20,0.1);
    a_edgelist.save_edgelist_to_sqlite(OUTPUT_DB,"a_edgelist");

    edgelist neg_laplacian = a_edgelist.take_neg_laplacian();
    neg_laplacian.save_edgelist_to_sqlite(OUTPUT_DB,"neg_laplacian_edgelist");

    edgelist g_edgelist = neg_laplacian.neg_laplacian_to_g();
    g_edgelist.save_edgelist_to_sqlite(OUTPUT_DB,"g_edgelist");

    //std::cout << "a_edgelist geodesic_tau\n";
    metrics::distance_btwn_vertices a_dbv_tau = metrics::cross_geodesic_distance_tau(a_edgelist, 1000);
    //metrics::print_distance_to_vertices(a_dbv_tau);
    metrics::dbv_to_sqlite(OUTPUT_DB, "a_dbv_tau", a_dbv_tau);
    std::cout << "a_edgelist geodesic_k\n";
    metrics::distance_btwn_vertices a_dbv_k = metrics::cross_geodesic_distance_k(a_edgelist, 10);
    //metrics::print_distance_to_vertices(a_dbv_k);
    //save_distance_btwn_vertices_to_file("a_dbv_k.csv", a_dbv_k);
    metrics::dbv_to_sqlite(OUTPUT_DB, "a_dbv_k", a_dbv_k);

    std::cout << "neg_laplacian geodesic_tau\n";
    metrics::distance_btwn_vertices nl_dbv_tau = metrics::cross_geodesic_distance_tau(neg_laplacian, 1000);
    //metrics::print_distance_to_vertices(nl_dbv_tau);
    metrics::dbv_to_sqlite(OUTPUT_DB, "nl_dbv_tau", nl_dbv_tau);

    std::cout << "g_edgelist geodesic_tau\n";
    metrics::distance_btwn_vertices g_dbv_tau = metrics::cross_geodesic_distance_tau(g_edgelist, 10000);
    //metrics::print_distance_to_vertices(g_dbv_tau);
    metrics::dbv_to_sqlite(OUTPUT_DB, "g_dbv_tau", g_dbv_tau);
    std::cout << "g_edgelist geodesic_k\n";
    metrics::distance_btwn_vertices g_dbv_k = metrics::cross_geodesic_distance_k(g_edgelist, 10);
    //metrics::print_distance_to_vertices(g_dbv_k);
    //save_distance_btwn_vertices_to_file("g_dbv_k.csv", g_dbv_k);
    metrics::dbv_to_sqlite(OUTPUT_DB, "g_dbv_k", g_dbv_k);
}

/**
 * Loads a network from a given file and calculates the Gamma Neighborhood S_avg to be saved in the results SQLite3 db under a task id.
 * @param ID The ID to save the results under
 * @param FILEPATH The path to the file storing the edgelist in the format of "src dest [weight]" each line
 * @param WEIGHTED Whether if the network in the edgelist file is weighted
 * @param DIRECTED Whether if the network in the edgelist is directed
*/
void process_network_s_avg(std::string ID, std::string FILEPATH, bool WEIGHTED, bool DIRECTED){
    // Initialize database
    sqlite3 *db;
    char *zErrMsg = 0;
    if (sqlite3_open("results.db", &db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // load network file
    std::cout << ID <<";" << FILEPATH << ";"<< std::to_string(WEIGHTED) << ";"<< std::to_string(DIRECTED) << "\n";
    edgelist a_edgelist = edgelist_from_file(WEIGHTED, FILEPATH);
    a_edgelist.set_directional(DIRECTED);
    edgelist neg_laplacian = a_edgelist.take_neg_laplacian();
    edgelist g_edgelist = neg_laplacian.neg_laplacian_to_g();
    //std::cout << neg_laplacian.to_string();

    std::string sql_create_s_avg_table_query = "CREATE TABLE IF NOT EXISTs " \
      "S_average ("  \
      "NET_ID          TEXT     NOT NULL," \
      "GAMMA          REAL     NOT NULL," \
      "avg_s    REAL    NOT NULL);";
    if( sqlite3_exec(db, sql_create_s_avg_table_query.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK ){
        fprintf(stderr, "Table Creation SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(1);
    }
    
    // Calculate S_Avg
    double S_avg = metrics::s_avg_gamma(neg_laplacian, g_edgelist, 0.05);
    std::string sql_insert_query = "INSERT INTO S_average (NET_ID,GAMMA,avg_s) " \
    "VALUES ('" + ID + "', 0.05, " + std::to_string(S_avg) + ")\n";

    if (sqlite3_exec(db, sql_insert_query.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK){
        fprintf(stderr, "Insertion SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

/**
 * Loads a network from a given file and calculates the L_neighborhood_reduction_avg to be saved in the results SQLite3 db under a task id.
 * @param ID The ID to save the results under
 * @param FILEPATH The path to the file storing the edgelist in the format of "src dest [weight]" each line
 * @param WEIGHTED Whether if the network in the edgelist file is weighted
 * @param DIRECTED Whether if the network in the edgelist is directed
*/
void process_network_l_neighborhood_reduction(std::string ID, std::string FILEPATH, bool WEIGHTED, bool DIRECTED){
    // Initialize database
    sqlite3 *db;
    if (sqlite3_open("results.db", &db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    std::string sql_create_l_neigborhood_reduction_avg_table_query = "CREATE TABLE IF NOT EXISTs " \
      "L_neighborhood_reduction_average ("  \
      "NET_ID          TEXT     NOT NULL," \
      "L          INT     NOT NULL," \
      "avg_lnr    REAL    NOT NULL);";
    char *zErrMsg = 0;
    if( sqlite3_exec(db, sql_create_l_neigborhood_reduction_avg_table_query.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK ){
        fprintf(stderr, "Table Creation SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(1);
    }

    // load file
    std::cout << ID <<";" << FILEPATH << ";"<< std::to_string(WEIGHTED) << ";"<< std::to_string(DIRECTED) << "\n";
    edgelist a_edgelist = edgelist_from_file(WEIGHTED, FILEPATH);
    a_edgelist.set_directional(DIRECTED);
    edgelist neg_laplacian = a_edgelist.take_neg_laplacian();
    edgelist g_edgelist = neg_laplacian.neg_laplacian_to_g();
    //std::cout << neg_laplacian.to_string();

    // Calculate Avg L Reduction Neighborhood
    for (int L = 1; L <=100; L++){
        double avg_lnr = metrics::L_neighborhood_reduction_rate_average(neg_laplacian, g_edgelist, L);
        std::string sql_insert_query = "INSERT INTO L_neighborhood_reduction_average (NET_ID,L,avg_lnr) " \
        "VALUES ('" + ID + "', " + std::to_string(L) + ", " + std::to_string(avg_lnr) + ")\n";

        if (sqlite3_exec(db, sql_insert_query.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK){
            fprintf(stderr, "Insertion SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
}

/**
 * Calculates & prints the Gamma Neighborhood S_avg for a given Watts_Strogatz network with the given configuration.
 * @param SIZE The size (vertex count) of the generated network.
 * @param AVG_DEG The average number of edges (undirected network) connected to each vertex in the generated network. Must be even.
 * @param REWIRING_PROB The probability that each edge in the initial network is rewired. Represented as a double between 0 and 1 (inclusive, accurate to 1E-3).
*/
void watts_strogatz_small_world_network_gamma_neighborhood_task(int SIZE, int AVG_DEG, double REWIRING_PROB){
    edgelist a_edgelist;
    generate_watts_strogatz_small_world_network(a_edgelist,SIZE,AVG_DEG,REWIRING_PROB);

    edgelist neg_laplacian = a_edgelist.take_neg_laplacian();

    edgelist g_edgelist = neg_laplacian.neg_laplacian_to_g();

    a_edgelist.save_edgelist_to_sqlite("debug_out.db", "a_edgelist");
    neg_laplacian.save_edgelist_to_sqlite("debug_out.db", "neg_laplacian");
    g_edgelist.save_edgelist_to_sqlite("debug_out.db", "g_edgelist");

    double S_avg = metrics::s_avg_gamma(neg_laplacian, g_edgelist, 0.05);
    std::cout << S_avg << "\n";
}

/**
 * Calculates & prints the L Neighborhood Reduction avg for a given Watts_Strogatz network with the given configuration.
 * @param L Neighborhood size L
 * @param SIZE The size (vertex count) of the generated network.
 * @param AVG_DEG The average number of edges (undirected network) connected to each vertex in the generated network. Must be even.
 * @param REWIRING_PROB The probability that each edge in the initial network is rewired. Represented as a double between 0 and 1 (inclusive, accurate to 1E-3).
 * @return The reduction average
*/
double watts_strogatz_small_world_network_L_neighborhood_reduction_avg_task(int L, int SIZE, int AVG_DEG, double REWIRING_PROB){
    edgelist a_edgelist;
    generate_watts_strogatz_small_world_network(a_edgelist,SIZE,AVG_DEG,REWIRING_PROB);

    edgelist neg_laplacian = a_edgelist.take_neg_laplacian();

    edgelist g_edgelist = neg_laplacian.neg_laplacian_to_g();

    a_edgelist.save_edgelist_to_sqlite("debug_out.db", "a_edgelist");
    neg_laplacian.save_edgelist_to_sqlite("debug_out.db", "neg_laplacian");
    g_edgelist.save_edgelist_to_sqlite("debug_out.db", "g_edgelist");

    double L_neighborhood_reduction_avg = metrics::L_neighborhood_reduction_rate_average(neg_laplacian, g_edgelist, L);
    //std::cout << L_neighborhood_reduction_avg << "\n";

    return L_neighborhood_reduction_avg;
}

int main(int argc, char* argv[]){
    std::setprecision(10);

    
    const std::string OUTPUT_DB = "output-8.db";
    watts_strogatz_generation_task(OUTPUT_DB);
    

    watts_strogatz_small_world_network_gamma_neighborhood_task(6.5E4,6,0.2);

    /*
    for (int L = 1; L <=100; L++){
        double avg = 0;
        for (int i = 0; i < 20; i++){
            avg += watts_strogatz_small_world_network_L_neighborhood_reduction_avg_task(L,3E4,6,0.2);
        }
        avg /= 20;
        std::cout << "L=" << L << "; avg L_neighborhood_reduction =" << avg <<"\n";
    }
    */

    /*
    // Process network file
    if (argc < 4){
        exit(1);
    }
    const std::string ID = argv[1];
    const std::string FILEPATH = argv[2];
    const bool WEIGHTED = bool(atoi(argv[3]));
    const bool DIRECTED = bool(atoi(argv[4]));
    process_network(ID, FILEPATH, WEIGHTED, DIRECTED);
    */

    return 0;
}