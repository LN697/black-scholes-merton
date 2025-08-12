#pragma once

/**
 * @file stats.hpp
 * @brief Statistical structures and utility functions for Monte Carlo results
 * 
 * This module provides data structures and functions for handling statistical
 * results from Monte Carlo simulations, including price estimates, standard
 * errors, and Greeks calculations.
 * 
 * @author LN697
 * @version 1.0
 */

#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>

namespace bsm {

/**
 * @brief Result structure for Monte Carlo pricing simulations
 * 
 * Contains the price estimate, statistical measures of uncertainty,
 * and Greeks (sensitivities) when computed.
 */
struct MCResult {
    double price{0.0};
    double std_error{0.0}; 
    double delta{0.0};
    double delta_se{0.0};
    double vega{0.0};
    double vega_se{0.0};
    double gamma{0.0};
    double gamma_se{0.0};
    double theta{0.0};
    double theta_se{0.0};
    long num_paths{0};
    long num_steps{0};
    unsigned long seed{0};
    
    /**
     * @brief Get confidence interval for price estimate
     * @param confidence_level Confidence level (e.g., 0.95 for 95%)
     * @return Pair of (lower_bound, upper_bound)
     */
    std::pair<double, double> confidence_interval(double confidence_level = 0.95) const {
        // For large samples, use normal approximation
        double z_score = 1.96; // 95% confidence
        if (confidence_level == 0.99) z_score = 2.576;
        else if (confidence_level == 0.90) z_score = 1.645;
        
        double margin = z_score * std_error;
        return {price - margin, price + margin};
    }
    
    /**
     * @brief Check if price estimate is statistically significant vs zero
     * @param significance_level Significance level (e.g., 0.05 for 5%)
     * @return True if price is significantly different from zero
     */
    bool is_significant(double significance_level = 0.05) const {
        if (std_error <= 0.0) return false;
        double t_stat = std::abs(price / std_error);
        double critical_value = (significance_level == 0.05) ? 1.96 : 
                               (significance_level == 0.01) ? 2.576 : 1.645;
        return t_stat > critical_value;
    }
};

inline double mean(const std::vector<double>& x) {
    if (x.empty()) return 0.0;
    const double sum = std::accumulate(x.begin(), x.end(), 0.0);
    return sum / static_cast<double>(x.size());
}

/**
 * @brief Compute sample variance with Bessel's correction
 * @param x Input vector
 * @return Sample variance (unbiased estimator)
 * @par Complexity: O(n)
 * @par Thread Safety: Yes (read-only)
 */
inline double variance(const std::vector<double>& x) {
    if (x.size() < 2) return 0.0;
    const double m = mean(x);
    double sum_sq_diff = 0.0;
    for (const double v : x) {
        const double diff = v - m;
        sum_sq_diff += diff * diff;
    }
    return sum_sq_diff / static_cast<double>(x.size() - 1);
}

/**
 * @brief Compute sample standard deviation
 * @param x Input vector
 * @return Sample standard deviation
 * @par Complexity: O(n)
 */
inline double standard_deviation(const std::vector<double>& x) {
    return std::sqrt(variance(x));
}

/**
 * @brief Compute standard error of the mean
 * @param x Input vector
 * @return Standard error of sample mean
 * @par Formula: SE = σ/√n where σ is sample standard deviation
 */
inline double standard_error(const std::vector<double>& x) {
    if (x.size() < 2) return 0.0;
    return standard_deviation(x) / std::sqrt(static_cast<double>(x.size()));
}

/**
 * @brief Compute sample covariance between two vectors
 * @param x First vector
 * @param y Second vector
 * @return Sample covariance
 * @note Vectors must have the same size
 */
inline double covariance(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 2) return 0.0;
    
    const double mean_x = mean(x);
    const double mean_y = mean(y);
    double sum_cov = 0.0;
    
    for (size_t i = 0; i < x.size(); ++i) {
        sum_cov += (x[i] - mean_x) * (y[i] - mean_y);
    }
    
    return sum_cov / static_cast<double>(x.size() - 1);
}

/**
 * @brief Compute Pearson correlation coefficient
 * @param x First vector
 * @param y Second vector
 * @return Correlation coefficient [-1, 1]
 */
inline double correlation(const std::vector<double>& x, const std::vector<double>& y) {
    const double cov = covariance(x, y);
    const double std_x = standard_deviation(x);
    const double std_y = standard_deviation(y);
    
    if (std_x <= 0.0 || std_y <= 0.0) return 0.0;
    return cov / (std_x * std_y);
}

/**
 * @brief Compute percentile of a vector
 * @param x Input vector
 * @param percentile Percentile to compute (0-100)
 * @return Value at the specified percentile
 * @note This function modifies the input vector (sorts it)
 */
inline double percentile(std::vector<double>& x, double percentile) {
    if (x.empty()) return 0.0;
    if (percentile <= 0.0) return *std::min_element(x.begin(), x.end());
    if (percentile >= 100.0) return *std::max_element(x.begin(), x.end());
    
    std::sort(x.begin(), x.end());
    const double rank = (percentile / 100.0) * (x.size() - 1);
    const size_t lower_idx = static_cast<size_t>(std::floor(rank));
    const size_t upper_idx = static_cast<size_t>(std::ceil(rank));
    
    if (lower_idx == upper_idx) {
        return x[lower_idx];
    } else {
        const double weight = rank - lower_idx;
        return x[lower_idx] * (1.0 - weight) + x[upper_idx] * weight;
    }
}

/**
 * @brief Compute Value at Risk (VaR) for a P&L distribution
 * @param pnl_vector Vector of profit/loss values
 * @param confidence_level Confidence level (e.g., 0.95 for 95% VaR)
 * @return VaR estimate (positive value represents potential loss)
 */
inline double value_at_risk(std::vector<double> pnl_vector, double confidence_level = 0.95) {
    if (pnl_vector.empty()) return 0.0;
    const double percentile_level = (1.0 - confidence_level) * 100.0;
    const double var_value = percentile(pnl_vector, percentile_level);
    return -var_value; // Convention: VaR is positive for losses
}

/**
 * @brief Compute Expected Shortfall (ES) / Conditional VaR
 * @param pnl_vector Vector of profit/loss values
 * @param confidence_level Confidence level (e.g., 0.95)
 * @return Expected Shortfall estimate
 */
inline double expected_shortfall(std::vector<double> pnl_vector, double confidence_level = 0.95) {
    if (pnl_vector.empty()) return 0.0;
    
    std::sort(pnl_vector.begin(), pnl_vector.end());
    const size_t cutoff_idx = static_cast<size_t>((1.0 - confidence_level) * pnl_vector.size());
    
    if (cutoff_idx == 0) return -pnl_vector[0];
    
    double sum = 0.0;
    for (size_t i = 0; i < cutoff_idx; ++i) {
        sum += pnl_vector[i];
    }
    
    return -sum / static_cast<double>(cutoff_idx);
}

/**
 * @brief Combine multiple MC results with proper error propagation
 * @param results Vector of MCResult objects
 * @return Combined MCResult with aggregated statistics
 */
inline MCResult combine_mc_results(const std::vector<MCResult>& results) {
    if (results.empty()) return MCResult{};
    
    MCResult combined;
    double total_weight = 0.0;
    double weighted_price_sum = 0.0;
    double variance_sum = 0.0;
    
    for (const auto& result : results) {
        if (result.num_paths <= 0) continue;
        
        const double weight = static_cast<double>(result.num_paths);
        total_weight += weight;
        weighted_price_sum += weight * result.price;
        variance_sum += weight * result.std_error * result.std_error;
    }
    
    if (total_weight > 0.0) {
        combined.price = weighted_price_sum / total_weight;
        combined.std_error = std::sqrt(variance_sum) / total_weight;
        combined.num_paths = static_cast<long>(total_weight);
    }
    
    return combined;
}

}
