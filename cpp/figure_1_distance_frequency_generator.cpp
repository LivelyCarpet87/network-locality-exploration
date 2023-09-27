#include "edgelist.h"
#include "utils.h"
#include "network_metrics.h"

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
*/
int main() {
    edgelist a_edgelist;
    const std::string OUTPUT_DB = "cross_distances-output.db"; // File path of the database to be stored into.
    generate_watts_strogatz_small_world_network(a_edgelist,1000,20,0.1);
    a_edgelist.save_edgelist_to_sqlite(OUTPUT_DB,"a_edgelist");

    edgelist neg_laplacian = a_edgelist.take_neg_laplacian();
    neg_laplacian.save_edgelist_to_sqlite(OUTPUT_DB,"neg_laplacian_edgelist");

    edgelist g_edgelist = neg_laplacian.neg_laplacian_to_g();
    g_edgelist.save_edgelist_to_sqlite(OUTPUT_DB,"g_edgelist");

    std::cout << "a_edgelist geodesic_tau\n";
    metrics::distance_btwn_vertices a_dbv_tau = metrics::cross_geodesic_distance_tau(a_edgelist, 1000);
    //metrics::print_distance_to_vertices(a_dbv_tau);
    //save_distance_btwn_vertices_to_file("a_dbv_tau.csv", a_dbv_tau);
    metrics::dbv_to_sqlite(OUTPUT_DB, "a_dbv_tau", a_dbv_tau);
    
    /*
    std::cout << "a_edgelist geodesic_k\n";
    metrics::distance_btwn_vertices a_dbv_k = metrics::cross_geodesic_distance_k(a_edgelist, 10);
    //metrics::print_distance_to_vertices(a_dbv_k);
    //save_distance_btwn_vertices_to_file("a_dbv_k.csv", a_dbv_k);
    metrics::dbv_to_sqlite(OUTPUT_DB, "a_dbv_k", a_dbv_k);
    */

    /*
    std::cout << "neg_laplacian geodesic_tau\n";
    metrics::distance_btwn_vertices nl_dbv_tau = metrics::cross_geodesic_distance_tau(neg_laplacian, 1000);
    //metrics::print_distance_to_vertices(nl_dbv_tau);
    metrics::dbv_to_sqlite(OUTPUT_DB, "nl_dbv_tau", nl_dbv_tau);
    */

    std::cout << "g_edgelist geodesic_tau\n";
    metrics::distance_btwn_vertices g_dbv_tau = metrics::cross_geodesic_distance_tau(g_edgelist, 10000);
    //metrics::print_distance_to_vertices(g_dbv_tau);
    metrics::dbv_to_sqlite(OUTPUT_DB, "g_dbv_tau", g_dbv_tau);

    /*
    std::cout << "g_edgelist geodesic_k\n";
    metrics::distance_btwn_vertices g_dbv_k = metrics::cross_geodesic_distance_k(g_edgelist, 10);
    //metrics::print_distance_to_vertices(g_dbv_k);
    //save_distance_btwn_vertices_to_file("g_dbv_k.csv", g_dbv_k);
    metrics::dbv_to_sqlite(OUTPUT_DB, "g_dbv_k", g_dbv_k);
    */

    return 0;
}