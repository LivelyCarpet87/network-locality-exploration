#include <cmath>
#include <algorithm>
#include <iostream>

#include "funcs.h"


double funcs::v_func(double x){
    return std::exp(funcs::ALPHA*std::pow(x,funcs::BETA)) * std::pow(1+x,funcs::Q);
}

void funcs::generate_v_func(double max_x = -1){
    if (max_x == -1) {
        max_x = 10000;
    }
    #ifdef _DEBUG
    std::cout << "Generating V function...\n";
    #endif
    for (double x = 0; x <= max_x; x += funcs::STEP_SIZE){
        double y = v_func(x);
        ValuePairs.emplace(y,x);
    }
    MAX_X = max_x;
    #ifdef _DEBUG
    std::cout << "Finished generating V function.\n";
    #endif
}

double funcs::w_func(double y){
    if (y < 0){
        std::cerr << "ERROR: w_func y lower bound exceeded. Got negative value.\n";
        exit(1);
    }
    if (ValuePairs.size() == 0){
        generate_v_func();
    }
    auto max_y_iter = ValuePairs.lower_bound(y);

    while (max_y_iter == ValuePairs.end()){
        #ifdef _DEBUG
        std::cout << "WARNING: w_func upper bound exceeded. Computing more.\n";
        #endif
        generate_v_func(MAX_X+1000*funcs::STEP_SIZE);
        max_y_iter = ValuePairs.lower_bound(y);
    }

    // Find the first y,x pair where y >= the y given
    double upper_bound_y = max_y_iter->first;
    double upper_bound_x = max_y_iter->second;
    
    if (upper_bound_y == y){ // If there is a match for the y given, the value is used
        return upper_bound_x;
    }

    // Otherwise get the y,x pair where y < the y given
    max_y_iter--;
    double lower_bound_y = max_y_iter->first;
    double lower_bound_x = max_y_iter->second;
    // Return the linear interpolated value
    return lower_bound_x + (upper_bound_x-lower_bound_x)* (y-lower_bound_y)/(upper_bound_y-lower_bound_y);
}

std::pair<double, double> funcs::max_approximation_threshold_w(double y){
    if (ValuePairs.size() == 0){
        generate_v_func();
    }
    auto max_y_iter = ValuePairs.lower_bound(y);

    while (max_y_iter == ValuePairs.end()){
        #ifdef _DEBUG
        std::cout << "WARNING: w_func upper bound exceeded. Computing more.\n";
        #endif
        generate_v_func(MAX_X+1000*funcs::STEP_SIZE);
        max_y_iter = ValuePairs.lower_bound(y);
    }

    double upper_bound_y = max_y_iter->first;
    double upper_bound_x = max_y_iter->second;
    if (upper_bound_y == y){
        return std::pair(upper_bound_x,upper_bound_y);
    }
    max_y_iter--;
    double lower_bound_y = max_y_iter->first;
    double lower_bound_x = max_y_iter->second;

    return std::pair(lower_bound_x,lower_bound_y);
} 