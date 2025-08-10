# User Guide

This guide provides comprehensive instructions for using the Black-Scholes-Merton Pricing Toolkit effectively.

## 🚀 Quick Start

### Installation

#### Option 1: Pre-built Binaries (Not Implemented)
```bash
# Download from releases page
wget https://github.com/ln697/black-scholes-merton/releases/latest/bsm-toolkit.tar.gz
tar -xzf bsm-toolkit.tar.gz
cd bsm-toolkit/
./bsm
```

#### Option 2: Build from Source
```bash
# Clone the repository
git clone https://github.com/ln697/black-scholes-merton.git
cd black-scholes-merton/

# Build with default settings
make

# Build with enhanced CLI interface (recommended)
make enhanced

# Run the demo
./build/release/bin/bsm

# Run tests
make test
```

### Interface Options

The toolkit provides two interface options:

#### Standard Interface
```bash
# Build standard interface
make

# Runs comprehensive demo with all pricing methods
./build/release/bin/bsm
```

#### Enhanced CLI Interface (Recommended)
```bash
# Build enhanced CLI
make enhanced

# Professional command-line interface
./bsm --help                    # Show available commands
./bsm price --help              # Help for specific command
./bsm --interactive             # Start interactive mode

# Example: Price a call option with Greeks
./bsm price --spot 100 --strike 105 --rate 0.05 --time 0.25 --vol 0.2 --type call --greeks
```

For detailed enhanced CLI documentation, see [Enhanced CLI Guide](enhanced_cli_guide.md).

### First Run

After building, run the demo application:

```bash
./build/release/bin/bsm
```

You should see output comparing different pricing methods for a standard European call option.

## 📖 Basic Usage

### Demo Application

The included demo application (`bsm`) showcases all major features:

```bash
# Basic run
./build/release/bin/bsm

# With OpenMP parallelization (if compiled with OMP=1)
OMP_NUM_THREADS=4 ./build/release/bin/bsm

# Benchmark mode
make BENCHMARK=1 && ./build/release/bin/bsm
```

### API Integration

To use the library in your own projects:

```cpp
#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"

using namespace bsm;

int main() {
    // Basic option parameters
    double S0 = 100.0;    // Current price
    double K = 105.0;     // Strike price
    double r = 0.05;      // Risk-free rate (5%)
    double T = 0.25;      // 3 months to expiration
    double sigma = 0.2;   // 20% volatility
    
    // Analytical pricing
    double bs_price = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
    std::cout << "Black-Scholes Price: " << bs_price << std::endl;
    
    // Monte Carlo pricing
    MCResult mc_result = mc_gbm_price(S0, K, r, T, sigma, 
                                      100000, OptionType::Call);
    std::cout << "Monte Carlo Price: " << mc_result.price 
              << " +/- " << mc_result.std_error << std::endl;
    
    return 0;
}
```

## 🔧 Advanced Configuration

### Build Options

The Makefile supports various configuration options:

```bash
# Debug build with symbols
make DEBUG=1

# Release build with OpenMP
make OMP=1

# Profile-guided optimization
make pgo

# Static binary
make STATIC=1

# Coverage analysis
make coverage
```

### Runtime Configuration

#### OpenMP Threading
```bash
# Set number of threads
export OMP_NUM_THREADS=8

# Control thread affinity (Linux)
export OMP_PROC_BIND=close
export OMP_PLACES=cores
```

#### Monte Carlo Parameters

Adjust simulation parameters based on your accuracy requirements:

```cpp
// High accuracy (slow)
MCResult result = mc_gbm_price(S0, K, r, T, sigma, 
                               5000000,  // 5M paths
                               OptionType::Call, 12345UL,
                               true,     // antithetic
                               true,     // control variates
                               true);    // QMC

// Fast approximation
MCResult result = mc_gbm_price(S0, K, r, T, sigma, 
                               50000,    // 50K paths
                               OptionType::Call, 12345UL,
                               false,    // no antithetic
                               false,    // no control variates
                               false);   // no QMC
```

#### PDE Grid Configuration

Balance accuracy vs. speed:

```cpp
// High accuracy
double price = pde_crank_nicolson(S0, K, r, T, sigma, 
                                  500, 250,  // Fine grid
                                  OptionType::Call);

// Fast computation
double price = pde_crank_nicolson(S0, K, r, T, sigma, 
                                  100, 50,   // Coarse grid
                                  OptionType::Call);
```

## 💼 Common Use Cases

### 1. Portfolio Valuation

```cpp
struct Position {
    double S0, K, r, T, sigma;
    OptionType type;
    double quantity;
};

double value_portfolio(const std::vector<Position>& positions) {
    double total_value = 0.0;
    
    for (const auto& pos : positions) {
        double price = black_scholes_price(pos.S0, pos.K, pos.r, 
                                          pos.T, pos.sigma, pos.type);
        total_value += price * pos.quantity;
    }
    
    return total_value;
}
```

### 2. Greeks Calculation

```cpp
void compute_full_greeks(double S0, double K, double r, double T, double sigma) {
    OptionType type = OptionType::Call;
    
    // Analytical Greeks
    double delta = black_scholes_delta(S0, K, r, T, sigma, type);
    double gamma = black_scholes_gamma(S0, K, r, T, sigma);
    double vega = black_scholes_vega(S0, K, r, T, sigma);
    double theta = black_scholes_theta(S0, K, r, T, sigma, type);
    double rho = black_scholes_rho(S0, K, r, T, sigma, type);
    
    std::cout << "Greeks:\n"
              << "  Delta: " << delta << "\n"
              << "  Gamma: " << gamma << "\n"
              << "  Vega:  " << vega << "\n"
              << "  Theta: " << theta << "\n"
              << "  Rho:   " << rho << "\n";
}
```

### 3. Implied Volatility Calculation

```cpp
double calculate_implied_vol(double market_price, double S0, double K, 
                           double r, double T, OptionType type) {
    auto bs_price_fn = [=](double vol) {
        return black_scholes_price(S0, K, r, T, vol, type);
    };
    
    return implied_vol(market_price, bs_price_fn);
}
```

### 4. Monte Carlo with Custom Payoffs

```cpp
// Custom payoff function for digital option
auto digital_payoff = [](double S, double K) -> double {
    return (S > K) ? 1.0 : 0.0;
};

// Manual Monte Carlo simulation
double mc_digital_option(double S0, double K, double r, double T, double sigma, long paths) {
    RNG rng(12345);
    double drift = (r - 0.5 * sigma * sigma) * T;
    double vol_sqrt_T = sigma * std::sqrt(T);
    
    double sum = 0.0;
    for (long i = 0; i < paths; ++i) {
        double Z = rng.gauss();
        double ST = S0 * std::exp(drift + vol_sqrt_T * Z);
        sum += digital_payoff(ST, K);
    }
    
    return std::exp(-r * T) * sum / static_cast<double>(paths);
}
```

### 5. Stochastic Local Volatility Models

```cpp
// Custom local volatility model
class TanhLocalVol {
private:
    double sigma_min, sigma_max, S_pivot, steepness;
    
public:
    TanhLocalVol(double sig_min, double sig_max, double pivot, double steep)
        : sigma_min(sig_min), sigma_max(sig_max), S_pivot(pivot), steepness(steep) {}
    
    LocalVolFn to_fn() const {
        return [=](double S, double t) -> double {
            double moneyness = S / S_pivot;
            double tanh_term = std::tanh(steepness * (moneyness - 1.0));
            return sigma_min + (sigma_max - sigma_min) * (1.0 + tanh_term) / 2.0;
        };
    }
};

// Usage
TanhLocalVol custom_vol{0.15, 0.35, 100.0, 2.0};
HestonParams heston{0.04, 2.0, 0.04, 0.3, -0.7};

MCResult slv_price = mc_slv_price(100.0, 100.0, 0.05, 1.0,
                                  500000, 252, OptionType::Call,
                                  heston, custom_vol.to_fn(), 77777UL);
```

## 🧪 Testing and Validation

### Running Tests

```bash
# Basic test suite
make test

# Comprehensive testing
make test-all

# Performance benchmarks
make benchmark

# Coverage analysis
make coverage && firefox *.gcov.html
```

### Custom Testing

Create your own test cases:

```cpp
#include "analytic_bs.hpp"
#include <cassert>
#include <cmath>

void test_put_call_parity() {
    double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    
    double call = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
    double put = black_scholes_price(S0, K, r, T, sigma, OptionType::Put);
    
    // Put-call parity: C - P = S - K*e^(-rT)
    double parity = call - put - (S0 - K * std::exp(-r * T));
    assert(std::abs(parity) < 1e-10);
    
    std::cout << "Put-call parity test passed!" << std::endl;
}
```

## 🐛 Troubleshooting

### Common Issues

#### Build Errors

**Issue**: `g++: command not found`
```bash
# Ubuntu/Debian
sudo apt install build-essential

# macOS
xcode-select --install

# Windows (MSYS2)
pacman -S mingw-w64-x86_64-gcc
```

**Issue**: OpenMP not found
```bash
# Make sure you have OpenMP support
g++ -fopenmp --version

# On macOS, you might need to install GCC separately
brew install gcc
make CXX=gcc-11 OMP=1
```

#### Runtime Issues

**Issue**: Numerical instability in SLV pricing
- Reduce time step size: increase `num_steps` parameter
- Use Andersen QE scheme: set `use_andersen_qe=true`
- Check Heston parameters for realistic values

**Issue**: Slow Monte Carlo convergence
- Increase number of paths
- Enable variance reduction techniques
- Use quasi-Monte Carlo for smooth payoffs

**Issue**: PDE pricing inaccuracies
- Increase grid resolution
- Check boundary conditions for exotic options
- Ensure stability criterion is satisfied

### Performance Optimization

#### Memory Usage
```bash
# Monitor memory usage
valgrind --tool=massif ./build/release/bin/bsm

# For large simulations, consider batching
# Split large path counts into smaller chunks
```

#### CPU Optimization
```bash
# Check CPU features
cat /proc/cpuinfo | grep flags

# Compile for specific architecture
make CXXFLAGS="-O3 -march=skylake"

# Profile hot paths
perf record ./build/release/bin/bsm
perf report
```

### Getting Help

1. **Documentation**: Check `docs/` directory
2. **API Reference**: See `docs/api_reference.md`
3. **Performance Guide**: See `docs/performance_guide.md`
4. **Source Code**: All code is well-commented
5. **Issues**: Report bugs on GitHub

## 📈 Performance Tuning

### Monte Carlo Optimization

```cpp
// Optimal path count selection
auto estimate_optimal_paths = [](double target_se, double initial_se, long initial_paths) {
    return static_cast<long>(std::pow(initial_se / target_se, 2) * initial_paths);
};

// Example: Achieve 0.01 standard error
long paths = estimate_optimal_paths(0.01, 0.1, 10000);  // ~1M paths
```

### Memory-Efficient Large Simulations

```cpp
// Batch processing for memory constraints
double batch_monte_carlo(/* parameters */, long total_paths, long batch_size) {
    double total_sum = 0.0;
    long completed_paths = 0;
    
    while (completed_paths < total_paths) {
        long current_batch = std::min(batch_size, total_paths - completed_paths);
        MCResult batch_result = mc_gbm_price(/* params */, current_batch, /* ... */);
        
        total_sum += batch_result.price * current_batch;
        completed_paths += current_batch;
    }
    
    return total_sum / static_cast<double>(total_paths);
}
```

### Parallel Processing

```cpp
#ifdef _OPENMP
#include <omp.h>

void parallel_portfolio_pricing() {
    std::vector<Position> portfolio = load_portfolio();
    std::vector<double> prices(portfolio.size());
    
    #pragma omp parallel for
    for (size_t i = 0; i < portfolio.size(); ++i) {
        prices[i] = black_scholes_price(/* portfolio[i] parameters */);
    }
    
    double total_value = std::accumulate(prices.begin(), prices.end(), 0.0);
}
#endif
```

## 🎯 Best Practices

### Code Organization

```cpp
// Group related functionality
namespace MyQuantLib {
    using namespace bsm;
    
    class OptionPricer {
    private:
        double S0, K, r, T, sigma;
        OptionType type;
        
    public:
        OptionPricer(double s0, double k, double rate, double time, double vol, OptionType opt_type)
            : S0(s0), K(k), r(rate), T(time), sigma(vol), type(opt_type) {}
        
        double analytical_price() const {
            return black_scholes_price(S0, K, r, T, sigma, type);
        }
        
        MCResult monte_carlo_price(long paths) const {
            return mc_gbm_price(S0, K, r, T, sigma, paths, type);
        }
        
        double pde_price(int S_steps, int T_steps) const {
            return pde_crank_nicolson(S0, K, r, T, sigma, S_steps, T_steps, type);
        }
    };
}
```

### Error Handling

```cpp
#include <stdexcept>

double safe_black_scholes_price(double S0, double K, double r, double T, double sigma, OptionType type) {
    if (S0 <= 0.0) throw std::invalid_argument("Spot price must be positive");
    if (K <= 0.0) throw std::invalid_argument("Strike price must be positive");
    if (T < 0.0) throw std::invalid_argument("Time to expiration cannot be negative");
    if (sigma < 0.0) throw std::invalid_argument("Volatility cannot be negative");
    
    return black_scholes_price(S0, K, r, T, sigma, type);
}
```

### Unit Testing

```cpp
#include <gtest/gtest.h>  // If using Google Test

TEST(BlackScholesTest, PutCallParity) {
    double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    
    double call = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
    double put = black_scholes_price(S0, K, r, T, sigma, OptionType::Put);
    
    double parity_diff = call - put - (S0 - K * std::exp(-r * T));
    EXPECT_NEAR(parity_diff, 0.0, 1e-10);
}

TEST(MonteCarloTest, ConvergenceToAnalytical) {
    double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    
    double analytical = black_scholes_price(S0, K, r, T, sigma, OptionType::Call);
    MCResult mc_result = mc_gbm_price(S0, K, r, T, sigma, 1000000, OptionType::Call);
    
    EXPECT_NEAR(mc_result.price, analytical, 3.0 * mc_result.std_error);
}
```

This user guide provides comprehensive coverage of the toolkit's features and best practices for effective usage.
