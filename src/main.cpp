/**
 * @file main.cpp
 * @brief Black-Scholes-Merton Pricing Toolkit Demo Application
 * 
 * This application demonstrates the capabilities of the BSM pricing toolkit
 * by comparing different pricing methods on a standard European option.
 * 
 * Features demonstrated:
 * - Analytical Black-Scholes pricing with Greeks
 * - Monte Carlo simulation with variance reduction
 * - PDE finite difference methods
 * - Stochastic Local Volatility (SLV) models
 * - Performance timing and statistical analysis
 * 
 * @author Black-Scholes-Merton Pricing Toolkit
 * @version 1.0
 */

#include <iostream>
#include <vector>
#include <iomanip>
#include <random>
#include <chrono>
#include <sstream>

#include "option_types.hpp"
#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"
#include "pde_cn.hpp"
#include "slv.hpp"
#include "stats.hpp"
#include "iv_solve.hpp"

#ifdef BENCHMARK_MODE
#include <ctime>
#endif

namespace {
    using namespace bsm;

    /**
     * @brief Configuration structure for demo parameters
     */
    struct DemoConfig {
        // Basic option parameters
        double S0 = 100.0;           ///< Current spot price
        double K = 100.0;            ///< Strike price
        double r = 0.05;             ///< Risk-free rate
        double T = 1.0;              ///< Time to expiration (years)
        double sigma = 0.20;         ///< Volatility
        OptionType type = OptionType::Call;  ///< Option type

        // Simulation parameters
        long mc_paths = 500000;      ///< MC simulation paths
        int pde_S_steps = 300;       ///< PDE spatial steps
        int pde_T_steps = 150;       ///< PDE time steps
        long slv_paths = 300000;     ///< SLV simulation paths
        long slv_steps = 252;        ///< SLV time steps (daily)
        int num_slv_seeds = 5;       ///< Number of SLV seed runs

        // Model configuration flags
        bool use_smile_local_vol = true;    ///< Use Smile vs CEV local vol
        bool use_gbm_qmc = true;            ///< Use QMC in GBM MC
        bool use_gbm_cv = true;             ///< Use control variates in GBM
        bool use_andersen_qe = true;        ///< Use Andersen QE in SLV
        bool compute_greeks = true;         ///< Compute Greeks in MC
        bool show_timing = true;            ///< Show timing information
        bool verbose_output = true;         ///< Detailed output
    };

    /**
     * @brief High-resolution timer for performance measurement
     */
    class Timer {
    private:
        std::chrono::high_resolution_clock::time_point start_time;

    public:
        void start() {
            start_time = std::chrono::high_resolution_clock::now();
        }

        double elapsed_ms() const {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            return duration.count() / 1000.0;  // Convert to milliseconds
        }
    };

    /**
     * @brief Format number with thousands separators
     */
    std::string format_number(long number) {
        std::string str = std::to_string(number);
        std::string result;
        int count = 0;
        for (int i = str.length() - 1; i >= 0; --i) {
            if (count && count % 3 == 0) result = "," + result;
            result = str[i] + result;
            ++count;
        }
        return result;
    }

    /**
     * @brief Print formatted section header
     */
    void print_header(const std::string& title) {
        const int width = 70;
        std::cout << std::string(width, '=') << "\n";
        std::cout << std::setw((width + title.length()) / 2) << title << "\n";
        std::cout << std::string(width, '=') << "\n";
    }

    /**
     * @brief Print parameter summary
     */
    void print_parameters(const DemoConfig& config) {
        std::cout << "Input Parameters:\n";
        std::cout << "  Spot Price (S0):      " << config.S0 << "\n";
        std::cout << "  Strike Price (K):     " << config.K << "\n";
        std::cout << "  Risk-free Rate (r):   " << config.r * 100 << "%\n";
        std::cout << "  Time to Expiry (T):   " << config.T << " years\n";
        std::cout << "  Volatility (sigma):       " << config.sigma * 100 << "%\n";
        std::cout << "  Option Type:          " << (config.type == OptionType::Call ? "Call" : "Put") << "\n";
        std::cout << "  Moneyness (S0/K):     " << config.S0 / config.K << "\n";
        std::cout << std::string(70, '-') << "\n";
    }

    /**
     * @brief Print Monte Carlo configuration
     */
    void print_mc_config(const DemoConfig& config) {
        std::cout << "Monte Carlo Configuration:\n";
        std::cout << "  GBM Paths:            " << format_number(config.mc_paths) << "\n";
        std::cout << "  SLV Paths:            " << format_number(config.slv_paths) << "\n";
        std::cout << "  SLV Time Steps:       " << config.slv_steps << " (daily)\n";
        std::cout << "  Quasi-Monte Carlo:    " << (config.use_gbm_qmc ? "Enabled" : "Disabled") << "\n";
        std::cout << "  Control Variates:     " << (config.use_gbm_cv ? "Enabled" : "Disabled") << "\n";
        std::cout << "  Greeks Computation:   " << (config.compute_greeks ? "Enabled" : "Disabled") << "\n";
        std::cout << std::string(70, '-') << "\n";
    }

    /**
     * @brief Run analytical pricing benchmark
     */
    void run_analytical_benchmark(const DemoConfig& config) {
        Timer timer;
        
        print_header("Analytical Black-Scholes Pricing");
        
        timer.start();
        const double price = black_scholes_price(config.S0, config.K, config.r, config.T, config.sigma, config.type);
        const double elapsed = timer.elapsed_ms();
        
        // Compute all Greeks
        const double delta = black_scholes_delta(config.S0, config.K, config.r, config.T, config.sigma, config.type);
        const double gamma = black_scholes_gamma(config.S0, config.K, config.r, config.T, config.sigma);
        const double vega = black_scholes_vega(config.S0, config.K, config.r, config.T, config.sigma);
        const double theta = black_scholes_theta(config.S0, config.K, config.r, config.T, config.sigma, config.type);
        const double rho = black_scholes_rho(config.S0, config.K, config.r, config.T, config.sigma, config.type);
        
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Price:                    " << price << "\n";
        std::cout << "Delta (dP/dS):           " << std::setprecision(4) << delta << "\n";
        std::cout << "Gamma (d2P/dS2):         " << std::setprecision(4) << gamma << "\n";
        std::cout << "Vega (dP/dsigma):            " << std::setprecision(4) << vega << "\n";
        std::cout << "Theta (dP/dT):           " << std::setprecision(4) << theta << "\n";
        std::cout << "Rho (dP/dr):             " << std::setprecision(4) << rho << "\n";
        
        if (config.show_timing) {
            std::cout << "Computation Time:         " << std::setprecision(3) << elapsed << " ms\n";
        }
        std::cout << std::string(70, '-') << "\n";
    }

    /**
     * @brief Run Monte Carlo pricing benchmark
     */
    void run_monte_carlo_benchmark(const DemoConfig& config) {
        Timer timer;
        
        print_header("Monte Carlo Pricing (GBM)");
        
        timer.start();
        const MCResult mc_result = mc_gbm_price(
            config.S0, config.K, config.r, config.T, config.sigma,
            config.mc_paths, config.type, 12345UL,
            true,  // antithetic
            config.use_gbm_cv,   // control_variate
            config.use_gbm_qmc,  // use_qmc
            true,  // two_pass_cv
            config.compute_greeks  // compute_greeks
        );
        const double elapsed = timer.elapsed_ms();
        
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Price:                    " << mc_result.price << "\n";
        std::cout << "Standard Error:           " << mc_result.std_error << "\n";
        std::cout << "Confidence Interval:      [" << mc_result.price - 1.96 * mc_result.std_error 
                  << ", " << mc_result.price + 1.96 * mc_result.std_error << "]\n";
        
        if (config.compute_greeks) {
            std::cout << "Delta:                    " << std::setprecision(4) << mc_result.delta 
                      << " (SE: " << mc_result.delta_se << ")\n";
            std::cout << "Vega:                     " << std::setprecision(4) << mc_result.vega 
                      << " (SE: " << mc_result.vega_se << ")\n";
        }
        
        std::cout << "Number of Paths:          " << format_number(config.mc_paths) << "\n";
        
        if (config.show_timing) {
            std::cout << "Computation Time:         " << std::setprecision(1) << elapsed << " ms\n";
            std::cout << "Paths per Second:         " << std::setprecision(0) 
                      << (config.mc_paths / elapsed * 1000.0) << "\n";
        }
        std::cout << std::string(70, '-') << "\n";
    }

    /**
     * @brief Run PDE pricing benchmark
     */
    void run_pde_benchmark(const DemoConfig& config) {
        Timer timer;
        
        print_header("PDE Finite Difference Pricing");
        
        timer.start();
        const double pde_price = pde_crank_nicolson(
            config.S0, config.K, config.r, config.T, config.sigma,
            config.pde_S_steps, config.pde_T_steps, config.type
        );
        const double elapsed = timer.elapsed_ms();
        
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Price:                    " << pde_price << "\n";
        std::cout << "Grid Size:                " << config.pde_S_steps << " x " << config.pde_T_steps 
                  << " (S x T)\n";
        std::cout << "Total Grid Points:        " << format_number(config.pde_S_steps * config.pde_T_steps) << "\n";
        
        if (config.show_timing) {
            std::cout << "Computation Time:         " << std::setprecision(1) << elapsed << " ms\n";
            std::cout << "Grid Points per Second:   " << std::setprecision(0) 
                      << (config.pde_S_steps * config.pde_T_steps / elapsed * 1000.0) << "\n";
        }
        std::cout << std::string(70, '-') << "\n";
    }

    /**
     * @brief Run SLV pricing benchmark
     */
    void run_slv_benchmark(const DemoConfig& config) {
        Timer timer;
        
        print_header("Stochastic Local Volatility Pricing");
        
        // Setup SLV model
        HestonParams heston{0.04, 2.0, 0.04, 0.3, -0.7};  // Default parameters
        
        auto local_vol_fn = config.use_smile_local_vol ? 
            SmileLocalVol{0.22, 0.95, 0.25, 0.15, config.S0, 0.01}.to_fn() :
            CEVLocalVol{0.25, 0.9, config.S0}.to_fn();
        
        // Single run
        timer.start();
        const MCResult slv_result = mc_slv_price(
            config.S0, config.K, config.r, config.T,
            config.slv_paths, config.slv_steps, config.type,
            heston, local_vol_fn, 77777UL,
            true,  // antithetic
            config.use_andersen_qe  // use_andersen_qe
        );
        const double single_elapsed = timer.elapsed_ms();
        
        // Multiple seed runs for stability analysis
        std::vector<unsigned long> seeds;
        std::mt19937_64 seeder(424242ULL);
        for (int i = 0; i < config.num_slv_seeds; ++i) {
            seeds.push_back(seeder());
        }
        
        timer.start();
        const auto slv_multi_runs = mc_slv_multi_seeds(
            config.S0, config.K, config.r, config.T,
            config.slv_paths / 2, config.slv_steps, config.type,
            heston, local_vol_fn, seeds,
            true,  // antithetic
            config.use_andersen_qe  // use_andersen_qe
        );
        const double multi_elapsed = timer.elapsed_ms();
        
        // Results
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Single Run Price:         " << slv_result.price << "\n";
        std::cout << "Single Run Std Error:     " << slv_result.std_error << "\n";
        
        // Multi-run statistics
        std::vector<double> prices;
        for (const auto& result : slv_multi_runs) {
            prices.push_back(result.price);
        }
        
        const double mean_price = mean(prices);
        const double price_std = standard_deviation(prices);
        const double se_of_mean = price_std / std::sqrt(prices.size());
        
        std::cout << "Multi-run Mean Price:     " << mean_price << "\n";
        std::cout << "Multi-run Std Dev:        " << price_std << "\n";
        std::cout << "Standard Error of Mean:   " << se_of_mean << "\n";
        
        std::cout << "Model Configuration:\n";
        std::cout << "  Heston Parameters:      kappa=" << heston.kappa << ", theta=" << heston.theta 
                  << ", xi=" << heston.xi << ", rho=" << heston.rho << "\n";
        std::cout << "  Local Vol Model:        " << (config.use_smile_local_vol ? "Smile" : "CEV") << "\n";
        std::cout << "  Variance Scheme:        " << (config.use_andersen_qe ? "Andersen QE" : "Euler") << "\n";
        std::cout << "  Number of Paths:        " << format_number(config.slv_paths) << "\n";
        std::cout << "  Time Steps:             " << config.slv_steps << "\n";
        
        if (config.show_timing) {
            std::cout << "Single Run Time:          " << std::setprecision(1) << single_elapsed << " ms\n";
            std::cout << "Multi-run Time:           " << std::setprecision(1) << multi_elapsed << " ms\n";
            std::cout << "Paths per Second:         " << std::setprecision(0) 
                      << (config.slv_paths / single_elapsed * 1000.0) << "\n";
        }
        
        if (config.verbose_output) {
            std::cout << "\nIndividual Seed Results:\n";
            for (size_t i = 0; i < slv_multi_runs.size(); ++i) {
                std::cout << "  Seed[" << i << "]: " << slv_multi_runs[i].price 
                          << " (SE: " << slv_multi_runs[i].std_error << ")\n";
            }
        }
        std::cout << std::string(70, '-') << "\n";
    }

    /**
     * @brief Compare all methods and show analysis
     */
    void run_comparative_analysis(const DemoConfig& config) {
        print_header("Comparative Analysis");
        
        // Re-run all methods for comparison (could cache results in practice)
        const double analytical = black_scholes_price(config.S0, config.K, config.r, config.T, config.sigma, config.type);
        
        const MCResult mc_result = mc_gbm_price(
            config.S0, config.K, config.r, config.T, config.sigma,
            config.mc_paths, config.type, 12345UL, true, true, true, true, false
        );
        
        const double pde_price = pde_crank_nicolson(
            config.S0, config.K, config.r, config.T, config.sigma,
            config.pde_S_steps, config.pde_T_steps, config.type
        );
        
        // Analysis vs analytical benchmark
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Method Comparison (vs Analytical):\n";
        std::cout << "  Analytical BS:          " << analytical << " (baseline)\n";
        std::cout << "  Monte Carlo:            " << mc_result.price 
                  << " (error: " << std::abs(mc_result.price - analytical) << ")\n";
        std::cout << "  PDE Crank-Nicolson:     " << pde_price 
                  << " (error: " << std::abs(pde_price - analytical) << ")\n";
        
        // Relative errors
        const double mc_rel_error = std::abs(mc_result.price - analytical) / analytical * 100.0;
        const double pde_rel_error = std::abs(pde_price - analytical) / analytical * 100.0;
        
        std::cout << "\nRelative Errors:\n";
        std::cout << "  Monte Carlo:            " << std::setprecision(4) << mc_rel_error << "%\n";
        std::cout << "  PDE Method:             " << std::setprecision(4) << pde_rel_error << "%\n";
        
        // Implied volatility analysis
        auto bs_price_fn = [&](double vol) { 
            return black_scholes_price(config.S0, config.K, config.r, config.T, vol, config.type); 
        };
        
        const double iv_mc = implied_vol(mc_result.price, bs_price_fn);
        const double iv_pde = implied_vol(pde_price, bs_price_fn);
        
        std::cout << "\nImplied Volatilities:\n";
        std::cout << "  Input Volatility:       " << std::setprecision(4) << config.sigma * 100 << "%\n";
        std::cout << "  MC Implied Vol:         " << std::setprecision(4) << iv_mc * 100 << "%\n";
        std::cout << "  PDE Implied Vol:        " << std::setprecision(4) << iv_pde * 100 << "%\n";
        
        std::cout << std::string(70, '=') << "\n";
    }

} // anonymous namespace

int main() {
    try {
        DemoConfig config;
        
        // Print application header
        print_header("Black-Scholes-Merton Pricing Toolkit Demo");
        
        std::cout << "Version: 1.0\n";
        std::cout << "Build: " << __DATE__ << " " << __TIME__ << "\n";
        
#ifdef USE_OPENMP
        std::cout << "OpenMP: Enabled\n";
#else
        std::cout << "OpenMP: Disabled\n";
#endif

#ifdef BENCHMARK_MODE
        std::cout << "Benchmark Mode: Enabled\n";
        config.show_timing = true;
#endif
        
        std::cout << "\n";
        
        // Show configuration
        print_parameters(config);
        print_mc_config(config);
        
        // Run pricing methods
        run_analytical_benchmark(config);
        run_monte_carlo_benchmark(config);
        run_pde_benchmark(config);
        run_slv_benchmark(config);
        
        // Comparative analysis
        run_comparative_analysis(config);
        
        std::cout << "Demo completed successfully!\n";
        std::cout << "For more information, see: docs/api_reference.md\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}

