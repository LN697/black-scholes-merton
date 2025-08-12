#pragma once
#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>
#include "dupire.hpp"
#include "slv.hpp"
#include "option_types.hpp"

namespace bsm {

struct LeverageGrid {
    std::vector<double> t;
    std::vector<double> S;
    std::vector<std::vector<double>> L;
    
    double interpolate(double St, double tt) const {
        if (t.empty() || S.empty()) return 1.0;
        if (tt <= t.front()) {
            if (St <= S.front()) return L.front().front();
            if (St >= S.back()) return L.front().back();
            auto itS = std::upper_bound(S.begin(), S.end(), St) - S.begin();
            size_t i1 = std::max<size_t>(1, itS) - 1, i2 = std::min(i1 + 1, S.size() - 1);
            double w = (St - S[i1]) / std::max(1e-12, S[i2] - S[i1]);
            return (1 - w) * L.front()[i1] + w * L.front()[i2];
        }
        if (tt >= t.back()) {
            if (St <= S.front()) return L.back().front();
            if (St >= S.back()) return L.back().back();
            auto itS = std::upper_bound(S.begin(), S.end(), St) - S.begin();
            size_t i1 = std::max<size_t>(1, itS) - 1, i2 = std::min(i1 + 1, S.size() - 1);
            double w = (St - S[i1]) / std::max(1e-12, S[i2] - S[i1]);
            return (1 - w) * L.back()[i1] + w * L.back()[i2];
        }
        auto itT = std::upper_bound(t.begin(), t.end(), tt) - t.begin();
        size_t j1 = std::max<size_t>(1, itT) - 1, j2 = std::min(j1 + 1, t.size() - 1);
        double wt = (tt - t[j1]) / std::max(1e-12, t[j2] - t[j1]);
        auto itS = std::upper_bound(S.begin(), S.end(), St) - S.begin();
        size_t i1 = std::max<size_t>(1, itS) - 1, i2 = std::min(i1 + 1, S.size() - 1);
        double ws = (St - S[i1]) / std::max(1e-12, S[i2] - S[i1]);
        double v11 = L[j1][i1], v12 = L[j1][i2], v21 = L[j2][i1], v22 = L[j2][i2];
        double v1 = (1 - ws) * v11 + ws * v12;
        double v2 = (1 - ws) * v21 + ws * v22;
        return (1 - wt) * v1 + wt * v2;
    }
};

// Calibration configuration
struct SLVCalibrationConfig {
    int max_iterations = 20;
    double tolerance = 1e-4;
    double damping_factor = 0.5;  // For stability
    long num_paths = 100000;      // For MC estimation
    int num_time_steps = 50;      // For MC paths
    double min_leverage = 0.1;    // Bounds for leverage
    double max_leverage = 3.0;
};

// Forward declarations
double estimate_model_implied_volatility_mc(
    double S, double t, double K, double r, double T,
    const HestonParams& heston, const LeverageGrid& leverage, 
    const SLVCalibrationConfig& config);

double estimate_model_implied_volatility_fd(
    double S, double t, const HestonParams& heston, 
    const LeverageGrid& leverage, double dt = 1e-4);

// Utility functions for testing and examples
DupireSurface create_sample_dupire_surface();
LeverageGrid create_sample_leverage_grid(const DupireSurface& dupire);
bool validate_slv_calibration();

// Main calibration function
inline void calibrate_leverage_iterative(const DupireSurface& target, const HestonParams& h,
                                         LeverageGrid& lev, int iterations = 5) {
    SLVCalibrationConfig config;
    config.max_iterations = iterations;
    
    if (lev.t.empty() || lev.S.empty()) {
        return;  // Invalid grid
    }
    
    // Initialize leverage to 1.0 if not set
    if (lev.L.empty()) {
        lev.L.resize(lev.t.size(), std::vector<double>(lev.S.size(), 1.0));
    }
    
    const double r = 0.05; // Risk-free rate for calibration
    (void)r; // Mark as used for potential future enhancement
    
    for (int iter = 0; iter < config.max_iterations; ++iter) {
        double max_error = 0.0;
        
        for (size_t j = 0; j < lev.t.size(); ++j) {
            double tt = lev.t[j];
            if (tt <= 1e-6) continue; // Skip very small times
            
            for (size_t i = 0; i < lev.S.size(); ++i) {
                double St = lev.S[i];
                double sig_target = target.at(j, i);
                
                if (sig_target <= 0.0) continue; // Skip invalid target volatilities
                
                // Estimate model-implied local volatility using finite differences
                double sig_model = estimate_model_implied_volatility_fd(St, tt, h, lev);
                
                if (sig_model <= 1e-6) {
                    sig_model = sig_target; // Fallback to avoid division by zero
                }
                
                // Update leverage with damping
                double ratio = sig_target / sig_model;
                double new_leverage = lev.L[j][i] * ratio;
                
                // Apply damping for stability
                new_leverage = lev.L[j][i] + config.damping_factor * (new_leverage - lev.L[j][i]);
                
                // Apply bounds
                new_leverage = std::max(config.min_leverage, std::min(config.max_leverage, new_leverage));
                
                // Track convergence
                double error = std::abs(sig_target - sig_model) / sig_target;
                max_error = std::max(max_error, error);
                
                lev.L[j][i] = new_leverage;
            }
        }
        
        // Check convergence
        if (max_error < config.tolerance) {
            break;
        }
        
        // Reduce damping as we converge
        config.damping_factor *= 0.9;
    }
}

// Alternative calibration using Monte Carlo estimation
inline void calibrate_leverage_mc_based(const DupireSurface& target, const HestonParams& h,
                                        LeverageGrid& lev, const SLVCalibrationConfig& config = {}) {
    if (lev.t.empty() || lev.S.empty()) {
        return;
    }
    
    if (lev.L.empty()) {
        lev.L.resize(lev.t.size(), std::vector<double>(lev.S.size(), 1.0));
    }
    
    const double r = 0.05;
    
    for (int iter = 0; iter < config.max_iterations; ++iter) {
        double max_error = 0.0;
        
        for (size_t j = 0; j < lev.t.size(); ++j) {
            double T = lev.t[j];
            if (T <= 1e-6) continue;
            
            for (size_t i = 0; i < lev.S.size(); ++i) {
                double S0 = lev.S[i];
                double K = S0; // ATM for local vol estimation
                double sig_target = target.at(j, i);
                
                if (sig_target <= 0.0) continue;
                
                // Use MC to estimate model-implied local volatility
                double sig_model = estimate_model_implied_volatility_mc(
                    S0, 0.0, K, r, T, h, lev, config);
                
                if (sig_model <= 1e-6) {
                    sig_model = sig_target;
                }
                
                double ratio = sig_target / sig_model;
                double new_leverage = lev.L[j][i] * ratio;
                
                new_leverage = lev.L[j][i] + config.damping_factor * (new_leverage - lev.L[j][i]);
                new_leverage = std::max(config.min_leverage, std::min(config.max_leverage, new_leverage));
                
                double error = std::abs(sig_target - sig_model) / sig_target;
                max_error = std::max(max_error, error);
                
                lev.L[j][i] = new_leverage;
            }
        }
        
        if (max_error < config.tolerance) {
            break;
        }
    }
}

}
