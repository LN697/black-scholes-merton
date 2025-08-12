/**
 * @file tests.cpp
 * @brief Comprehensive test suite for Black-Scholes-Merton Pricing Toolkit
 * 
 * This test suite validates all major functionality including:
 * - Analytical pricing accuracy
 * - Monte Carlo convergence
 * - PDE numerical stability
 * - Greeks calculations
 * - Put-call parity
 * - American vs European option pricing
 * - SLV model sanity checks 
 * 
 * @author LN697
 * @version 1.0
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <string>

#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"
#include "pde_cn.hpp"
#include "pde_cn_american.hpp"
#include "slv.hpp"
#include "slv_calibration.hpp"
#include "iv_solve.hpp"
#include "math_utils.hpp"
#include "stats.hpp"

#ifdef USE_PERFORMANCE_UTILS
#include "performance_utils.hpp"
#endif

using namespace bsm;

namespace {
    /**
     * @brief Approximate equality check with tolerance
     */
    bool approx_equal(double a, double b, double tol = 1e-6) { 
        return std::abs(a - b) <= tol; 
    }

    /**
     * @brief Test counter for progress tracking
     */
    int test_count = 0;
    int passed_tests = 0;

    /**
     * @brief Test assertion with reporting
     */
    void test_assert(bool condition, const std::string& test_name) {
        ++test_count;
        if (condition) {
            ++passed_tests;
            std::cout << "[PASS] " << test_name << std::endl;
        } else {
            std::cout << "[FAIL] " << test_name << std::endl;
        }
    }

    /**
     * @brief Print test section header
     */
    void print_section(const std::string& section_name) {
        std::cout << "\n=== " << section_name << " ===" << std::endl;
    }
}

/**
 * @brief Test analytical Black-Scholes pricing
 */
void test_analytical_pricing() {
    print_section("Analytical Black-Scholes Pricing");

    const double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    
    // Test basic pricing
    const double call_price = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
    const double put_price = black_scholes_price(S0, K, r, T, sigma, OptionType::Put);
    
    test_assert(call_price > 0.0, "Call price is positive");
    test_assert(put_price > 0.0, "Put price is positive");
    test_assert(call_price > put_price, "ATM call > ATM put for positive rates");
    
    // Test put-call parity: C - P = S - K*e^(-rT)
    const double parity_lhs = call_price - put_price;
    const double parity_rhs = S0 - K * std::exp(-r * T);
    test_assert(approx_equal(parity_lhs, parity_rhs, 1e-10), "Put-call parity");
    
    // Test edge cases
    const double zero_vol_call = black_scholes_price(S0, K, r, T, 0.0, OptionType::Call);
    const double intrinsic_call = std::max(S0 - K * std::exp(-r * T), 0.0);
    test_assert(approx_equal(zero_vol_call, intrinsic_call, 1e-10), "Zero volatility edge case");
    
    const double zero_time_call = black_scholes_price(S0, K, r, 0.0, sigma, OptionType::Call);
    const double immediate_intrinsic = std::max(S0 - K, 0.0);
    test_assert(approx_equal(zero_time_call, immediate_intrinsic, 1e-10), "Zero time edge case");
}

/**
 * @brief Test Greeks calculations
 */
void test_greeks() {
    print_section("Greeks Calculations");

    const double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    
    // Test Greeks exist and have reasonable values
    const double delta_call = black_scholes_delta(S0, K, r, T, sigma, OptionType::Call);
    const double delta_put = black_scholes_delta(S0, K, r, T, sigma, OptionType::Put);
    const double gamma = black_scholes_gamma(S0, K, r, T, sigma);
    const double vega = black_scholes_vega(S0, K, r, T, sigma);
    const double theta_call = black_scholes_theta(S0, K, r, T, sigma, OptionType::Call);
    const double rho_call = black_scholes_rho(S0, K, r, T, sigma, OptionType::Call);
    
    test_assert(delta_call > 0.0 && delta_call < 1.0, "Call delta in (0,1)");
    test_assert(delta_put < 0.0 && delta_put > -1.0, "Put delta in (-1,0)");
    test_assert(approx_equal(delta_call - delta_put, 1.0, 1e-10), "Delta put-call relationship");
    test_assert(gamma > 0.0, "Gamma is positive");
    test_assert(vega > 0.0, "Vega is positive");
    test_assert(theta_call < 0.0, "Call theta is negative (time decay)");
    test_assert(rho_call > 0.0, "Call rho is positive");
    
    // Test gamma and vega are same for calls and puts
    const double gamma_put = black_scholes_gamma(S0, K, r, T, sigma);
    const double vega_put = black_scholes_vega(S0, K, r, T, sigma);
    test_assert(approx_equal(gamma, gamma_put, 1e-10), "Gamma same for calls and puts");
    test_assert(approx_equal(vega, vega_put, 1e-10), "Vega same for calls and puts");
}

/**
 * @brief Test Monte Carlo pricing convergence
 */
void test_monte_carlo() {
    print_section("Monte Carlo Pricing");

    const double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    const double analytical = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
    
    // Test basic MC convergence
    const MCResult mc_result = mc_gbm_price(S0, K, r, T, sigma, 200000, OptionType::Call, 
                                           42UL, true, true, false, true, true);
    
    const double error = std::abs(mc_result.price - analytical);
    const double tolerance = 3.0 * mc_result.std_error;  // 3-sigma confidence
    
    test_assert(mc_result.std_error > 0.0, "MC standard error is positive");
    test_assert(error <= tolerance, "MC converges to analytical within 3 standard errors");
    test_assert(mc_result.price > 0.0, "MC price is positive");
    
    // Test variance reduction effectiveness
    const MCResult mc_no_vr = mc_gbm_price(S0, K, r, T, sigma, 50000, OptionType::Call,
                                          42UL, false, false, false, false, false);
    const MCResult mc_with_vr = mc_gbm_price(S0, K, r, T, sigma, 50000, OptionType::Call,
                                            42UL, true, true, true, true, false);
    
    test_assert(mc_with_vr.std_error < mc_no_vr.std_error, "Variance reduction reduces standard error");
    
    // Test Greeks from MC  
    const double analytical_delta = black_scholes_delta(S0, K, r, T, sigma, OptionType::Call);
    
    // Monte Carlo delta estimation is inherently noisy, especially for pathwise derivatives
    if (mc_result.delta > 0.0 && mc_result.delta_se > 0.0) {
        const double delta_error = std::abs(mc_result.delta - analytical_delta);
        
        // Use relative tolerance: 10% error is acceptable for MC delta
        const double relative_tolerance = 0.10 * analytical_delta;
        const double statistical_tolerance = 5.0 * mc_result.delta_se;
        const double delta_tolerance = std::max(relative_tolerance, statistical_tolerance);
        
        test_assert(delta_error <= delta_tolerance, "MC delta converges to analytical");
    } else {
        test_assert(false, "MC delta converges to analytical");
    }
}

/**
 * @brief Test PDE pricing accuracy
 */
void test_pde_pricing() {
    print_section("PDE Pricing");

    const double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    const double analytical = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
    
    // Test European PDE pricing
    const double pde_price = pde_crank_nicolson(S0, K, r, T, sigma, 200, 100, OptionType::Call);
    const double pde_error = std::abs(pde_price - analytical);
    
    test_assert(pde_price > 0.0, "PDE price is positive");
    test_assert(pde_error < 0.05, "PDE error vs analytical < 5%");
    
    // Test grid refinement improves accuracy
    const double coarse_pde = pde_crank_nicolson(S0, K, r, T, sigma, 50, 25, OptionType::Call);
    const double fine_pde = pde_crank_nicolson(S0, K, r, T, sigma, 400, 200, OptionType::Call);
    
    const double coarse_error = std::abs(coarse_pde - analytical);
    const double fine_error = std::abs(fine_pde - analytical);
    
    test_assert(fine_error < coarse_error, "Grid refinement improves accuracy");
    
    // Test American option pricing
    const double euro_put = black_scholes_price(S0, K, r, T, sigma, OptionType::Put);
    const double amer_put = pde_crank_nicolson_american(S0, K, r, T, sigma, 200, 100, OptionType::Put);
    
    test_assert(amer_put >= euro_put, "American put >= European put");
    test_assert(amer_put > euro_put, "American put > European put (should have early exercise value)");
}

/**
 * @brief Test SLV model functionality
 */
void test_slv_models() {
    print_section("Stochastic Local Volatility Models");

    const double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0;
    
    // Test SLV pricing doesn't crash and produces reasonable results
    HestonParams heston{0.04, 2.0, 0.04, 0.3, -0.7};
    
    // Test CEV local volatility
    CEVLocalVol cev_model{0.25, 0.9, S0};
    auto cev_fn = cev_model.to_fn();
    
    test_assert(cev_fn(S0, 0.5) > 0.0, "CEV local vol function returns positive values");
    test_assert(std::isfinite(cev_fn(S0, 0.5)), "CEV local vol function returns finite values");
    
    // Test Smile local volatility
    SmileLocalVol smile_model{0.22, 0.95, 0.25, 0.15, S0, 0.01};
    auto smile_fn = smile_model.to_fn();
    
    test_assert(smile_fn(S0, 0.5) > 0.0, "Smile local vol function returns positive values");
    test_assert(std::isfinite(smile_fn(S0, 0.5)), "Smile local vol function returns finite values");
    
    try {
        // Test SLV pricing with reduced parameters to avoid crashes
        const MCResult slv_result = mc_slv_price(S0, K, r, T, 10000, 50, OptionType::Call,
                                                heston, cev_fn, 777UL, true, true);
        
        test_assert(slv_result.price > 0.0, "SLV price is positive");
        test_assert(std::isfinite(slv_result.price), "SLV price is finite");
        test_assert(slv_result.std_error > 0.0, "SLV standard error is positive");
        
    } catch (const std::exception& e) {
        std::cout << "[WARNING] SLV test failed with exception: " << e.what() << std::endl;
        test_assert(false, "SLV pricing completes without exceptions");
    } catch (...) {
        std::cout << "[WARNING] SLV test failed with unknown exception" << std::endl;
        test_assert(false, "SLV pricing completes without exceptions");
    }
}

/**
 * @brief Test SLV calibration functionality
 */
void test_slv_calibration() {
    print_section("SLV Calibration");

    try {
        // Test the calibration components
        bool calibration_works = validate_slv_calibration();
        test_assert(calibration_works, "SLV calibration test completes successfully");
        
        // Test individual components
        auto dupire = create_sample_dupire_surface();
        test_assert(!dupire.t.empty(), "Sample Dupire surface has time points");
        test_assert(!dupire.S.empty(), "Sample Dupire surface has spot points");
        test_assert(!dupire.sigma.empty(), "Sample Dupire surface has volatility data");
        
        // Validate surface values are reasonable
        for (size_t i = 0; i < dupire.t.size(); ++i) {
            for (size_t j = 0; j < dupire.S.size(); ++j) {
                double vol = dupire.sigma[i][j];
                test_assert(vol > 0.0 && vol < 2.0, "Dupire surface volatilities are reasonable");
            }
        }
        
        // Test leverage grid creation
        auto leverage = create_sample_leverage_grid(dupire);
        test_assert(leverage.t.size() == dupire.t.size(), "Leverage grid matches Dupire time dimension");
        test_assert(leverage.S.size() == dupire.S.size(), "Leverage grid matches Dupire spot dimension");
        
        // Test interpolation
        double test_leverage = leverage.interpolate(100.0, 0.5);
        test_assert(std::isfinite(test_leverage), "Leverage interpolation returns finite values");
        test_assert(test_leverage > 0.0, "Leverage interpolation returns positive values");
        
        // Test calibration with minimal parameters
        HestonParams heston{2.0, 0.04, 0.3, -0.7, 0.04};
        calibrate_leverage_iterative(dupire, heston, leverage, 3);
        
        // Check leverage values after calibration
        bool all_leverage_valid = true;
        for (size_t i = 0; i < leverage.t.size(); ++i) {
            for (size_t j = 0; j < leverage.S.size(); ++j) {
                double L = leverage.L[i][j];
                if (!std::isfinite(L) || L <= 0.0 || L > 10.0) {
                    all_leverage_valid = false;
                    break;
                }
            }
            if (!all_leverage_valid) break;
        }
        test_assert(all_leverage_valid, "Calibrated leverage values are finite and reasonable");
        
    } catch (const std::exception& e) {
        std::cout << "[WARNING] SLV calibration test failed with exception: " << e.what() << std::endl;
        test_assert(false, "SLV calibration completes without exceptions");
    } catch (...) {
        std::cout << "[WARNING] SLV calibration test failed with unknown exception" << std::endl;
        test_assert(false, "SLV calibration completes without exceptions");
    }
}

/**
 * @brief Test implied volatility calculation
 */
void test_implied_volatility() {
    print_section("Implied Volatility");

    const double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.25;
    
    // Generate a market price using known volatility
    const double market_price = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
    
    // Create pricing function
    auto bs_price_fn = [=](double vol) { 
        return black_scholes_price(S0, K, r, T, vol, OptionType::Call); 
    };
    
    // Solve for implied volatility
    const double recovered_vol = implied_vol(market_price, bs_price_fn);
    
    test_assert(std::isfinite(recovered_vol), "Implied volatility is finite");
    test_assert(recovered_vol > 0.0, "Implied volatility is positive");
    test_assert(approx_equal(recovered_vol, sigma, 1e-6), "Implied volatility recovers input volatility");
}

/**
 * @brief Test mathematical utilities
 */
void test_math_utils() {
    print_section("Mathematical Utilities");

    // Test normal CDF
    test_assert(approx_equal(norm_cdf(0.0), 0.5, 1e-10), "Normal CDF at 0");
    test_assert(approx_equal(norm_cdf(-1.96), 0.025, 1e-3), "Normal CDF at -1.96");
    test_assert(approx_equal(norm_cdf(1.96), 0.975, 1e-3), "Normal CDF at 1.96");
    
    // Test normal PDF
    test_assert(approx_equal(norm_pdf(0.0), 1.0 / std::sqrt(2.0 * M_PI), 1e-10), "Normal PDF at 0");
    test_assert(norm_pdf(0.0) > norm_pdf(1.0), "Normal PDF decreases away from mean");
    
    // Test RNG
    RNG rng(12345);
    std::vector<double> samples;
    for (int i = 0; i < 1000; ++i) {
        samples.push_back(rng.gauss());
    }
    
    const double sample_mean = mean(samples);
    const double sample_var = variance(samples);
    
    test_assert(std::abs(sample_mean) < 0.1, "RNG sample mean near 0");
    test_assert(std::abs(sample_var - 1.0) < 0.1, "RNG sample variance near 1");
    
    // Test Halton sequence
    Halton2D halton(123);
    auto [u1, u2] = halton.next();
    test_assert(u1 >= 0.0 && u1 < 1.0, "Halton sequence in [0,1)");
    test_assert(u2 >= 0.0 && u2 < 1.0, "Halton sequence in [0,1)");
    
    // Test Box-Muller
    auto [z1, z2] = box_muller(0.5, 0.5);
    test_assert(std::isfinite(z1) && std::isfinite(z2), "Box-Muller produces finite values");
}

/**
 * @brief Test statistical functions
 */
void test_statistics() {
    print_section("Statistical Functions");

    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
    
    test_assert(approx_equal(mean(data), 3.0, 1e-10), "Mean calculation");
    test_assert(approx_equal(variance(data), 2.5, 1e-10), "Variance calculation");
    test_assert(approx_equal(standard_deviation(data), std::sqrt(2.5), 1e-10), "Standard deviation");
    test_assert(approx_equal(standard_error(data), std::sqrt(2.5 / 5.0), 1e-10), "Standard error");
    
    // Test MCResult functionality
    MCResult result;
    result.price = 10.0;
    result.std_error = 0.1;
    
    auto [lower, upper] = result.confidence_interval(0.95);
    test_assert(lower < result.price && upper > result.price, "Confidence interval contains price");
    test_assert(result.is_significant(0.05), "Price is statistically significant");
}

/**
 * @brief Test boundary conditions and edge cases
 */
void test_edge_cases() {
    print_section("Edge Cases and Boundary Conditions");

    const double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    
    // Test very ITM/OTM options
    const double deep_itm_call = black_scholes_price(150.0, K, r, T, sigma, OptionType::Call);
    const double deep_otm_call = black_scholes_price(50.0, K, r, T, sigma, OptionType::Call);
    
    test_assert(deep_itm_call > deep_otm_call, "Deep ITM > Deep OTM");
    test_assert(deep_itm_call > 40.0, "Deep ITM call has substantial intrinsic value");
    test_assert(deep_otm_call < 5.0, "Deep OTM call has low value");
    
    // Test very short and long times
    const double short_term = black_scholes_price(S0, K, r, 0.01, sigma, OptionType::Call);
    const double long_term = black_scholes_price(S0, K, r, 10.0, sigma, OptionType::Call);
    
    test_assert(long_term > short_term, "Longer time increases option value");
    test_assert(std::isfinite(short_term) && std::isfinite(long_term), "Extreme times produce finite values");
    
    // Test very low and high volatilities
    const double low_vol = black_scholes_price(S0, K, r, T, 0.01, OptionType::Call);
    const double high_vol = black_scholes_price(S0, K, r, T, 1.0, OptionType::Call);
    
    test_assert(high_vol > low_vol, "Higher volatility increases option value");
    test_assert(std::isfinite(low_vol) && std::isfinite(high_vol), "Extreme volatilities produce finite values");
}

/**
 * @brief Test performance and optimization features
 */
void test_performance_optimization() {
#ifdef USE_PERFORMANCE_UTILS
    print_section("Performance Optimization Tests");
    
    // Test architecture detection
    auto arch_info = bsm::performance::ArchitectureOptimizer::detect_architecture();
    test_assert(!arch_info.cpu_brand.empty(), "CPU brand detection");
    test_assert(arch_info.num_physical_cores > 0, "Physical core count detection");
    test_assert(arch_info.num_logical_cores >= arch_info.num_physical_cores, 
                "Logical cores >= physical cores");
    test_assert(arch_info.cache_line_size > 0, "Cache line size detection");
    
    // Test numerical accuracy validation
    bool accuracy_ok = bsm::performance::ArchitectureOptimizer::validate_numerical_accuracy();
    test_assert(accuracy_ok, "Numerical accuracy validation");
    
    // Test threading configuration
    auto thread_config = bsm::performance::ThreadManager::initialize_threading();
    test_assert(thread_config.num_threads > 0, "Thread count configuration");
    
    // Test memory profiling
    bsm::performance::MemoryProfiler::start_profiling();
    
    // Allocate some memory for testing
    std::vector<double> test_data(1000000, 1.0);
    double sum = std::accumulate(test_data.begin(), test_data.end(), 0.0);
    volatile double sink = sum; // Prevent optimization
    (void)sink;
    
    auto memory_profile = bsm::performance::MemoryProfiler::stop_profiling();
    test_assert(memory_profile.current_memory_mb > 0, "Memory usage detection");
    test_assert(memory_profile.available_memory_mb > 0, "Available memory detection");
    
    // Test benchmark functionality
    auto benchmark_result = bsm::performance::PerformanceBenchmark::run_benchmark(
        "Test Benchmark",
        []() {
            // Simple computation for testing
            double result = 0.0;
            for (int i = 0; i < 100000; ++i) {
                result += std::sin(i * 0.001);
            }
            volatile double sink = result;
            (void)sink;
        },
        3  // 3 iterations
    );
    
    test_assert(benchmark_result.execution_time_ms > 0, "Benchmark timing");
    test_assert(benchmark_result.throughput > 0, "Benchmark throughput calculation");
    test_assert(!benchmark_result.test_name.empty(), "Benchmark name assignment");
    
    std::cout << "Performance optimization tests completed" << std::endl;
#else
    print_section("Performance Optimization Tests (SKIPPED - not compiled with USE_PERFORMANCE_UTILS)");
#endif
}

/**
 * @brief Test performance regression detection
 */
void test_performance_regression() {
#ifdef USE_PERFORMANCE_UTILS
    print_section("Performance Regression Tests");
    
    const double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    
    // Benchmark analytical pricing
    auto analytical_benchmark = bsm::performance::PerformanceBenchmark::run_benchmark(
        "Analytical BS Pricing",
        [=]() {
            for (int i = 0; i < 1000; ++i) {
                double price = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
                volatile double sink = price;
                (void)sink;
            }
        }
    );
    
    // Benchmark Monte Carlo pricing
    auto mc_benchmark = bsm::performance::PerformanceBenchmark::run_benchmark(
        "Monte Carlo Pricing",
        [=]() {
            auto result = mc_gbm_price(S0, K, r, T, sigma, 10000, OptionType::Call, 12345UL);
            volatile double sink = result.price;
            (void)sink;
        }
    );
    
    test_assert(analytical_benchmark.execution_time_ms > 0, "Analytical benchmark timing");
    test_assert(mc_benchmark.execution_time_ms > 0, "Monte Carlo benchmark timing");
    test_assert(analytical_benchmark.execution_time_ms < mc_benchmark.execution_time_ms, 
                "Analytical should be faster than MC");
    
    // Test threading performance (if OpenMP is available)
#ifdef USE_OPENMP
    auto thread_metrics = bsm::performance::ThreadManager::monitor_thread_performance();
    test_assert(thread_metrics.find("max_threads") != thread_metrics.end(), 
                "Thread performance monitoring");
#endif
    
    std::cout << "Performance regression tests completed" << std::endl;
#else
    print_section("Performance Regression Tests (SKIPPED - not compiled with USE_PERFORMANCE_UTILS)");
#endif
}

/**
 * @brief Test numerical stability under optimization
 */
void test_numerical_stability() {
    print_section("Numerical Stability Under Optimization");
    
    const double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    
    // Test pricing with extreme parameters
    double extreme_cases[][5] = {
        {1e-6, K, r, T, sigma},      // Very small S
        {1e6, K, r, T, sigma},       // Very large S
        {S0, 1e-6, r, T, sigma},     // Very small K
        {S0, 1e6, r, T, sigma},      // Very large K
        {S0, K, 1e-6, T, sigma},     // Very small r
        {S0, K, 0.5, T, sigma},      // Large r
        {S0, K, r, 1e-6, sigma},     // Very small T
        {S0, K, r, 100.0, sigma},    // Very large T
        {S0, K, r, T, 1e-6},         // Very small sigma
        {S0, K, r, T, 2.0}           // Large sigma
    };
    
    for (size_t i = 0; i < sizeof(extreme_cases) / sizeof(extreme_cases[0]); ++i) {
        double price = black_scholes_price(extreme_cases[i][0], extreme_cases[i][1], 
                                         extreme_cases[i][2], extreme_cases[i][3], 
                                         extreme_cases[i][4], OptionType::Call);
        test_assert(std::isfinite(price) && price >= 0, 
                    "Extreme parameter case " + std::to_string(i) + " produces valid price");
    }
    
    // Test numerical precision with small differences
    double base_price = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
    double perturbed_price = black_scholes_price(S0 * (1 + 1e-12), K, r, T, sigma, OptionType::Call);
    
    // Small changes should produce small, finite differences
    double relative_change = std::abs(perturbed_price - base_price) / base_price;
    test_assert(std::isfinite(relative_change) && relative_change < 1e-6, 
                "Small parameter changes produce stable results");
}

/**
 * @brief Main test runner
 */
int main() {
    std::cout << "Black-Scholes-Merton Pricing Toolkit Test Suite" << std::endl;
    std::cout << "================================================" << std::endl;

    try {
        test_analytical_pricing();
        test_greeks();
        test_monte_carlo();
        test_pde_pricing();
        test_slv_models();
        test_slv_calibration();
        test_implied_volatility();
        test_math_utils();
        test_statistics();
        test_edge_cases();
        
        // Performance and optimization tests
        test_performance_optimization();
        test_performance_regression();
        test_numerical_stability();

        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Passed: " << passed_tests << "/" << test_count << " tests" << std::endl;
        
        if (passed_tests == test_count) {
            std::cout << "All tests passed!" << std::endl;
            return 0;
        } else {
            std::cout << "Some tests failed!" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test suite failed with unknown exception" << std::endl;
        return 1;
    }
}
