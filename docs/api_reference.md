# API Reference

This document provides a comprehensive reference for all public APIs in the Black-Scholes-Merton pricing toolkit.

## Core Types and Enumerations

### `OptionType`
```cpp
enum class OptionType { Call, Put };
```
Specifies the option type for pricing functions.

### `MCResult`
```cpp
struct MCResult {
    double price{};        // Option price estimate
    double std_error{};    // Standard error of the estimate
    double delta{};        // Delta (∂Price/∂S₀)
    double delta_se{};     // Standard error of delta
    double vega{};         // Vega (∂Price/∂σ)
    double vega_se{};      // Standard error of vega
};
```
Result structure for Monte Carlo pricing methods, including price estimates and Greeks with their standard errors.

### `HestonParams`
```cpp
struct HestonParams {
    double v0{0.04};       // Initial variance
    double kappa{2.0};     // Mean reversion speed
    double theta{0.04};    // Long-term variance
    double xi{0.3};        // Vol of vol
    double rho{-0.7};      // Correlation between asset and variance
};
```
Parameters for the Heston stochastic volatility model.

## Analytical Pricing

### `black_scholes_price`
```cpp
double black_scholes_price(double S0, double K, double r, double T, 
                          double sigma, OptionType type);
```

**Description**: Computes the analytical Black-Scholes price for European options.

**Parameters**:
- `S0`: Current asset price
- `K`: Strike price  
- `r`: Risk-free rate (continuously compounded)
- `T`: Time to expiration (in years)
- `sigma`: Volatility (annualized)
- `type`: Option type (Call or Put)

**Returns**: Option price

**Example**:
```cpp
double call_price = black_scholes_price(100.0, 105.0, 0.05, 0.25, 0.2, OptionType::Call);
// Result: ~2.32 for a 3-month ATM call with 20% vol
```

**Mathematical Formula**:
- **Call**: `S₀N(d₁) - Ke^(-rT)N(d₂)`
- **Put**: `Ke^(-rT)N(-d₂) - S₀N(-d₁)`

Where:
- `d₁ = [ln(S₀/K) + (r + σ²/2)T] / (σ√T)`
- `d₂ = d₁ - σ√T`

### `implied_vol`
```cpp
double implied_vol(double market_price, 
                   std::function<double(double)> bs_price_fn,
                   double initial_guess = 0.2, double tolerance = 1e-6);
```

**Description**: Solves for implied volatility using Newton-Raphson method.

**Parameters**:
- `market_price`: Observed market price
- `bs_price_fn`: Function that takes volatility and returns BS price
- `initial_guess`: Starting point for solver (default: 0.2)
- `tolerance`: Convergence tolerance (default: 1e-6)

**Returns**: Implied volatility

**Example**:
```cpp
auto price_fn = [=](double vol) { 
    return black_scholes_price(100.0, 100.0, 0.05, 1.0, vol, OptionType::Call); 
};
double iv = implied_vol(10.45, price_fn);
// Result: ~0.20 (20% implied volatility)
```

## Monte Carlo Pricing

### `mc_gbm_price`
```cpp
MCResult mc_gbm_price(double S0, double K, double r, double T, double sigma,
                      long num_paths, OptionType type, 
                      unsigned long seed = 12345,
                      bool antithetic = true, 
                      bool control_variate = true, 
                      bool use_qmc = false,
                      bool two_pass_cv = true, 
                      bool compute_greeks = true);
```

**Description**: Monte Carlo pricing under geometric Brownian motion with multiple variance reduction techniques.

**Parameters**:
- `S0, K, r, T, sigma`: Standard option parameters
- `num_paths`: Number of simulation paths
- `type`: Option type
- `seed`: Random number generator seed
- `antithetic`: Enable antithetic variates (default: true)
- `control_variate`: Enable control variate technique (default: true)
- `use_qmc`: Use quasi-Monte Carlo (Halton) instead of pseudo-random (default: false)
- `two_pass_cv`: Use two-pass control variate for optimal beta (default: true)
- `compute_greeks`: Calculate delta and vega (default: true)

**Returns**: `MCResult` with price, standard error, and Greeks

**Example**:
```cpp
MCResult result = mc_gbm_price(100.0, 100.0, 0.05, 1.0, 0.2, 
                               1000000, OptionType::Call, 12345UL,
                               true, true, false, true, true);
                               
std::cout << "Price: " << result.price << " ± " << result.std_error << std::endl;
std::cout << "Delta: " << result.delta << " ± " << result.delta_se << std::endl;
```

**Performance Notes**:
- **Antithetic Variates**: ~50% variance reduction, negligible cost
- **Control Variates**: 60-80% variance reduction, ~20% additional cost
- **QMC**: Better convergence rate, similar computational cost
- **Greeks**: ~50% additional cost when enabled

### `mc_slv_price`
```cpp
MCResult mc_slv_price(double S0, double K, double r, double T,
                      long num_paths, long num_steps, OptionType type,
                      const HestonParams& heston, 
                      const LocalVolFn& local_vol_fn,
                      unsigned long seed = 12345,
                      bool antithetic = true, 
                      bool use_andersen_qe = true);
```

**Description**: Monte Carlo pricing under Stochastic Local Volatility model combining Heston variance with local volatility.

**Parameters**:
- `S0, K, r, T`: Standard option parameters
- `num_paths`: Number of simulation paths
- `num_steps`: Number of time steps per path
- `type`: Option type
- `heston`: Heston model parameters
- `local_vol_fn`: Local volatility function σ(S,t)
- `seed`: Random number generator seed
- `antithetic`: Enable antithetic variates
- `use_andersen_qe`: Use Andersen QE scheme for variance (recommended)

**Returns**: `MCResult` with price and standard error

**Example**:
```cpp
HestonParams heston{0.04, 2.0, 0.04, 0.3, -0.7};
SmileLocalVol smile{0.22, 0.95, 0.25, 0.15, 100.0, 0.01};

MCResult slv_result = mc_slv_price(100.0, 100.0, 0.05, 1.0,
                                   500000, 252, OptionType::Call,
                                   heston, smile.to_fn(), 77777UL,
                                   true, true);
```

## PDE Solvers

### `pde_crank_nicolson`
```cpp
double pde_crank_nicolson(double S0, double K, double r, double T, double sigma,
                         int num_S_steps, int num_T_steps, OptionType type);
```

**Description**: European option pricing using Crank-Nicolson finite difference scheme.

**Parameters**:
- `S0, K, r, T, sigma`: Standard option parameters
- `num_S_steps`: Number of spatial grid points
- `num_T_steps`: Number of time steps
- `type`: Option type

**Returns**: Option price

**Example**:
```cpp
double pde_price = pde_crank_nicolson(100.0, 100.0, 0.05, 1.0, 0.2, 
                                      300, 150, OptionType::Call);
// Highly accurate European option price
```

**Grid Setup**:
- **S-axis**: [0, S_max] where S_max = 3×S₀ typically
- **T-axis**: [0, T] with uniform time steps
- **Stability**: Unconditionally stable for Crank-Nicolson (θ = 0.5)

### `pde_crank_nicolson_american`
```cpp
double pde_crank_nicolson_american(double S0, double K, double r, double T, double sigma,
                                  int num_S_steps, int num_T_steps, OptionType type);
```

**Description**: American option pricing using projected Crank-Nicolson scheme.

**Parameters**: Same as European version

**Returns**: American option price (≥ European price)

**Example**:
```cpp
double american_put = pde_crank_nicolson_american(100.0, 110.0, 0.05, 1.0, 0.3,
                                                  400, 200, OptionType::Put);
```

**Algorithm**: Applies early exercise constraint at each time step:
```cpp
V_new = max(CN_step(V_old), Payoff)
```

## Local Volatility Models

### `CEVLocalVol`
```cpp
class CEVLocalVol {
public:
    CEVLocalVol(double sigma0, double beta, double S0);
    LocalVolFn to_fn() const;
};
```

**Description**: Constant Elasticity of Variance local volatility model.

**Parameters**:
- `sigma0`: Reference volatility level
- `beta`: Elasticity parameter (β = 1 for log-normal, β < 1 for leverage effect)
- `S0`: Reference spot price

**Mathematical Model**: `σ(S) = σ₀(S/S₀)^(β-1)`

**Example**:
```cpp
CEVLocalVol cev{0.25, 0.9, 100.0};  // 25% vol, 90% elasticity
auto lv_fn = cev.to_fn();
double vol_at_110 = lv_fn(110.0, 0.5);  // Vol when S=110, t=0.5
```

### `SmileLocalVol`
```cpp
class SmileLocalVol {
public:
    SmileLocalVol(double sigma_atm, double skew, double convexity, 
                  double term_structure, double S0, double min_vol);
    LocalVolFn to_fn() const;
};
```

**Description**: Parametric local volatility model capturing smile and term structure.

**Parameters**:
- `sigma_atm`: At-the-money volatility
- `skew`: Linear skew parameter
- `convexity`: Convexity (smile curvature)
- `term_structure`: Time decay of volatility
- `S0`: Reference spot price
- `min_vol`: Minimum volatility floor

**Mathematical Model**:
```cpp
σ(S,t) = σ_ATM × (1 + skew×ln(S/S₀) + convexity×ln²(S/S₀)) × exp(-term_structure×t)
```

**Example**:
```cpp
SmileLocalVol smile{0.22, 0.95, 0.25, 0.15, 100.0, 0.01};
auto smile_fn = smile.to_fn();
// Creates downward-sloping volatility smile with term structure decay
```

## Utility Functions

### Random Number Generation

#### `RNG`
```cpp
class RNG {
public:
    RNG();                          // Random seed
    explicit RNG(uint64_t seed);    // Fixed seed
    
    double gauss();                 // Standard normal random variable
    double uni();                   // Uniform [0,1) random variable
};
```

#### `Halton2D`
```cpp
class Halton2D {
public:
    Halton2D();                     // Default construction
    explicit Halton2D(uint64_t seed); // With random shifts
    
    std::pair<double, double> next(); // Next quasi-random pair
};
```

### Statistical Functions

#### `norm_cdf`
```cpp
double norm_cdf(double x);
```
Standard normal cumulative distribution function using error function.

#### `correlated_gaussians`
```cpp
void correlated_gaussians(double rho, RNG& rng, double& z1, double& z2);
```
Generates pair of correlated Gaussian random variables with correlation ρ.

## Error Handling and Edge Cases

### Input Validation
- **Negative Parameters**: Prices, strikes, and volatilities must be non-negative
- **Zero Time**: T ≤ 0 returns intrinsic value
- **Zero Volatility**: σ ≤ 0 returns discounted intrinsic value

### Numerical Stability
- **Large Parameters**: Robust handling of extreme input values
- **Underflow/Overflow**: Graceful degradation with appropriate limits
- **Convergence**: Iterative methods include maximum iteration limits

### Performance Considerations
- **Memory Usage**: O(N) for MC methods, O(N²) for PDE grids
- **Computational Complexity**: See individual method documentation
- **Parallelization**: OpenMP support where applicable

## Building and Linking

### Header Dependencies
```cpp
#include "analytic_bs.hpp"        // Analytical pricing
#include "monte_carlo_gbm.hpp"    // Monte Carlo methods  
#include "pde_cn.hpp"             // PDE solvers
#include "slv.hpp"                // SLV framework
#include "math_utils.hpp"         // Mathematical utilities
#include "stats.hpp"              // Result structures
```

### Compiler Requirements
- **C++17**: Required for std::function, constexpr, etc.
- **Math Library**: Link with `-lm` on Unix systems
- **OpenMP**: Optional, enable with `-fopenmp`

### Example Compilation
```bash
g++ -std=c++17 -O3 -march=native -Iinclude your_code.cpp src/*.cpp -o your_app
```
