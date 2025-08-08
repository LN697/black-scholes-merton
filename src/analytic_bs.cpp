#include "analytic_bs.hpp"
#include "math_utils.hpp"
#include <algorithm>
#include <cmath>

namespace bsm {

double black_scholes_price(double S0, double K, double r, double T, double sigma, OptionType type) {
    if (sigma <= 0.0 || T <= 0.0) {
        double intrinsic = (type == OptionType::Call) ? std::max(S0 - K, 0.0) : std::max(K - S0, 0.0);
        return std::exp(-r * T) * intrinsic;
    }
    double d1 = (std::log(S0 / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    double d2 = d1 - sigma * std::sqrt(T);
    if (type == OptionType::Call) {
        return S0 * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
    } else {
        return K * std::exp(-r * T) * norm_cdf(-d2) - S0 * norm_cdf(-d1);
    }
}

}
