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
 * @param GAMMA Value of gamma when calculating the neighborhood
*/
void process_network_s_avg(std::string ID, std::string FILEPATH, bool WEIGHTED, bool DIRECTED, double GAMMA){
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
    double S_avg = metrics::s_avg_gamma(neg_laplacian, g_edgelist, GAMMA);
    std::string sql_insert_query = "INSERT INTO S_average (NET_ID,GAMMA,avg_s) " \
    "VALUES ('" + ID + "', "+std::to_string(GAMMA)+", " + std::to_string(S_avg) + ")\n";

    if (sqlite3_exec(db, sql_insert_query.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK){
        fprintf(stderr, "Insertion SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

/**
 * Loads a network from a given file and calculates the L_neighborhood_reduction_avg to be saved in the results SQLite3 db under a task id.
 * Calculates for 0 < L <= 100.
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
    
    const std::string OUTPUT_DB = "output.db";
    int parsed_args = 0;

    // Read Edgelist Source
    std::string dataset_source;
    if (argc < parsed_args + 1) {
        std::cerr << "Insufficient Arguments\n";
        exit(1);
    } else {
        dataset_source = argv[1];
        parsed_args ++;
    }

    // Determine Edgelist Source
    edgelist a_edgelist;
    if (dataset_source.compare("gen_watts_strogatz") == 0){
        if (argc < parsed_args + 3) {
            std::cerr << "Insufficient Arguments To Generate Watts Strogatz Network.\n";
            exit(1);
        }
        std::cout << "Attempting to generate a Watts Strogatz network.\n";
        int SIZE;
        int AVG_DEG;
        double REWIRING_PROB;
        SIZE = atoi(argv[parsed_args + 1]);
        AVG_DEG = atoi(argv[parsed_args + 2]);
        try {
            REWIRING_PROB = std::stod(argv[parsed_args + 3]);
        } catch (...){
            std::cerr << "ERROR: Invalid REWIRING_PROB value.\n";
            exit(1);
        }
        if ( !(SIZE > 0) ) {
            std::cerr << "ERROR: Invalid network size value.\n";
            exit(1);
        } else if ( !(AVG_DEG > 0 && AVG_DEG % 2 == 0) ) {
            std::cerr << "ERROR: Invalid AVG_DEG value.\n";
            exit(1);
        } else if ( !(REWIRING_PROB>=0 && REWIRING_PROB <= 1) ){
            std::cerr << "ERROR: Invalid REWIRING_PROB value. Please pick a number between 0 and 1 (inclusive).\n";
            exit(1);
        }
        parsed_args += 3;
        generate_watts_strogatz_small_world_network(a_edgelist, SIZE, AVG_DEG, REWIRING_PROB);

    } else if (dataset_source.compare("load_file") == 0){
        if (argc < parsed_args + 3) {
            std::cerr << "Insufficient Arguments To Load Edgelist File.\n";
            exit(1);
        }
        std::cout << "Attempting to load an edgelist file.\n";
        const std::string filepath = argv[parsed_args + 1];
        const bool weighted = bool(atoi(argv[parsed_args + 2]));
        const bool directed = bool(atoi(argv[parsed_args + 3]));
        parsed_args += 3;

        a_edgelist = edgelist_from_file(weighted, filepath);
        a_edgelist.set_directional(directed);
    } else {
        std::cerr << "Invalid Edgelist Source Option\n";
        exit(1);
    }

    // Calculate neg_laplacian and g_tilda
    //edgelist nl_edgelist = a_edgelist.take_neg_laplacian();
    //edgelist g_edgelist = nl_edgelist.neg_laplacian_to_g();
    //std::cout << g_edgelist.to_string();
    
    // Read Action
    std::string action;
    if (argc < parsed_args + 1) {
        std::cerr << "Insufficient Arguments To Define Action.\n";
        exit(1);
    } else {
        action = argv[parsed_args+1];
        parsed_args ++;
    }

    // Determine action
    if (action.compare("convert_g_tilda") == 0){
        if (argc < parsed_args + 1) {
            std::cerr << "ERROR: Insufficient Arguments To Write To Edgelist File.\n";
            exit(1);
        }
        const std::string filepath = argv[parsed_args + 1];
        parsed_args += 1;

        edgelist nl_edgelist = a_edgelist.take_neg_laplacian();
        edgelist g_edgelist = nl_edgelist.neg_laplacian_to_g();

        std::cout << "Attempting to write edgelist to file.\n";
        g_edgelist.save_edgelist_as_plaintext(filepath);

    } else if (action.compare("dtv_k") == 0){
        int src;
        int k;

        if (argc < parsed_args + 2) {
            std::cerr << "ERROR: Insufficient Arguments To Define Action.\n";
            exit(1);
        }

        src = atoi(argv[parsed_args + 1]);
        k = atoi(argv[parsed_args + 2]);
        parsed_args += 2;

        if ( !( (src > 0 || (src == 0 && argv[parsed_args + 1][0] == '0' ) ) && src <= a_edgelist.max_vertex() ) ){
            std::cerr << "ERROR: Invalid starting SRC vertex ID for Distance To Vertex operation.\n";
            exit(1);
        }
        if ( !( k > 0 ) ){
            std::cerr << "ERROR: Invalid limit k for Distance To Vertex operation.\n";
            exit(1);
        }

        metrics::distance_to_vertices dtv = metrics::geodesic_distance_k(a_edgelist, src, k);
        metrics::print_distance_to_vertices(dtv);

    } else if (action.compare("dtv_tau") == 0){
        int src;
        double tau;

        if (argc < parsed_args + 2) {
            std::cerr << "ERROR: Insufficient Arguments To Define Action.\n";
            exit(1);
        }

        src = atoi(argv[parsed_args + 1]);
        try {
            tau = std::stod(argv[parsed_args + 2]);
        } catch (...){
            std::cerr << "ERROR: Invalid limit tau for Distance To Vertex operation.\n";
            exit(1);
        }

        parsed_args += 2;

        if ( !( (src > 0 || (src == 0 && argv[parsed_args + 1][0] == '0' ) ) && src <= a_edgelist.max_vertex() ) ){
            std::cerr << "ERROR: Invalid starting SRC vertex ID for Distance To Vertex operation.\n";
            exit(1);
        }
        if ( !( tau > 0 ) ){
            std::cerr << "ERROR: Invalid limit tau for Distance To Vertex operation.\n";
            exit(1);
        }

        metrics::distance_to_vertices dtv = metrics::geodesic_distance_tau(a_edgelist, src, tau);
        metrics::print_distance_to_vertices(dtv);

    } else if (action.compare("dbv_k") == 0){
        int k;

        if (argc < parsed_args + 1) {
            std::cerr << "Insufficient Arguments To Define Action.\n";
            exit(1);
        }

        k = atoi(argv[parsed_args + 1]);
        parsed_args += 1;
        if ( !( k > 0 ) ){
            std::cerr << "Invalid limit k for Distance To Vertex operation.\n";
            exit(1);
        }

        metrics::distance_btwn_vertices dbv = metrics::cross_geodesic_distance_k(a_edgelist, k);
        metrics::print_distance_to_vertices(dbv);

    } else if (action.compare("dbv_tau") == 0){
        double tau;

        if (argc < parsed_args + 1) {
            std::cerr << "ERROR: Insufficient Arguments To Define Action.\n";
            exit(1);
        }

        try {
            tau = std::stod(argv[parsed_args + 1]);
            parsed_args += 1;
        } catch (...){
            std::cerr << "ERROR: Invalid limit tau for Distance To Vertex operation.\n";
            exit(1);
        }

        if ( !( tau > 0 ) ){
            std::cerr << "ERROR: Invalid limit tau for Distance To Vertex operation.\n";
            exit(1);
        }

        metrics::distance_btwn_vertices dbv = metrics::cross_geodesic_distance_tau(a_edgelist, tau);
        metrics::print_distance_to_vertices(dbv);

    } else if (action.compare("s_avg") == 0){
        double GAMMA;

        if (argc < parsed_args + 1) {
            std::cerr << "ERROR: Insufficient Arguments To Define Action.\n";
            exit(1);
        }

        try {
            GAMMA = std::stod(argv[parsed_args + 1]);
            parsed_args += 1;
        } catch (...){
            std::cerr << "ERROR: Invalid Gamma.\n";
            exit(1);
        }

        if ( !( GAMMA > 0 && GAMMA < 1) ){
            std::cerr << "ERROR: Invalid Gamma value given.\n";
            exit(1);
        }

        edgelist nl_edgelist = a_edgelist.take_neg_laplacian();
        edgelist g_edgelist = nl_edgelist.neg_laplacian_to_g();

        double S_avg = metrics::s_avg_gamma(nl_edgelist, g_edgelist, GAMMA);

        std::cout << "S_avg= " << S_avg << "\n";
    } else {
        std::cerr << "Invalid Action Option\n";
        exit(1);
    }
    

    return 0;
}