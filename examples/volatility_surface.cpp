/**
 * @file volatility_surface.cpp
 * @brief Advanced volatility surface analysis and modeling
 * 
 * This program creates and analyzes volatility surfaces from option chain data:
 * - Build implied volatility surface
 * - Fit volatility smile models (Heston, SLV)
 * - Analyze term structure of volatility
 * - Export data for visualization
 * - Calibrate stochastic volatility models
 * 
 * @author Black-Scholes-Merton Pricing Toolkit
 * @version 1.0
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

#include "analytic_bs.hpp"
#include "iv_solve.hpp"
#include "slv.hpp"
#include "monte_carlo_gbm.hpp"
#include "stats.hpp"

using namespace bsm;

/**
 * @brief Volatility surface point
 */
struct VolPoint {
    double strike;
    double time_to_expiry;
    double implied_vol;
    double market_price;
    double volume;
    OptionType type;
    
    double moneyness(double S0) const { return strike / S0; }
    double log_moneyness(double S0) const { return std::log(strike / S0); }
};

/**
 * @brief Create volatility surface from option chain
 */
std::vector<VolPoint> create_volatility_surface(double S0, double r) {
    std::vector<VolPoint> surface;
    
    // Option chain data for 30-day expiration
    // In practice, you'd have multiple expirations
    const double T = 30.0 / 365.0;
    
    struct OptionDataPoint {
        double strike;
        double call_mid, put_mid;
        double call_vol, put_vol;
    };
    
    // Sample data points (from the option chain image)
    std::vector<OptionDataPoint> data = {
        {23800, 302.60, 16.605, 0, 0},
        {23900, 258.20, 19.995, 0, 0},
        {24000, 215.35, 28.98, 0, 0},
        {24100, 175.25, 22.34, 0, 0},
        {24200, 138.37, 39.66, 0, 0},
        {24300, 108.32, 49.24, 0, 0},
        {24400, 85.995, 42.48, 0, 0},
        {24500, 66.30, 49.56, 0, 0},
        {24600, 53.40, 22.37, 0, 0},
        {24700, 42.55, 12.51, 0, 0},
        {24800, 33.10, 5.52, 0, 0},
        {24900, 25.20, 4.78, 0, 0}
    };
    
    for (const auto& point : data) {
        // Process call option
        if (point.call_mid > 1.0) {  // Only include liquid options
            VolPoint vol_point;
            vol_point.strike = point.strike;
            vol_point.time_to_expiry = T;
            vol_point.market_price = point.call_mid;
            vol_point.volume = point.call_vol;
            vol_point.type = OptionType::Call;
            
            // Calculate implied volatility
            auto bs_price_fn = [=](double vol) {
                return black_scholes_price(S0, point.strike, r, T, vol, OptionType::Call);
            };
            
            vol_point.implied_vol = implied_vol(point.call_mid, bs_price_fn);
            
            if (std::isfinite(vol_point.implied_vol) && vol_point.implied_vol > 0.01) {
                surface.push_back(vol_point);
            }
        }
        
        // Process put option
        if (point.put_mid > 1.0) {  // Only include liquid options
            VolPoint vol_point;
            vol_point.strike = point.strike;
            vol_point.time_to_expiry = T;
            vol_point.market_price = point.put_mid;
            vol_point.volume = point.put_vol;
            vol_point.type = OptionType::Put;
            
            // Calculate implied volatility
            auto bs_price_fn = [=](double vol) {
                return black_scholes_price(S0, point.strike, r, T, vol, OptionType::Put);
            };
            
            vol_point.implied_vol = implied_vol(point.put_mid, bs_price_fn);
            
            if (std::isfinite(vol_point.implied_vol) && vol_point.implied_vol > 0.01) {
                surface.push_back(vol_point);
            }
        }
    }
    
    return surface;
}

/**
 * @brief Analyze volatility smile characteristics
 */
void analyze_volatility_smile(const std::vector<VolPoint>& surface, double S0) {
    std::cout << "\n=== VOLATILITY SMILE ANALYSIS ===\n";
    
    // Separate by option type
    std::vector<VolPoint> calls, puts;
    for (const auto& point : surface) {
        if (point.type == OptionType::Call) {
            calls.push_back(point);
        } else {
            puts.push_back(point);
        }
    }
    
    // Sort by moneyness
    auto sort_by_moneyness = [S0](const VolPoint& a, const VolPoint& b) {
        return a.moneyness(S0) < b.moneyness(S0);
    };
    
    std::sort(calls.begin(), calls.end(), sort_by_moneyness);
    std::sort(puts.begin(), puts.end(), sort_by_moneyness);
    
    std::cout << std::fixed << std::setprecision(4);
    
    // Analyze call smile
    if (!calls.empty()) {
        std::cout << "\nCall Options Volatility Smile:\n";
        std::cout << std::setw(8) << "Strike" 
                  << std::setw(12) << "Moneyness"
                  << std::setw(12) << "Log(K/S)"
                  << std::setw(10) << "IV"
                  << std::setw(12) << "Market Price"
                  << std::endl;
        std::cout << std::string(56, '-') << std::endl;
        
        for (const auto& call : calls) {
            std::cout << std::setw(8) << call.strike
                      << std::setw(12) << call.moneyness(S0)
                      << std::setw(12) << call.log_moneyness(S0)
                      << std::setw(9) << (call.implied_vol * 100.0) << "%"
                      << std::setw(12) << call.market_price
                      << std::endl;
        }
    }
    
    // Analyze put smile
    if (!puts.empty()) {
        std::cout << "\nPut Options Volatility Smile:\n";
        std::cout << std::setw(8) << "Strike" 
                  << std::setw(12) << "Moneyness"
                  << std::setw(12) << "Log(K/S)"
                  << std::setw(10) << "IV"
                  << std::setw(12) << "Market Price"
                  << std::endl;
        std::cout << std::string(56, '-') << std::endl;
        
        for (const auto& put : puts) {
            std::cout << std::setw(8) << put.strike
                      << std::setw(12) << put.moneyness(S0)
                      << std::setw(12) << put.log_moneyness(S0)
                      << std::setw(9) << (put.implied_vol * 100.0) << "%"
                      << std::setw(12) << put.market_price
                      << std::endl;
        }
    }
    
    // Calculate smile characteristics
    std::vector<double> all_ivs;
    std::vector<double> all_moneyness;
    
    for (const auto& point : surface) {
        all_ivs.push_back(point.implied_vol);
        all_moneyness.push_back(point.moneyness(S0));
    }
    
    if (!all_ivs.empty()) {
        double min_iv = *std::min_element(all_ivs.begin(), all_ivs.end());
        double max_iv = *std::max_element(all_ivs.begin(), all_ivs.end());
        double avg_iv = std::accumulate(all_ivs.begin(), all_ivs.end(), 0.0) / all_ivs.size();
        
        std::cout << "\nVolatility Smile Statistics:\n";
        std::cout << "  Average IV: " << std::setprecision(2) << (avg_iv * 100.0) << "%\n";
        std::cout << "  Min IV: " << std::setprecision(2) << (min_iv * 100.0) << "%\n";
        std::cout << "  Max IV: " << std::setprecision(2) << (max_iv * 100.0) << "%\n";
        std::cout << "  IV Range: " << std::setprecision(2) << ((max_iv - min_iv) * 100.0) << "%\n";
        std::cout << "  Smile Steepness: " << std::setprecision(3) << ((max_iv - min_iv) / avg_iv) << "\n";
    }
}

/**
 * @brief Fit SLV model to volatility surface
 */
void fit_slv_model(const std::vector<VolPoint>& surface, double S0, double r) {
    std::cout << "\n=== STOCHASTIC LOCAL VOLATILITY MODEL FITTING ===\n";
    
    if (surface.empty()) {
        std::cout << "No volatility surface data available for SLV fitting.\n";
        return;
    }
    
    // Use representative volatility for initial calibration
    double avg_iv = 0.0;
    for (const auto& point : surface) {
        avg_iv += point.implied_vol;
    }
    avg_iv /= surface.size();
    
    std::cout << "Average implied volatility: " << std::setprecision(2) << (avg_iv * 100.0) << "%\n";
    
    // Set up Heston parameters (initial guess)
    HestonParams heston;
    heston.v0 = avg_iv * avg_iv;  // Initial variance
    heston.kappa = 2.0;           // Mean reversion speed
    heston.theta = avg_iv * avg_iv; // Long-term variance
    heston.xi = 0.3;              // Vol of vol
    heston.rho = -0.7;            // Correlation
    
    std::cout << "\nInitial Heston Parameters:\n";
    std::cout << "  v0 (initial variance): " << std::setprecision(4) << heston.v0 << "\n";
    std::cout << "  kappa (mean reversion): " << std::setprecision(2) << heston.kappa << "\n";
    std::cout << "  theta (long-term var): " << std::setprecision(4) << heston.theta << "\n";
    std::cout << "  xi (vol of vol): " << std::setprecision(2) << heston.xi << "\n";
    std::cout << "  rho (correlation): " << std::setprecision(2) << heston.rho << "\n";
    
    // Test local volatility models
    std::cout << "\nTesting Local Volatility Models:\n";
    
    // CEV model
    CEVLocalVol cev_model{avg_iv, 0.8, S0};
    auto cev_fn = cev_model.to_fn();
    
    std::cout << "CEV Model:\n";
    std::cout << "  sigma_0 = " << std::setprecision(3) << avg_iv << "\n";
    std::cout << "  beta = 0.8\n";
    std::cout << "  S_0 = " << S0 << "\n";
    
    // Test CEV local vol at different levels
    std::vector<double> test_spots = {S0 * 0.9, S0, S0 * 1.1};
    for (double S : test_spots) {
        double local_vol = cev_fn(S, 0.1);
        std::cout << "    sigma_local(" << S << ") = " << std::setprecision(3) << (local_vol * 100.0) << "%\n";
    }
    
    // Smile model
    SmileLocalVol smile_model{avg_iv, 0.95, 0.25, 0.15, S0, 0.01};
    auto smile_fn = smile_model.to_fn();
    
    std::cout << "\nSmile Model:\n";
    std::cout << "  sigma_atm = " << std::setprecision(3) << avg_iv << "\n";
    std::cout << "  p = 0.95\n";
    std::cout << "  nu = 0.25\n";
    std::cout << "  rho = 0.15\n";
    
    // Test Smile local vol at different levels
    for (double S : test_spots) {
        double local_vol = smile_fn(S, 0.1);
        std::cout << "    sigma_local(" << S << ") = " << std::setprecision(3) << (local_vol * 100.0) << "%\n";
    }
    
    // Price a representative option using SLV
    auto atm_it = std::min_element(surface.begin(), surface.end(),
        [S0](const VolPoint& a, const VolPoint& b) {
            return std::abs(a.strike - S0) < std::abs(b.strike - S0);
        });
    
    if (atm_it != surface.end()) {
        const auto& atm_option = *atm_it;
        
        std::cout << "\nSLV Pricing Validation (Strike " << atm_option.strike << "):\n";
        
        try {
            // Price with SLV using CEV local vol
            MCResult slv_cev = mc_slv_price(S0, atm_option.strike, r, atm_option.time_to_expiry,
                                          50000, 100, atm_option.type, heston, cev_fn, 777UL, true, true);
            
            // Price with SLV using Smile local vol
            MCResult slv_smile = mc_slv_price(S0, atm_option.strike, r, atm_option.time_to_expiry,
                                            50000, 100, atm_option.type, heston, smile_fn, 888UL, true, true);
            
            // Compare with Black-Scholes
            double bs_price = black_scholes_price(S0, atm_option.strike, r, atm_option.time_to_expiry,
                                                atm_option.implied_vol, atm_option.type);
            
            std::cout << "  Market Price: $" << std::setprecision(2) << atm_option.market_price << "\n";
            std::cout << "  Black-Scholes: $" << std::setprecision(2) << bs_price << "\n";
            std::cout << "  SLV-CEV: $" << std::setprecision(2) << slv_cev.price 
                      << " +/- " << (1.96 * slv_cev.std_error) << "\n";
            std::cout << "  SLV-Smile: $" << std::setprecision(2) << slv_smile.price 
                      << " +/- " << (1.96 * slv_smile.std_error) << "\n";
            
        } catch (const std::exception& e) {
            std::cout << "  SLV pricing failed: " << e.what() << "\n";
        }
    }
}

/**
 * @brief Export volatility surface data for external analysis
 */
void export_surface_data(const std::vector<VolPoint>& surface, double S0, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing.\n";
        return;
    }
    
    // Write header
    file << "Strike,Moneyness,LogMoneyness,TimeToExpiry,ImpliedVol,MarketPrice,Volume,Type\n";
    
    // Write data
    for (const auto& point : surface) {
        file << std::fixed << std::setprecision(6)
             << point.strike << ","
             << point.moneyness(S0) << ","
             << point.log_moneyness(S0) << ","
             << point.time_to_expiry << ","
             << point.implied_vol << ","
             << point.market_price << ","
             << point.volume << ","
             << (point.type == OptionType::Call ? "Call" : "Put") << "\n";
    }
    
    file.close();
    std::cout << "Volatility surface data exported to: " << filename << "\n";
}

/**
 * @brief Main volatility surface analysis
 */
int main() {
    std::cout << "Black-Scholes-Merton Volatility Surface Analysis\n";
    std::cout << "================================================\n";
    
    // Market parameters
    const double S0 = 24300.0;
    const double r = 0.05;
    
    std::cout << "Market Parameters:\n";
    std::cout << "  Underlying Price: $" << S0 << "\n";
    std::cout << "  Risk-free Rate: " << (r * 100.0) << "%\n";
    
    // Create volatility surface
    auto surface = create_volatility_surface(S0, r);
    std::cout << "  Volatility Points: " << surface.size() << "\n";
    
    if (surface.empty()) {
        std::cout << "No valid volatility surface data found.\n";
        return 1;
    }
    
    // Perform analysis
    analyze_volatility_smile(surface, S0);
    fit_slv_model(surface, S0, r);
    
    // Export data
    export_surface_data(surface, S0, "examples/volatility_surface.csv");
    
    std::cout << "\nVolatility surface analysis complete!\n";
    return 0;
}
