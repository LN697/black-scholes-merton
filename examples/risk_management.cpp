/**
 * @file risk_management.cpp
 * @brief Portfolio risk management and Greeks analysis
 * 
 * This program performs comprehensive risk analysis on option portfolios:
 * - Calculate portfolio Greeks (Delta, Gamma, Vega, Theta)
 * - Value-at-Risk (VaR) and Expected Shortfall calculations
 * - Scenario analysis and stress testing
 * - Hedge ratio calculations
 * - Risk attribution analysis
 * 
 * @author Black-Scholes-Merton Pricing Toolkit
 * @version 1.0
 */

#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <random>
#include <algorithm>

#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"
#include "stats.hpp"
#include "math_utils.hpp"

using namespace bsm;

/**
 * @brief Portfolio position structure
 */
struct Position {
    double strike;
    double time_to_expiry;
    double implied_vol;
    int quantity;  // positive = long, negative = short
    OptionType type;
    
    std::string description() const {
        std::string type_str = (type == OptionType::Call) ? "Call" : "Put";
        std::string pos_str = (quantity > 0) ? "Long" : "Short";
        return pos_str + " " + std::to_string(std::abs(quantity)) + " " + type_str + " " + std::to_string(static_cast<int>(strike));
    }
};

/**
 * @brief Portfolio Greeks structure
 */
struct PortfolioGreeks {
    double delta = 0.0;
    double gamma = 0.0;
    double vega = 0.0;
    double theta = 0.0;
    double rho = 0.0;
    double value = 0.0;
    
    void add(const PortfolioGreeks& other) {
        delta += other.delta;
        gamma += other.gamma;
        vega += other.vega;
        theta += other.theta;
        rho += other.rho;
        value += other.value;
    }
    
    void scale(double factor) {
        delta *= factor;
        gamma *= factor;
        vega *= factor;
        theta *= factor;
        rho *= factor;
        value *= factor;
    }
};

/**
 * @brief Calculate Greeks for a single position
 */
PortfolioGreeks calculate_position_greeks(const Position& pos, double S0, double r) {
    PortfolioGreeks greeks;
    
    // Calculate option price and Greeks
    double price = black_scholes_price(S0, pos.strike, r, pos.time_to_expiry, pos.implied_vol, pos.type);
    double delta = black_scholes_delta(S0, pos.strike, r, pos.time_to_expiry, pos.implied_vol, pos.type);
    double gamma = black_scholes_gamma(S0, pos.strike, r, pos.time_to_expiry, pos.implied_vol);
    double vega = black_scholes_vega(S0, pos.strike, r, pos.time_to_expiry, pos.implied_vol);
    double theta = black_scholes_theta(S0, pos.strike, r, pos.time_to_expiry, pos.implied_vol, pos.type);
    double rho = black_scholes_rho(S0, pos.strike, r, pos.time_to_expiry, pos.implied_vol, pos.type);
    
    // Scale by quantity
    greeks.value = price * pos.quantity;
    greeks.delta = delta * pos.quantity;
    greeks.gamma = gamma * pos.quantity;
    greeks.vega = vega * pos.quantity;
    greeks.theta = theta * pos.quantity;
    greeks.rho = rho * pos.quantity;
    
    return greeks;
}

/**
 * @brief Create sample portfolio from option chain data
 */
std::vector<Position> create_sample_portfolio() {
    std::vector<Position> portfolio;
    
    // Sample portfolio: Short straddle with protective wings
    const double T = 30.0 / 365.0;
    
    // Short ATM straddle
    portfolio.push_back({24300, T, 0.45, -10, OptionType::Call});   // Short 10 ATM calls
    portfolio.push_back({24300, T, 0.45, -10, OptionType::Put});    // Short 10 ATM puts
    
    // Long protective wings
    portfolio.push_back({24500, T, 0.50, 10, OptionType::Call});    // Long 10 OTM calls
    portfolio.push_back({24100, T, 0.50, 10, OptionType::Put});     // Long 10 OTM puts
    
    // Delta hedge with underlying
    portfolio.push_back({0, T, 0.0, 50, OptionType::Call});  // Long 50 shares (represented as strike=0)
    
    return portfolio;
}

/**
 * @brief Calculate total portfolio Greeks
 */
PortfolioGreeks calculate_portfolio_greeks(const std::vector<Position>& portfolio, double S0, double r) {
    PortfolioGreeks total;
    
    for (const auto& pos : portfolio) {
        if (pos.strike == 0) {
            // Handle underlying position (strike = 0 indicates shares)
            total.value += S0 * pos.quantity;
            total.delta += pos.quantity;
            // Other Greeks are zero for underlying
        } else {
            PortfolioGreeks pos_greeks = calculate_position_greeks(pos, S0, r);
            total.add(pos_greeks);
        }
    }
    
    return total;
}

/**
 * @brief Perform scenario analysis
 */
void scenario_analysis(const std::vector<Position>& portfolio, double S0, double r) {
    std::cout << "\n=== SCENARIO ANALYSIS ===\n";
    
    // Define scenarios: [spot_change%, vol_change%, time_decay_days]
    std::vector<std::tuple<double, double, double, std::string>> scenarios = {
        {0.0, 0.0, 0.0, "Base Case"},
        {5.0, 0.0, 1.0, "5% Up, 1 Day"},
        {-5.0, 0.0, 1.0, "5% Down, 1 Day"},
        {0.0, 20.0, 1.0, "Vol +20%, 1 Day"},
        {0.0, -20.0, 1.0, "Vol -20%, 1 Day"},
        {2.0, 10.0, 7.0, "Moderate Up + Vol"},
        {-2.0, 10.0, 7.0, "Moderate Down + Vol"},
        {0.0, -50.0, 7.0, "Vol Crush"},
        {10.0, 0.0, 7.0, "Large Move Up"},
        {-10.0, 0.0, 7.0, "Large Move Down"}
    };
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(20) << "Scenario"
              << std::setw(12) << "Spot"
              << std::setw(12) << "Portfolio"
              << std::setw(12) << "P&L"
              << std::setw(10) << "Delta"
              << std::setw(10) << "Gamma"
              << std::setw(10) << "Vega"
              << std::endl;
    std::cout << std::string(76, '-') << std::endl;
    
    PortfolioGreeks base_greeks = calculate_portfolio_greeks(portfolio, S0, r);
    
    for (const auto& scenario : scenarios) {
        double spot_change = std::get<0>(scenario) / 100.0;
        double vol_change = std::get<1>(scenario) / 100.0;
        double time_decay = std::get<2>(scenario) / 365.0;
        const std::string& name = std::get<3>(scenario);
        
        double new_spot = S0 * (1.0 + spot_change);
        
        // Calculate portfolio value under scenario
        PortfolioGreeks scenario_greeks;
        
        for (const auto& pos : portfolio) {
            if (pos.strike == 0) {
                // Handle underlying
                scenario_greeks.value += new_spot * pos.quantity;
                scenario_greeks.delta += pos.quantity;
            } else {
                double new_vol = pos.implied_vol * (1.0 + vol_change);
                double new_time = std::max(0.001, pos.time_to_expiry - time_decay);
                
                double price = black_scholes_price(new_spot, pos.strike, r, new_time, new_vol, pos.type);
                double delta = black_scholes_delta(new_spot, pos.strike, r, new_time, new_vol, pos.type);
                double gamma = black_scholes_gamma(new_spot, pos.strike, r, new_time, new_vol);
                double vega = black_scholes_vega(new_spot, pos.strike, r, new_time, new_vol);
                
                scenario_greeks.value += price * pos.quantity;
                scenario_greeks.delta += delta * pos.quantity;
                scenario_greeks.gamma += gamma * pos.quantity;
                scenario_greeks.vega += vega * pos.quantity;
            }
        }
        
        double pnl = scenario_greeks.value - base_greeks.value;
        
        std::cout << std::setw(20) << name
                  << std::setw(12) << new_spot
                  << std::setw(12) << scenario_greeks.value
                  << std::setw(12) << pnl
                  << std::setw(10) << scenario_greeks.delta
                  << std::setw(10) << scenario_greeks.gamma
                  << std::setw(10) << scenario_greeks.vega
                  << std::endl;
    }
}

/**
 * @brief Calculate Value-at-Risk using Monte Carlo
 */
void calculate_var(const std::vector<Position>& portfolio, double S0, double r, int num_simulations = 100000) {
    std::cout << "\n=== VALUE-AT-RISK ANALYSIS ===\n";
    
    std::cout << "Running " << num_simulations << " Monte Carlo simulations...\n";
    
    RNG rng(42);
    std::vector<double> pnl_distribution;
    pnl_distribution.reserve(num_simulations);
    
    PortfolioGreeks base_greeks = calculate_portfolio_greeks(portfolio, S0, r);
    
    // Simulation parameters
    const double vol_underlying = 0.25;  // Underlying volatility
    const double vol_of_vol = 0.3;       // Volatility of volatility
    const double dt = 1.0 / 365.0;       // 1 day
    
    for (int i = 0; i < num_simulations; ++i) {
        // Generate correlated shocks
        double z1 = rng.gauss();
        double z2 = rng.gauss();
        // Note: z3 available for future correlation modeling
        
        // Stock price shock
        double spot_shock = vol_underlying * std::sqrt(dt) * z1;
        double new_spot = S0 * std::exp(-0.5 * vol_underlying * vol_underlying * dt + spot_shock);
        
        // Volatility shock (mean-reverting)
        double vol_shock = vol_of_vol * std::sqrt(dt) * z2;
        
        // Calculate portfolio value under shocks
        double portfolio_value = 0.0;
        
        for (const auto& pos : portfolio) {
            if (pos.strike == 0) {
                portfolio_value += new_spot * pos.quantity;
            } else {
                double new_vol = std::max(0.05, pos.implied_vol + vol_shock);
                double new_time = std::max(0.001, pos.time_to_expiry - dt);
                
                double price = black_scholes_price(new_spot, pos.strike, r, new_time, new_vol, pos.type);
                portfolio_value += price * pos.quantity;
            }
        }
        
        double pnl = portfolio_value - base_greeks.value;
        pnl_distribution.push_back(pnl);
    }
    
    // Sort for quantile calculation
    std::sort(pnl_distribution.begin(), pnl_distribution.end());
    
    // Calculate VaR and Expected Shortfall
    double var_95 = pnl_distribution[static_cast<int>(0.05 * num_simulations)];
    double var_99 = pnl_distribution[static_cast<int>(0.01 * num_simulations)];
    
    // Expected Shortfall (average of worst 5% and 1%)
    double es_95 = 0.0, es_99 = 0.0;
    int es_95_count = static_cast<int>(0.05 * num_simulations);
    int es_99_count = static_cast<int>(0.01 * num_simulations);
    
    for (int i = 0; i < es_95_count; ++i) {
        es_95 += pnl_distribution[i];
    }
    es_95 /= es_95_count;
    
    for (int i = 0; i < es_99_count; ++i) {
        es_99 += pnl_distribution[i];
    }
    es_99 /= es_99_count;
    
    // Calculate statistics
    double mean_pnl = mean(pnl_distribution);
    double std_pnl = standard_deviation(pnl_distribution);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Portfolio Risk Metrics (1-day horizon):\n";
    std::cout << "  Mean P&L: $" << mean_pnl << "\n";
    std::cout << "  P&L Std Dev: $" << std_pnl << "\n";
    std::cout << "  95% VaR: $" << (-var_95) << " (5% chance of losing more)\n";
    std::cout << "  99% VaR: $" << (-var_99) << " (1% chance of losing more)\n";
    std::cout << "  95% Expected Shortfall: $" << (-es_95) << " (average of worst 5%)\n";
    std::cout << "  99% Expected Shortfall: $" << (-es_99) << " (average of worst 1%)\n";
    
    // Probability of profit
    int profit_count = std::count_if(pnl_distribution.begin(), pnl_distribution.end(),
                                   [](double pnl) { return pnl > 0; });
    double prob_profit = static_cast<double>(profit_count) / num_simulations * 100.0;
    
    std::cout << "  Probability of Profit: " << std::setprecision(1) << prob_profit << "%\n";
}

/**
 * @brief Hedge analysis
 */
void hedge_analysis(const std::vector<Position>& portfolio, double S0, double r) {
    std::cout << "\n=== HEDGE ANALYSIS ===\n";
    
    PortfolioGreeks greeks = calculate_portfolio_greeks(portfolio, S0, r);
    
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Current Portfolio Greeks:\n";
    std::cout << "  Portfolio Value: $" << std::setprecision(2) << greeks.value << "\n";
    std::cout << "  Delta: " << std::setprecision(4) << greeks.delta << "\n";
    std::cout << "  Gamma: " << std::setprecision(6) << greeks.gamma << "\n";
    std::cout << "  Vega: " << std::setprecision(2) << greeks.vega << "\n";
    std::cout << "  Theta: " << std::setprecision(2) << greeks.theta << "\n";
    std::cout << "  Rho: " << std::setprecision(2) << greeks.rho << "\n";
    
    // Delta hedge recommendations
    std::cout << "\nDelta Hedge Recommendations:\n";
    if (std::abs(greeks.delta) > 0.1) {
        double hedge_shares = -greeks.delta;
        double hedge_cost = hedge_shares * S0;
        std::cout << "  Current Delta Exposure: " << greeks.delta << "\n";
        std::cout << "  Recommended Hedge: " << (hedge_shares > 0 ? "Buy" : "Sell") 
                  << " " << std::abs(hedge_shares) << " shares\n";
        std::cout << "  Hedge Cost: $" << std::setprecision(2) << std::abs(hedge_cost) << "\n";
    } else {
        std::cout << "  Portfolio is approximately delta-neutral.\n";
    }
    
    // Gamma exposure
    std::cout << "\nGamma Analysis:\n";
    if (std::abs(greeks.gamma) > 0.001) {
        double gamma_risk = 0.5 * greeks.gamma * S0 * S0 * 0.01 * 0.01;  // 1% move
        std::cout << "  Current Gamma: " << std::setprecision(6) << greeks.gamma << "\n";
        std::cout << "  Gamma P&L for 1% move: $" << std::setprecision(2) << gamma_risk << "\n";
        if (greeks.gamma > 0) {
            std::cout << "  Position benefits from volatility (long gamma)\n";
        } else {
            std::cout << "  Position suffers from volatility (short gamma)\n";
        }
    }
    
    // Vega exposure
    std::cout << "\nVega Analysis:\n";
    if (std::abs(greeks.vega) > 1.0) {
        std::cout << "  Current Vega: " << std::setprecision(2) << greeks.vega << "\n";
        std::cout << "  P&L for 1% vol increase: $" << std::setprecision(2) << greeks.vega << "\n";
        if (greeks.vega > 0) {
            std::cout << "  Position benefits from rising volatility\n";
        } else {
            std::cout << "  Position suffers from rising volatility\n";
        }
    }
    
    // Theta analysis
    std::cout << "\nTime Decay Analysis:\n";
    std::cout << "  Daily Theta: $" << std::setprecision(2) << greeks.theta << "\n";
    if (greeks.theta < 0) {
        std::cout << "  Portfolio loses $" << std::setprecision(2) << (-greeks.theta) << " per day from time decay\n";
    } else {
        std::cout << "  Portfolio gains $" << std::setprecision(2) << greeks.theta << " per day from time decay\n";
    }
}

/**
 * @brief Main risk management analysis
 */
int main() {
    std::cout << "Black-Scholes-Merton Risk Management Analysis\n";
    std::cout << "============================================\n";
    
    // Market parameters
    const double S0 = 24300.0;
    const double r = 0.05;
    
    std::cout << "Market Parameters:\n";
    std::cout << "  Underlying Price: $" << S0 << "\n";
    std::cout << "  Risk-free Rate: " << (r * 100.0) << "%\n";
    
    // Create sample portfolio
    auto portfolio = create_sample_portfolio();
    
    std::cout << "\nSample Portfolio:\n";
    for (size_t i = 0; i < portfolio.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << portfolio[i].description() << "\n";
    }
    
    // Perform risk analysis
    hedge_analysis(portfolio, S0, r);
    scenario_analysis(portfolio, S0, r);
    calculate_var(portfolio, S0, r, 50000);  // Reduced for faster execution
    
    std::cout << "\nRisk management analysis complete!\n";
    std::cout << "\nNote: This analysis uses simplified assumptions.\n";
    std::cout << "In practice, consider:\n";
    std::cout << "  - More sophisticated volatility models\n";
    std::cout << "  - Correlation between assets\n";
    std::cout << "  - Liquidity constraints\n";
    std::cout << "  - Transaction costs\n";
    std::cout << "  - Model risk\n";
    
    return 0;
}
