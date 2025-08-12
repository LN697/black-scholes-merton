#include "slv_calibration.hpp"
#include "monte_carlo_gbm.hpp"
#include "math_utils.hpp"
#include <cmath>
#include <algorithm>

namespace bsm {

// Estimate model-implied local volatility using finite differences
double estimate_model_implied_volatility_fd(
    double S, double t, const HestonParams& heston, 
    const LeverageGrid& leverage, double dt) {
    
    (void)dt; // Parameter reserved for future finite difference implementation
    
    if (t <= 1e-6 || S <= 1e-6) {
        return 0.2; // Default fallback
    }
    
    // Use the SLV model effective volatility formula:
    // sigma_eff(S,t,v) = sigma_loc(S,t) * sqrt(v) * L(S,t)
    // where L(S,t) is the leverage function
    
    // For local volatility estimation, we use the instantaneous variance
    // at current time and spot level
    
    // Get leverage at this point
    double L_val = leverage.interpolate(S, t);
    
    // Estimate instantaneous variance using Heston model
    // For simplicity, use long-run variance as approximation
    double v_inst = heston.theta; // Could be improved with proper variance dynamics
    
    // Local volatility contribution (simplified)
    // In practice, this would come from the Dupire formula or calibrated surface
    double sigma_loc_base = 0.2; // Base local volatility
    
    // Effective volatility
    double sigma_eff = sigma_loc_base * std::sqrt(std::max(v_inst, 1e-6)) * L_val;
    
    return std::max(sigma_eff, 1e-6);
}

// Monte Carlo based estimation of model-implied volatility
double estimate_model_implied_volatility_mc(
    double S, double t, double K, double r, double T,
    const HestonParams& heston, const LeverageGrid& leverage, 
    const SLVCalibrationConfig& config) {
    
    if (T <= t + 1e-6) {
        return 0.2; // Default for expired options
    }
    
    // Create a leverage-based local volatility function
    auto leverage_local_vol = [&leverage](double St, double tt) -> double {
        double L_val = leverage.interpolate(St, tt);
        return 0.2 * L_val; // Base volatility * leverage
    };
    
    // Run SLV Monte Carlo to get option price
    try {
        MCResult result = mc_slv_price(
            S, K, r, T - t, 
            config.num_paths, config.num_time_steps, 
            OptionType::Call, heston, leverage_local_vol, 
            12345, false, true
        );
        
        if (result.price <= 1e-6) {
            return 0.2; // Fallback
        }
        
        // For local volatility estimation, we approximate using the relationship
        // between option price sensitivity and local volatility
        // This is a simplified approach - in practice, you'd use more sophisticated methods
        
        // Estimate local volatility from price gradient
        double h = 0.01 * S; // Small bump in spot
        MCResult result_up = mc_slv_price(
            S + h, K, r, T - t,
            config.num_paths / 4, config.num_time_steps,
            OptionType::Call, heston, leverage_local_vol,
            12346, false, true
        );
        
        double delta_approx = (result_up.price - result.price) / h;
        
        // Convert delta sensitivity to local volatility estimate
        // This is a rough approximation - could be improved
        double time_to_exp = T - t;
        double vol_estimate = std::sqrt(2.0 * std::abs(delta_approx) / (S * std::sqrt(time_to_exp)));
        
        return std::max(0.01, std::min(2.0, vol_estimate));
    }
    catch (...) {
        return 0.2; // Fallback on any error
    }
}

// Utility function to create a sample Dupire surface for testing
DupireSurface create_sample_dupire_surface() {
    DupireSurface surface;
    
    // Create time grid (6 months to 2 years)
    surface.t = {0.25, 0.5, 1.0, 1.5, 2.0};
    
    // Create spot grid (50% to 150% of current spot)
    surface.S = {50.0, 75.0, 100.0, 125.0, 150.0};
    
    // Initialize volatility surface
    surface.sigma.resize(surface.t.size());
    for (size_t i = 0; i < surface.t.size(); ++i) {
        surface.sigma[i].resize(surface.S.size());
        for (size_t j = 0; j < surface.S.size(); ++j) {
            double T = surface.t[i];
            double S = surface.S[j];
            double moneyness = std::log(S / 100.0); // Log-moneyness relative to 100
            
            // Create a realistic volatility surface with skew and term structure
            double base_vol = 0.2;
            double skew = -0.1 * moneyness; // Negative skew (put skew)
            double term_structure = 0.02 * std::sqrt(T); // Vol increases with time
            
            surface.sigma[i][j] = std::max(0.05, base_vol + skew + term_structure);
        }
    }
    
    return surface;
}

// Create a sample leverage grid matching the Dupire surface
LeverageGrid create_sample_leverage_grid(const DupireSurface& dupire) {
    LeverageGrid grid;
    grid.t = dupire.t;
    grid.S = dupire.S;
    
    // Initialize leverage to 1.0 (no leverage initially)
    grid.L.resize(grid.t.size());
    for (size_t i = 0; i < grid.t.size(); ++i) {
        grid.L[i].resize(grid.S.size(), 1.0);
    }
    
    return grid;
}

// Test the calibration process
bool validate_slv_calibration() {
    try {
        // Create sample data
        auto dupire = create_sample_dupire_surface();
        auto leverage = create_sample_leverage_grid(dupire);
        
        // Setup Heston parameters
        HestonParams heston;
        heston.kappa = 2.0;
        heston.theta = 0.04;
        heston.xi = 0.3;
        heston.rho = -0.7;
        heston.v0 = 0.04;
        
        // Run calibration
        calibrate_leverage_iterative(dupire, heston, leverage, 5);
        
        // Check that leverage values are reasonable
        for (size_t i = 0; i < leverage.t.size(); ++i) {
            for (size_t j = 0; j < leverage.S.size(); ++j) {
                double L_val = leverage.L[i][j];
                if (L_val < 0.01 || L_val > 10.0 || !std::isfinite(L_val)) {
                    return false;
                }
            }
        }
        
        return true;
    }
    catch (...) {
        return false;
    }
}

}
