/**
 * @file option_chain_analysis.cpp
 * @brief Comprehensive analysis of real option chain data using BSM toolkit
 * 
 * This program analyzes option chain data to:
 * - Calculate theoretical prices using Black-Scholes
 * - Compute implied volatilities from market prices
 * - Analyze volatility smile/skew patterns
 * - Identify arbitrage opportunities
 * - Compute Greeks for risk management
 * - Perform statistical analysis of pricing discrepancies
 * 
 * @author Black-Scholes-Merton Pricing Toolkit
 * @version 1.0
 */

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "analytic_bs.hpp"
#include "iv_solve.hpp"
#include "monte_carlo_gbm.hpp"
#include "stats.hpp"
#include "math_utils.hpp"

using namespace bsm;

/**
 * @brief Structure to hold option data from market
 */
struct OptionData {
    double strike;
    double bid;
    double ask;
    double last;
    double volume;
    double open_interest;
    double iv_market;  // Market implied volatility (if available)
    OptionType type;
    
    double mid_price() const { return (bid + ask) / 2.0; }
    double spread() const { return ask - bid; }
    double spread_pct() const { return spread() / mid_price() * 100.0; }
};

/**
 * @brief Structure to hold analysis results
 */
struct OptionAnalysis {
    OptionData market_data;
    double theoretical_price;
    double implied_vol;
    double price_discrepancy;
    double price_discrepancy_pct;
    
    // Greeks
    double delta;
    double gamma;
    double vega;
    double theta;
    double rho;
    
    // Risk metrics
    bool arbitrage_opportunity;
    double profit_potential;
};

/**
 * @brief Parse option chain data from the image
 * Note: This data appears to be calls and puts for different strikes
 */
std::vector<OptionData> load_option_chain_data() {
    std::vector<OptionData> options;
    
    // Data extracted from the option chain image
    // Format: Strike, Call Bid, Call Ask, Call Last, Call Volume, Call OI, Call IV,
    //         Put Bid, Put Ask, Put Last, Put Volume, Put OI, Put IV
    
    struct RawData {
        double strike;
        double call_bid, call_ask, call_last, call_vol, call_oi, call_iv;
        double put_bid, put_ask, put_last, put_vol, put_oi, put_iv;
    };
    
    // Data from the option chain (representative sample)
    std::vector<RawData> raw_data = {
        {23800, 604.80, 0.44, 45.57, 0, 11.40, 53.02, 33.21, 0, 47.34, 0, 0, 0},
        {23850, 560.40, 0.06, 32.00, 0, 14.80, 76.19, 6.31, 0, 26.42, 0, 0, 0},
        {23900, 516.10, 0.29, 59.26, 0, 17.25, 77.84, 19.99, 0, 38.50, 0, 0, 0},
        {23950, 467.95, 0.10, 282.86, 0, 20.60, 86.43, 7.56, 0, 47.07, 0, 0, 0},
        {24000, 415.55, 5.20, 52.41, 0, 25.65, 100.39, 58.11, 0, 25.83, 0, 0, 0},
        {24050, 374.95, 0.22, 208.60, 0, 31.45, 115.41, 11.09, 0, 65.58, 0, 0, 0},
        {24100, 331.70, 1.16, 78.70, 0, 38.40, 121.97, 22.34, 0, 68.14, 0, 0, 0},
        {24150, 293.00, 0.54, 280.95, 0, 47.40, 129.54, 11.98, 0, 110.62, 0, 0, 0},
        {24200, 251.55, 5.20, 100.20, 0, 59.00, 144.86, 39.66, 0, 20.79, 0, 0, 0},
        {24250, 214.80, 1.55, 364.86, 0, 72.70, 149.81, 14.77, 0, 81.80, 0, 0, 0},
        {24300, 183.05, 13.65, 269.43, 0, 89.50, 181.40, 49.24, 0, 97.37, 0, 0, 0},
        {24350, 152.45, 11.50, 528.50, 0, 109.90, 147.24, 20.69, 0, 35.86, 0, 0, 0},
        {24400, 125.60, 45.14, 325.45, 0, 132.85, 148.09, 42.48, 0, 18.37, 0, 0, 0},
        {24450, 103.30, 37.66, 614.79, 0, 160.30, 146.57, 19.84, 0, 65.23, 0, 0, 0},
        {24500, 83.20, 116.89, 366.30, 0, 189.95, 131.61, 49.56, 0, 7.11, 0, 0, 0},
        {24550, 66.70, 41.69, 526.68, 0, 223.75, 128.43, 3.44, 0, -16.84, 0, 0, 0},
        {24600, 53.40, 109.04, 254.50, 0, 259.00, 123.56, 22.37, 0, 24.18, 0, 0, 0},
        {24650, 42.55, 37.07, 273.22, 0, 297.95, 120.62, 4.70, 0, -51.91, 0, 0, 0},
        {24700, 33.10, 81.23, 182.79, 0, 341.00, 106.48, 12.51, 0, -11.43, 0, 0, 0},
        {24750, 26.00, 30.06, 202.00, 0, 382.40, 44.75, 2.40, 0, -1.75, 0, 0, 0},
        {24800, 20.40, 79.86, 159.91, 0, 427.95, 89.44, 5.52, 0, 22.75, 0, 0, 0},
        {24850, 16.65, 25.29, 208.38, 0, 470.00, 77.36, 0.51, 0, 33.07, 0, 0, 0},
        {24900, 12.70, 60.40, 163.16, 0, 516.05, 71.22, 4.78, 0, -5.30, 0, 0, 0}
    };
    
    for (const auto& data : raw_data) {
        // Add call option
        if (data.call_bid > 0.01 || data.call_ask > 0.01) {
            OptionData call;
            call.strike = data.strike;
            call.bid = data.call_bid;
            call.ask = data.call_ask;
            call.last = data.call_last;
            call.volume = data.call_vol;
            call.open_interest = data.call_oi;
            call.iv_market = data.call_iv / 100.0;  // Convert percentage to decimal
            call.type = OptionType::Call;
            options.push_back(call);
        }
        
        // Add put option
        if (data.put_bid > 0.01 || data.put_ask > 0.01) {
            OptionData put;
            put.strike = data.strike;
            put.bid = data.put_bid;
            put.ask = data.put_ask;
            put.last = data.put_last;
            put.volume = data.put_vol;
            put.open_interest = data.put_oi;
            put.iv_market = data.put_iv / 100.0;  // Convert percentage to decimal
            put.type = OptionType::Put;
            options.push_back(put);
        }
    }
    
    return options;
}

/**
 * @brief Analyze individual option
 */
OptionAnalysis analyze_option(const OptionData& option, double S0, double r, double T) {
    OptionAnalysis analysis;
    analysis.market_data = option;
    
    // Estimate implied volatility from market price (mid-price)
    double market_price = option.mid_price();
    
    auto bs_price_fn = [=](double vol) {
        return black_scholes_price(S0, option.strike, r, T, vol, option.type);
    };
    
    // Calculate implied volatility
    analysis.implied_vol = implied_vol(market_price, bs_price_fn);
    
    // Calculate theoretical price using implied volatility
    if (std::isfinite(analysis.implied_vol) && analysis.implied_vol > 0.0) {
        analysis.theoretical_price = black_scholes_price(S0, option.strike, r, T, 
                                                       analysis.implied_vol, option.type);
        
        // Calculate Greeks
        analysis.delta = black_scholes_delta(S0, option.strike, r, T, analysis.implied_vol, option.type);
        analysis.gamma = black_scholes_gamma(S0, option.strike, r, T, analysis.implied_vol);
        analysis.vega = black_scholes_vega(S0, option.strike, r, T, analysis.implied_vol);
        analysis.theta = black_scholes_theta(S0, option.strike, r, T, analysis.implied_vol, option.type);
        analysis.rho = black_scholes_rho(S0, option.strike, r, T, analysis.implied_vol, option.type);
    } else {
        // Use market IV if implied vol calculation fails
        double vol_estimate = std::max(0.1, option.iv_market);
        analysis.implied_vol = vol_estimate;
        analysis.theoretical_price = black_scholes_price(S0, option.strike, r, T, vol_estimate, option.type);
        
        // Calculate Greeks with estimated volatility
        analysis.delta = black_scholes_delta(S0, option.strike, r, T, vol_estimate, option.type);
        analysis.gamma = black_scholes_gamma(S0, option.strike, r, T, vol_estimate);
        analysis.vega = black_scholes_vega(S0, option.strike, r, T, vol_estimate);
        analysis.theta = black_scholes_theta(S0, option.strike, r, T, vol_estimate, option.type);
        analysis.rho = black_scholes_rho(S0, option.strike, r, T, vol_estimate, option.type);
    }
    
    // Calculate price discrepancy
    analysis.price_discrepancy = market_price - analysis.theoretical_price;
    analysis.price_discrepancy_pct = (analysis.price_discrepancy / analysis.theoretical_price) * 100.0;
    
    // Check for arbitrage opportunities (simplified)
    analysis.arbitrage_opportunity = false;
    analysis.profit_potential = 0.0;
    
    // Basic arbitrage check: if market price is significantly below theoretical
    if (market_price < analysis.theoretical_price * 0.95) {
        analysis.arbitrage_opportunity = true;
        analysis.profit_potential = analysis.theoretical_price - option.ask;
    }
    
    return analysis;
}

/**
 * @brief Print detailed analysis results
 */
void print_analysis(const std::vector<OptionAnalysis>& analyses, double S0) {
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "\n=== OPTION CHAIN ANALYSIS RESULTS ===\n";
    std::cout << "Underlying Price: $" << S0 << "\n\n";
    
    // Header
    std::cout << std::setw(6) << "Type" 
              << std::setw(8) << "Strike"
              << std::setw(8) << "Bid"
              << std::setw(8) << "Ask"
              << std::setw(8) << "Mid"
              << std::setw(8) << "Theory"
              << std::setw(8) << "IV%"
              << std::setw(8) << "Delta"
              << std::setw(8) << "Gamma"
              << std::setw(8) << "Vega"
              << std::setw(8) << "Theta"
              << std::setw(8) << "Disc%"
              << std::endl;
    
    std::cout << std::string(96, '-') << std::endl;
    
    // Print results
    for (const auto& analysis : analyses) {
        const auto& opt = analysis.market_data;
        
        std::cout << std::setw(6) << (opt.type == OptionType::Call ? "Call" : "Put")
                  << std::setw(8) << opt.strike
                  << std::setw(8) << opt.bid
                  << std::setw(8) << opt.ask
                  << std::setw(8) << opt.mid_price()
                  << std::setw(8) << analysis.theoretical_price
                  << std::setw(7) << (analysis.implied_vol * 100.0) << "%"
                  << std::setw(8) << analysis.delta
                  << std::setw(8) << analysis.gamma
                  << std::setw(8) << analysis.vega
                  << std::setw(8) << analysis.theta
                  << std::setw(7) << analysis.price_discrepancy_pct << "%"
                  << std::endl;
    }
}

/**
 * @brief Analyze volatility smile/skew
 */
void analyze_volatility_smile(const std::vector<OptionAnalysis>& analyses, double S0) {
    std::cout << "\n=== VOLATILITY SMILE ANALYSIS ===\n";
    
    // Separate calls and puts
    std::vector<OptionAnalysis> calls, puts;
    for (const auto& analysis : analyses) {
        if (analysis.market_data.type == OptionType::Call) {
            calls.push_back(analysis);
        } else {
            puts.push_back(analysis);
        }
    }
    
    // Sort by strike
    auto sort_by_strike = [](const OptionAnalysis& a, const OptionAnalysis& b) {
        return a.market_data.strike < b.market_data.strike;
    };
    
    std::sort(calls.begin(), calls.end(), sort_by_strike);
    std::sort(puts.begin(), puts.end(), sort_by_strike);
    
    // Analyze calls
    if (!calls.empty()) {
        std::cout << "\nCall Options Volatility Profile:\n";
        std::cout << std::setw(8) << "Strike" 
                  << std::setw(12) << "Moneyness"
                  << std::setw(10) << "IV%"
                  << std::setw(12) << "Bid-Ask%"
                  << std::setw(10) << "Volume"
                  << std::endl;
        std::cout << std::string(52, '-') << std::endl;
        
        for (const auto& call : calls) {
            double moneyness = call.market_data.strike / S0;
            std::cout << std::setw(8) << call.market_data.strike
                      << std::setw(12) << moneyness
                      << std::setw(9) << (call.implied_vol * 100.0) << "%"
                      << std::setw(11) << call.market_data.spread_pct() << "%"
                      << std::setw(10) << call.market_data.volume
                      << std::endl;
        }
    }
    
    // Analyze puts
    if (!puts.empty()) {
        std::cout << "\nPut Options Volatility Profile:\n";
        std::cout << std::setw(8) << "Strike" 
                  << std::setw(12) << "Moneyness"
                  << std::setw(10) << "IV%"
                  << std::setw(12) << "Bid-Ask%"
                  << std::setw(10) << "Volume"
                  << std::endl;
        std::cout << std::string(52, '-') << std::endl;
        
        for (const auto& put : puts) {
            double moneyness = put.market_data.strike / S0;
            std::cout << std::setw(8) << put.market_data.strike
                      << std::setw(12) << moneyness
                      << std::setw(9) << (put.implied_vol * 100.0) << "%"
                      << std::setw(11) << put.market_data.spread_pct() << "%"
                      << std::setw(10) << put.market_data.volume
                      << std::endl;
        }
    }
}

/**
 * @brief Statistical analysis of the option chain
 */
void statistical_analysis(const std::vector<OptionAnalysis>& analyses) {
    std::cout << "\n=== STATISTICAL ANALYSIS ===\n";
    
    std::vector<double> implied_vols, price_discrepancies, deltas, vegas;
    
    for (const auto& analysis : analyses) {
        if (std::isfinite(analysis.implied_vol) && analysis.implied_vol > 0) {
            implied_vols.push_back(analysis.implied_vol);
            price_discrepancies.push_back(std::abs(analysis.price_discrepancy_pct));
            deltas.push_back(std::abs(analysis.delta));
            vegas.push_back(analysis.vega);
        }
    }
    
    if (!implied_vols.empty()) {
        std::cout << "Implied Volatility Statistics:\n";
        std::cout << "  Average IV: " << std::setprecision(2) << (mean(implied_vols) * 100.0) << "%\n";
        std::cout << "  IV Std Dev: " << std::setprecision(2) << (standard_deviation(implied_vols) * 100.0) << "%\n";
        std::cout << "  Min IV: " << std::setprecision(2) << (*std::min_element(implied_vols.begin(), implied_vols.end()) * 100.0) << "%\n";
        std::cout << "  Max IV: " << std::setprecision(2) << (*std::max_element(implied_vols.begin(), implied_vols.end()) * 100.0) << "%\n";
        
        std::cout << "\nPrice Discrepancy Statistics:\n";
        std::cout << "  Average |Discrepancy|: " << std::setprecision(2) << mean(price_discrepancies) << "%\n";
        std::cout << "  Max |Discrepancy|: " << std::setprecision(2) << *std::max_element(price_discrepancies.begin(), price_discrepancies.end()) << "%\n";
        
        std::cout << "\nGreeks Statistics:\n";
        std::cout << "  Average |Delta|: " << std::setprecision(3) << mean(deltas) << "\n";
        std::cout << "  Average Vega: " << std::setprecision(2) << mean(vegas) << "\n";
    }
}

/**
 * @brief Check for arbitrage opportunities
 */
void arbitrage_analysis(const std::vector<OptionAnalysis>& analyses) {
    std::cout << "\n=== ARBITRAGE ANALYSIS ===\n";
    
    std::vector<OptionAnalysis> arbitrage_ops;
    for (const auto& analysis : analyses) {
        if (analysis.arbitrage_opportunity) {
            arbitrage_ops.push_back(analysis);
        }
    }
    
    if (arbitrage_ops.empty()) {
        std::cout << "No obvious arbitrage opportunities detected.\n";
    } else {
        std::cout << "Potential arbitrage opportunities:\n";
        std::cout << std::setw(6) << "Type" 
                  << std::setw(8) << "Strike"
                  << std::setw(8) << "Ask"
                  << std::setw(8) << "Theory"
                  << std::setw(10) << "Profit"
                  << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        
        for (const auto& arb : arbitrage_ops) {
            std::cout << std::setw(6) << (arb.market_data.type == OptionType::Call ? "Call" : "Put")
                      << std::setw(8) << arb.market_data.strike
                      << std::setw(8) << arb.market_data.ask
                      << std::setw(8) << arb.theoretical_price
                      << std::setw(10) << arb.profit_potential
                      << std::endl;
        }
    }
}

/**
 * @brief Main analysis function
 */
int main() {
    std::cout << "Black-Scholes-Merton Option Chain Analysis\n";
    std::cout << "==========================================\n";
    
    // Market parameters (estimated from option chain data)
    const double S0 = 24300.0;        // Underlying price (approximate from strikes)
    const double r = 0.05;            // Risk-free rate (5%)
    const double T = 30.0 / 365.0;    // Time to expiration (30 days)
    
    std::cout << "Market Parameters:\n";
    std::cout << "  Underlying Price: $" << S0 << "\n";
    std::cout << "  Risk-free Rate: " << (r * 100.0) << "%\n";
    std::cout << "  Time to Expiration: " << (T * 365.0) << " days\n";
    
    // Load and analyze option chain data
    auto options = load_option_chain_data();
    std::cout << "  Total Options Analyzed: " << options.size() << "\n";
    
    // Analyze each option
    std::vector<OptionAnalysis> analyses;
    for (const auto& option : options) {
        analyses.push_back(analyze_option(option, S0, r, T));
    }
    
    // Print comprehensive analysis
    print_analysis(analyses, S0);
    analyze_volatility_smile(analyses, S0);
    statistical_analysis(analyses);
    arbitrage_analysis(analyses);
    
    // Monte Carlo validation for selected options
    std::cout << "\n=== MONTE CARLO VALIDATION ===\n";
    std::cout << "Validating theoretical prices with Monte Carlo simulation...\n";
    
    // Pick an ATM option for validation
    auto atm_it = std::min_element(analyses.begin(), analyses.end(),
        [S0](const OptionAnalysis& a, const OptionAnalysis& b) {
            return std::abs(a.market_data.strike - S0) < std::abs(b.market_data.strike - S0);
        });
    
    if (atm_it != analyses.end() && atm_it->market_data.type == OptionType::Call) {
        const auto& atm_analysis = *atm_it;
        
        MCResult mc_result = mc_gbm_price(S0, atm_analysis.market_data.strike, r, T, 
                                        atm_analysis.implied_vol, 500000, 
                                        OptionType::Call, 42UL, true, true, false, true, true);
        
        std::cout << "ATM Call Strike " << atm_analysis.market_data.strike << ":\n";
        std::cout << "  Black-Scholes Price: $" << std::setprecision(2) << atm_analysis.theoretical_price << "\n";
        std::cout << "  Monte Carlo Price: $" << std::setprecision(2) << mc_result.price 
                  << " +/- " << (1.96 * mc_result.std_error) << "\n";
        std::cout << "  Market Mid Price: $" << std::setprecision(2) << atm_analysis.market_data.mid_price() << "\n";
        std::cout << "  MC vs BS Error: " << std::setprecision(3) 
                  << std::abs(mc_result.price - atm_analysis.theoretical_price) << "\n";
    }
    
    std::cout << "\nAnalysis complete!\n";
    return 0;
}
