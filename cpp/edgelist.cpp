#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sqlite3.h>
#include <omp.h>

#include "utils.h"
#include "funcs.h"

#include "edgelist.h"

// constructors
edgelist::edgelist(){
    this->Directional = false;
}


edgelist::edgelist(bool isDirectional){
    this->Directional = isDirectional;
}

void edgelist::update_edge(int src, int dest, double weight){

    // Test if the src vertex is in the first layer of Edges, insert it if it is not
    if (this->Edges.find(src) == this->Edges.end()){
        std::map<int, std::vector<double>> connections; // create empty datastructure to store edge connection data in future
        this->Edges.emplace(src, connections); // create vertex
    }
    // Test if dest vertex is in the second layer of edges
    if (this->Edges[src].find(dest) == this->Edges[src].end()){
        std::vector<double> weights;
        this->Edges[src].emplace(dest, weights);
    }

    // Test if the dest vertex is in the first layer of RevEdgePointers, insert it if it is not
    if (this->RevEdges.find(dest) == this->RevEdges.end()){
        std::map<int, std::vector<double>> revConnections; // create empty datastructure to store edge connection data in future
        this->RevEdges.emplace(dest, revConnections); // create vertex
    }
    // Test if the src vertex is in the second layer of Edges
    if (this->RevEdges[dest].find(src) == this->RevEdges[dest].end()){
        std::vector<double> weights;
        this->RevEdges[dest].emplace(src, weights);
    }

    this->Edges[src][dest].push_back(weight);
    this->RevEdges[dest][src].push_back(weight);
}

void edgelist::rm_edge(int src, int dest){
    if (this->Edges.find(src) != this->Edges.end()){
        this->Edges[src].erase(dest);
    }
    
    if (this->RevEdges.find(dest) != this->RevEdges.end()){
        this->RevEdges[dest].erase(src);
    }
}

void edgelist::set_directional(bool isDirectional){
    this->Directional = isDirectional;
}

std::vector<edge> edgelist::get_edges(){
    std::vector<edge> edges;
    for (auto iter_layer_one : this->Edges){
        int src = iter_layer_one.first;
        std::map<int, std::vector<double>> connections = iter_layer_one.second;
        for (auto iter_layer_two : connections){
            int dest = iter_layer_two.first;
            std::vector<double> edge_weights = iter_layer_two.second;
            for (double weight : edge_weights){
                struct edge new_edge = {
                    .src = src,
                    .dest = dest,
                    .weight = weight,
                };
                edges.push_back(new_edge);
            }
        }
    }
    
    return edges;
}

std::vector<edge> edgelist::get_edges_duplicate_on_undirectional(){
    std::vector<edge> edges;
    for (auto iter_layer_one : this->Edges){
        int src = iter_layer_one.first;
        std::map<int, std::vector<double>> connections = iter_layer_one.second;
        for (auto iter_layer_two : connections){
            int dest = iter_layer_two.first;
            std::vector<double> edge_weights = iter_layer_two.second;
            for (double weight : edge_weights){
                struct edge new_edge = {
                    .src = src,
                    .dest = dest,
                    .weight = weight,
                };
                edges.push_back(new_edge);
            }
        }
    }
    if (!this->Directional){
        for (auto iter_layer_one : this->RevEdges){
            int src = iter_layer_one.first;
            std::map<int, std::vector<double>> connections = iter_layer_one.second;
            for (auto iter_layer_two : connections){
                int dest = iter_layer_two.first;
                std::vector<double> edge_weights = iter_layer_two.second;
                for (double weight : edge_weights){
                    struct edge new_edge = {
                        .src = src,
                        .dest = dest,
                        .weight = weight,
                    };
                    edges.push_back(new_edge);
                }
            }
        }
    }
    return edges;
}

std::vector<edge> edgelist::get_edges(int src){
    std::vector<edge> edges;
    if (this->Edges.find(src) != this->Edges.end()){
        std::map<int, std::vector<double>> connections = this->Edges.at(src);
        for (auto iter_layer_two : connections){
            int dest = iter_layer_two.first;
            std::vector<double> edge_weights = iter_layer_two.second;
            for (double weight : edge_weights){
                struct edge new_edge = {
                    .src = src,
                    .dest = dest,
                    .weight = weight,
                };
                edges.push_back(new_edge);
            }
        }
    }
    if (!this->Directional && this->RevEdges.find(src) != this->RevEdges.end()){
        std::map<int, std::vector<double>> connections = this->RevEdges.at(src);
        for (auto iter_layer_two : connections){
            int dest = iter_layer_two.first;
            std::vector<double> edge_weights = iter_layer_two.second;
            for (double weight : edge_weights){
                struct edge new_edge = {
                    .src = src,
                    .dest = dest,
                    .weight = weight,
                };
                edges.push_back(new_edge);
            }
        }
    }
    return edges;
}

std::vector<int> edgelist::get_adjacent_vertices(int src){
    std::vector<int> vertices;
    if (this->Edges.find(src) != this->Edges.end()){
        std::map<int, std::vector<double>> connections = this->Edges.at(src);
        for (auto iter_layer_two : connections){
            int dest = iter_layer_two.first;
            vertices.push_back(dest);
        }
    }
    if (!this->Directional && this->RevEdges.find(src) != this->RevEdges.end()){
        std::map<int, std::vector<double>> connections = this->RevEdges.at(src);
        for (auto iter_layer_two : connections){
            int dest = iter_layer_two.first;
            vertices.push_back(dest);
        }
    }
    return vertices;
}

std::string edgelist::to_string(){
    std::string result_str = "";
    std::vector<edge> edges = get_edges();
    result_str += "Edgelist Edges:\n";
    for (struct edge edge : edges){
        result_str += std::to_string(edge.src) + " -> " + std::to_string(edge.dest) + " = ";
        char edge_weight [20];
        sprintf(edge_weight, "%.10E", edge.weight);
        result_str += std::string(edge_weight) + "\n";
    }
    return result_str;
}

std::vector<double>  edgelist::get_edge_weights(int src, int dest){
    std::vector<double> edge_weights;
    if (this->Edges.find(src) != this->Edges.end() && this->Edges.at(src).find(dest) != this->Edges.at(src).end()){
        std::vector<double> weights = this->Edges.at(src).at(dest);
        for (double weight : weights){
            edge_weights.push_back(weight);
        }
    }
    
    if (!this->Directional && this->RevEdges.find(src) != this->RevEdges.end() && this->RevEdges.at(src).find(dest) != this->RevEdges.at(src).end()){
        std::vector<double> weights = this->RevEdges.at(src).at(dest);
        for (double weight : weights){
            edge_weights.push_back(weight);
        }
    }

    return edge_weights;
}

int edgelist::max_vertex(){
    if (this->Edges.size() > 0){
        int largest_src = this->Edges.rbegin()->first;
        int largest_dest = this->RevEdges.rbegin()->first;
        if (largest_src > largest_dest){
            return largest_src;
        }
        return largest_dest;
    }
    return -1;
}

edgelist edgelist::take_neg_laplacian(){
    edgelist neg_laplacian = edgelist(true);
    int dim = max_vertex();
    #ifdef _DEBUG
    std::cout << "Calculating negative laplacian non-diagonal...\n";
    #endif

    omp_lock_t writelock;
    omp_init_lock(&writelock);

    #pragma omp parallel for collapse(2)
    for (int i = 0; i <= dim; i++){
        for (int j = 0; j <= dim; j++){
            if (i == j) {
                continue;
            }

            std::vector<double> edge_weights = get_edge_weights(i,j);
            
            if (!edge_weights.empty()){
                double max_weight = -INFINITY;
                for (double weight : edge_weights){
                    double abs_weight = std::abs(weight);
                    if (abs_weight > max_weight){
                        max_weight = abs_weight;
                    }
                }
                omp_set_lock(&writelock);
                neg_laplacian.update_edge(i, j, max_weight);
                omp_unset_lock(&writelock);
            }
        }
    }

    omp_destroy_lock(&writelock);

    #ifdef _DEBUG
    std::cout << "Calculating negative laplacian diagonal...\n";
    #endif
    // Iterate along diagonal
    double* diagonal_vals = new double[dim+1];
    #pragma omp parallel for
    for (int i = 0; i <= dim; i++){
        double total_weight = 0;
        for (int j = 0; j <=dim; j++){
            if (i == j){
                continue;
            }
            std::vector<double> edge_weights = get_edge_weights(i,j);
            if (!edge_weights.empty()){
                double max_weight = -INFINITY;
                for (double weight : edge_weights){
                    double abs_weight = std::abs(weight);
                    if (abs_weight > max_weight){
                        max_weight = abs_weight;
                    }
                }
                total_weight += max_weight;
            }
        }
        diagonal_vals[i] = -total_weight;
    }

    for (int i = 0; i <= dim; i++){
        neg_laplacian.update_edge(i, i, diagonal_vals[i]);
    }
    delete[] diagonal_vals;

    return neg_laplacian;
}

edgelist edgelist::neg_laplacian_to_g(){
    edgelist g_edgelist = edgelist(false);
    int dim = max_vertex();

    std::vector<struct edge> edges = get_edges();
    double max_absolute_weight = -INFINITY;
    for (struct edge edge : edges){
        double abs_weight = std::abs(edge.weight);
        if (abs_weight > max_absolute_weight){
            max_absolute_weight = abs_weight;
        }
    }

    #ifdef _DEBUG
    std::cout << "Calculating g...\n";
    #endif
    omp_lock_t writelock;
    omp_init_lock(&writelock);
    #pragma omp parallel for
    for (int i = 0; i <= dim; i++){
        for (int j = 0; j <= dim; j++){
            std::vector<double> edge_weights = get_edge_weights(i,j);
            std::vector<double> edge_weights_rev = get_edge_weights(j,i);
            edge_weights.insert(edge_weights.end(),edge_weights_rev.begin(),edge_weights_rev.end());
            if (!edge_weights.empty()){
                double max_abs_weight_for_edge = -INFINITY;
                for (double weight : edge_weights){
                    double abs_weight = std::abs(weight);
                    if (abs_weight > max_abs_weight_for_edge){
                        max_abs_weight_for_edge = abs_weight;
                    }
                }
                double y = max_absolute_weight / max_abs_weight_for_edge;
                omp_set_lock(&writelock);
                double x = funcs::w_func(y);
                double g = std::max(x,funcs::EPSILON);
                g_edgelist.update_edge(i, j, g);
                omp_unset_lock(&writelock);
            }
        }
    }
    omp_destroy_lock(&writelock);
    return g_edgelist;
}

void edgelist::save_edgelist_to_sqlite(std::string filepath, std::string table_name){
    sqlite3 *db;
    if (sqlite3_open(filepath.c_str(), &db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    std::string sql_create_table_query = "CREATE TABLE IF NOT EXISTs " \
     + table_name + "("  \
    "SRC                INT     NOT NULL," \
    "DST                INT     NOT NULL," \
    "WEIGHT             REAL    NOT NULL);";

    char *zErrMsg = 0;
    if( sqlite3_exec(db, sql_create_table_query.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK ){
        fprintf(stderr, "SQL Error When Creating Edgelist Table:\n%s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(1);
    }

    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
    std::vector<edge> edges = get_edges();
    for (struct edge edge : edges){
        char edge_weight [20];
        sprintf(edge_weight, "%.10E", edge.weight);

        std::string sql_insert_query = "INSERT INTO "+table_name+" (SRC,DST,WEIGHT) " \
            "VALUES (" + std::to_string(edge.src) + ", " + std::to_string(edge.dest) + ", " + edge_weight + ")\n";
        if (sqlite3_exec(db, sql_insert_query.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
    sqlite3_close(db);
}