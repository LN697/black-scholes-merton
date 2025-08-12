#include "analytic_bs.hpp"
#include "math_utils.hpp"
#include <algorithm>
#include <cmath>

namespace bsm {

double black_scholes_price(double S0, double K, double r, double T, double sigma, OptionType type) {
    if (T <= 0.0) {
        double intrinsic = (type == OptionType::Call) ? std::max(S0 - K, 0.0) : std::max(K - S0, 0.0);
        return intrinsic;
    }
    
    if (sigma <= 0.0) {
        if (type == OptionType::Call) {
            return std::max(S0 - K * std::exp(-r * T), 0.0);
        } else {
            return std::max(K * std::exp(-r * T) - S0, 0.0);
        }
    }
    
    const double sqrt_T = std::sqrt(T);
    const double d1 = (std::log(S0 / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt_T);
    const double d2 = d1 - sigma * sqrt_T;
    
    if (type == OptionType::Call) {
        return S0 * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
    } else {
        return K * std::exp(-r * T) * norm_cdf(-d2) - S0 * norm_cdf(-d1);
    }
}

double black_scholes_delta(double S0, double K, double r, double T, double sigma, OptionType type) {
    if (sigma <= 0.0 || T <= 0.0) {
        if (type == OptionType::Call) {
            return (S0 > K) ? 1.0 : 0.0;
        } else {
            return (S0 < K) ? -1.0 : 0.0;
        }
    }
    
    const double sqrt_T = std::sqrt(T);
    const double d1 = (std::log(S0 / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt_T);
    
    if (type == OptionType::Call) {
        return norm_cdf(d1);
    } else {
        return norm_cdf(d1) - 1.0;
    }
}

double black_scholes_gamma(double S0, double K, double r, double T, double sigma) {
    if (sigma <= 0.0 || T <= 0.0 || S0 <= 0.0) {
        return 0.0;  // Degenerate cases have zero gamma
    }
    
    const double sqrt_T = std::sqrt(T);
    const double d1 = (std::log(S0 / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt_T);
    
    // Standard normal PDF: φ(x) = (1/√(2π)) * exp(-x²/2)
    const double phi_d1 = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI);
    
    return phi_d1 / (S0 * sigma * sqrt_T);
}

double black_scholes_vega(double S0, double K, double r, double T, double sigma) {
    if (sigma <= 0.0 || T <= 0.0 || S0 <= 0.0) {
        return 0.0;  // Degenerate cases have zero vega
    }
    
    const double sqrt_T = std::sqrt(T);
    const double d1 = (std::log(S0 / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt_T);
    
    // Standard normal PDF
    const double phi_d1 = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI);
    
    return S0 * phi_d1 * sqrt_T;
}

double black_scholes_theta(double S0, double K, double r, double T, double sigma, OptionType type) {
    if (T <= 0.0) {
        return 0.0;  // No time decay at expiration
    }
    
    if (sigma <= 0.0) {
        // For zero volatility, theta is related to interest on intrinsic value
        double intrinsic = (type == OptionType::Call) ? std::max(S0 - K, 0.0) : std::max(K - S0, 0.0);
        return -r * std::exp(-r * T) * intrinsic;
    }
    
    const double sqrt_T = std::sqrt(T);
    const double d1 = (std::log(S0 / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt_T);
    const double d2 = d1 - sigma * sqrt_T;
    
    // Standard normal PDF
    const double phi_d1 = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI);
    
    if (type == OptionType::Call) {
        return (-S0 * phi_d1 * sigma) / (2.0 * sqrt_T) - r * K * std::exp(-r * T) * norm_cdf(d2);
    } else {
        return (-S0 * phi_d1 * sigma) / (2.0 * sqrt_T) + r * K * std::exp(-r * T) * norm_cdf(-d2);
    }
}

double black_scholes_rho(double S0, double K, double r, double T, double sigma, OptionType type) {
    if (T <= 0.0) {
        return 0.0;  // No interest rate sensitivity at expiration
    }
    
    if (sigma <= 0.0) {
        // For zero volatility, rho is related to discounting of intrinsic value
        double intrinsic = (type == OptionType::Call) ? std::max(S0 - K, 0.0) : std::max(K - S0, 0.0);
        return T * std::exp(-r * T) * intrinsic;
    }
    
    const double sqrt_T = std::sqrt(T);
    const double d2 = (std::log(S0 / K) + (r - 0.5 * sigma * sigma) * T) / (sigma * sqrt_T);
    
    if (type == OptionType::Call) {
        return K * T * std::exp(-r * T) * norm_cdf(d2);
    } else {
        return -K * T * std::exp(-r * T) * norm_cdf(-d2);
    }
}

} // namespace bsm
