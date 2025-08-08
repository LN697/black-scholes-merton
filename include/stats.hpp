#pragma once
#include <vector>
#include <cmath>
#include <numeric>

namespace bsm {

struct MCResult {
    double price{};
    double std_error{}; // standard error of the discounted payoff mean
    // Optional Greeks (if computed)
    double delta{};
    double delta_se{};
    double vega{};
    double vega_se{};
};

inline double mean(const std::vector<double>& x) {
    if (x.empty()) return 0.0;
    double s = std::accumulate(x.begin(), x.end(), 0.0);
    return s / static_cast<double>(x.size());
}

inline double variance(const std::vector<double>& x) {
    if (x.size() < 2) return 0.0;
    double m = mean(x);
    double acc = 0.0;
    for (double v : x) acc += (v - m) * (v - m);
    return acc / static_cast<double>(x.size() - 1);
}

inline double stdev(const std::vector<double>& x) { return std::sqrt(variance(x)); }

}
