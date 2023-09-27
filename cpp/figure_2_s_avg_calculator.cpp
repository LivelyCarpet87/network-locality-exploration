#include <sqlite3.h>

#include "edgelist.h"
#include "utils.h"
#include "network_metrics.h"

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
    if (sqlite3_open("s_avg_results.db", &db)) {
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

int main(int argc, char* argv[]){
    if (argc < 4){
        exit(1);
    }
    const std::string ID = argv[1];
    const std::string FILEPATH = argv[2];
    const bool WEIGHTED = bool(atoi(argv[3]));
    const bool DIRECTED = bool(atoi(argv[4]));
    process_network_s_avg(ID, FILEPATH, WEIGHTED, DIRECTED, 0.05);

    return 0;
}