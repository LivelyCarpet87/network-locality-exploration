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

void manual_test_network(edgelist &edgelist){
    /* Manual test network, view on https://graphonline.ru/en/
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
    edgelist.update_edge(1,2,1.2);
    edgelist.update_edge(2,3,0.7);
    edgelist.update_edge(3,4,0.9);
    edgelist.update_edge(4,5,0.1);
    edgelist.update_edge(5,6,1.6);
    edgelist.update_edge(6,7,1.3);
    edgelist.update_edge(7,1,0.85);
    edgelist.update_edge(1,5,0.7);
    edgelist.update_edge(2,6,0.3);
    edgelist.update_edge(3,7,0.8);
    edgelist.update_edge(4,1,0.8);
    edgelist.update_edge(5,8,1.6);

    edgelist.set_directional(false);
}

void generate_watts_strogatz_small_world_network(edgelist &edgelist, int SIZE, int AVG_DEG, double REWIRING_PROB){
    std::vector<edge> edges;
    edgelist.set_directional(false);

    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib_weight(0, 10E4);
    std::uniform_int_distribution<> distrib_reroll(0, 1000);
    std::uniform_int_distribution<> distrib_reroll_target(0, SIZE);

        
    for (int src = 0; src < SIZE; src ++){
        for (int offset = 1; offset <= AVG_DEG/2; offset++){
            int dest = (src + offset) % SIZE;
            double weight = distrib_weight(gen)/10E4;
            
            edgelist.update_edge(src,dest,weight);
        }
    }
    for (int src = 0; src < SIZE; src ++){
        for (int offset = 1; offset <= AVG_DEG/2; offset++){
            int dest_org = (src + offset) % SIZE;
            double weight = distrib_weight(gen)/10E4;
            if (distrib_reroll(gen)/(double)1000 <= REWIRING_PROB){ // Reroll chance
                int new_dest = distrib_reroll_target(gen);
                std::vector<int> current_adjacent = edgelist.get_adjacent_vertices(src);
                while(std::find(current_adjacent.begin(), current_adjacent.end(), new_dest) != current_adjacent.end() || new_dest == src){
                    new_dest = distrib_reroll_target(gen);
                }
                edgelist.update_edge(src,new_dest,weight);
                edgelist.rm_edge(src,dest_org);
            }
        }
    }
}

int watts_strogatz_generation_task(std::string OUTPUT_DB) {
    edgelist a_edgelist;
    generate_watts_strogatz_small_world_network(a_edgelist,1000,20,0.1);
    a_edgelist.save_edgelist_to_sqlite(OUTPUT_DB,"a_edgelist");

    edgelist neg_laplacian = a_edgelist.take_neg_laplacian();
    neg_laplacian.save_edgelist_to_sqlite(OUTPUT_DB,"neg_laplacian_edgelist");

    edgelist g_edgelist = neg_laplacian.neg_laplacian_to_g();
    g_edgelist.save_edgelist_to_sqlite(OUTPUT_DB,"g_edgelist");

    std::cout << "a_edgelist geodesic_tau\n";
    metrics::distance_btwn_vertices a_dbv_tau = metrics::cross_geodesic_distance_tau(a_edgelist, 100);
    //metrics::print_distance_to_vertices(a_dbv_tau);
    metrics::dbv_to_sqlite(OUTPUT_DB, "a_dbv_tau", a_dbv_tau);
    std::cout << "a_edgelist geodesic_k\n";
    metrics::distance_btwn_vertices a_dbv_k = metrics::cross_geodesic_distance_k(a_edgelist, 10);
    //metrics::print_distance_to_vertices(a_dbv_k);
    //save_distance_btwn_vertices_to_file("a_dbv_k.csv", a_dbv_k);
    metrics::dbv_to_sqlite(OUTPUT_DB, "a_dbv_k", a_dbv_k);

    std::cout << "g_edgelist geodesic_tau\n";
    metrics::distance_btwn_vertices g_dbv_tau = metrics::cross_geodesic_distance_tau(g_edgelist, 100);
    //metrics::print_distance_to_vertices(g_dbv_tau);
    metrics::dbv_to_sqlite(OUTPUT_DB, "g_dbv_tau", g_dbv_tau);
    std::cout << "g_edgelist geodesic_k\n";
    metrics::distance_btwn_vertices g_dbv_k = metrics::cross_geodesic_distance_k(g_edgelist, 10);
    //metrics::print_distance_to_vertices(g_dbv_k);
    //save_distance_btwn_vertices_to_file("g_dbv_k.csv", g_dbv_k);
    metrics::dbv_to_sqlite(OUTPUT_DB, "g_dbv_k", g_dbv_k);
    return 0;
}

int process_network(std::string ID, std::string FILEPATH, bool WEIGHTED, bool DIRECTED){
    // Initialize database
    sqlite3 *db;
    if (sqlite3_open("result.db", &db)) {
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
    return 0;
}

int watts_strogatz_small_world_network_gamma_neighborhood_task(){
    edgelist a_edgelist;
    generate_watts_strogatz_small_world_network(a_edgelist,5E4,6,0.2);

    edgelist neg_laplacian = a_edgelist.take_neg_laplacian();

    edgelist g_edgelist = neg_laplacian.neg_laplacian_to_g();

    a_edgelist.save_edgelist_to_sqlite("debug_out.db", "a_edgelist");
    neg_laplacian.save_edgelist_to_sqlite("debug_out.db", "neg_laplacian");
    g_edgelist.save_edgelist_to_sqlite("debug_out.db", "g_edgelist");

    double S_avg = metrics::s_avg_gamma(neg_laplacian, g_edgelist, 0.05);
    std::cout << S_avg << "\n";

    return 0;
}

/*
1E3 9.1903604999999988
5E3 8.7096329999999984
1E4 8.6390954999999998
5E4 8.2394960000000012 [Partial, 10]
1E5 2.1408385000000001
*/

double watts_strogatz_small_world_network_L_neighborhood_reduction_avg_task(int L){
    edgelist a_edgelist;
    generate_watts_strogatz_small_world_network(a_edgelist,1E3,6,0.2);

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
    //const bool WEIGHTED = false;
    //edgelist a_edgelist = edgelist_from_file(WEIGHTED, "./datasets/Technology/out.linux");
    //a_edgelist.set_directional(false);

    //const std::string OUTPUT_DB = "output-7.db";
    //watts_strogatz_generation_task(OUTPUT_DB);

    //watts_strogatz_small_world_network_gamma_neighborhood_task();

    /*
    for (int L = 1; L <=100; L++){
        double avg = 0;
        for (int i = 0; i < 20; i++){
            avg += watts_strogatz_small_world_network_L_neighborhood_reduction_avg_task(L);
        }
        avg /= 20;
        std::cout << "L=" << L << "; avg L_neighborhood_reduction =" << avg <<"\n";
    }
    */

    if (argc < 4){
        exit(1);
    }
    const std::string ID = argv[1];
    const std::string FILEPATH = argv[2];
    const bool WEIGHTED = bool(atoi(argv[3]));
    const bool DIRECTED = bool(atoi(argv[4]));
    process_network(ID, FILEPATH, WEIGHTED, DIRECTED);
    
    return 0;
}