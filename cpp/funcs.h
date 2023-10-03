#pragma once

#include <map>

namespace funcs {
    namespace { // This anonymous namespace is used to store the value pairs of the v function calculated
        std::map<double, double> ValuePairs; // Stored in the format of y,x where y=v(x)
        double MAX_X; // The maximum value of x calculated and stored
    }

    // The step size used to calculate the v_func
    const double STEP_SIZE = 0.0001;
    // The value of the other constants in the functions
    const double EPSILON = 1E-12;

    /*
    const double ALPHA = 1;
    const double BETA = 0.9;
    const double Q = 1.2;
    */
    const double ALPHA = 1;
    const double BETA = 0.9999;
    const double Q = 1.001;
    

    // The function v(x)
    double v_func(double x);

    /**
     * Calculate and store the values of v(x) up to the input target
     * @param x The target value of x to stop at
    */
    void generate_v_func(double x);

    // The returns an approximation of the value of the inverse function of v(x), where w(y) = x, using linear interpolation and the precomputed results of v(x)
    double w_func(double y);

    // Gives the maximum threshold y for approximation of the results of v(x) before v(x) needs to be calculated.
    // Returns the largest x,y pair where y < the value given. Possible only because v(x) is monotonous increasing function.
    std::pair<double, double> max_approximation_threshold_w(double y);
};