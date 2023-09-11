#pragma once

#include <map>


namespace funcs {
    namespace {
        std::map<double, double> ValuePairs; // Stored as y,x
        double MAX_X;
    }


    const double EPSILON = 1E-12;

    double v_func(double x);

    void generate_v_func(double x);

    void save_v_func();

    void load_v_func();

    double w_func(double y);

    std::pair<double, double> max_approximation_threshold_w(double y);
};