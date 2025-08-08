# Technical Architecture Overview

This document provides a comprehensive technical overview of the Black-Scholes-Merton pricing toolkit, detailing the mathematical models, numerical methods, and software architecture.

## 🏗️ System Architecture

### Design Principles
- **Modularity**: Each pricing method is self-contained with clean interfaces
- **Performance**: Optimized algorithms with minimal runtime overhead
- **Extensibility**: Plugin architecture for new models and payoffs
- **Reliability**: Comprehensive testing and numerical validation
- **Portability**: Cross-platform C++17 with minimal dependencies

### Core Components Hierarchy
```
Application Layer (main.cpp)
    ├── Analytics Module (analytic_bs.hpp)
    ├── Monte Carlo Engine (monte_carlo_gbm.hpp)
    ├── PDE Solvers (pde_cn*.hpp)
    ├── SLV Framework (slv.hpp)
    └── Utilities Layer (math_utils.hpp, stats.hpp)
```

### Memory and Performance Architecture
- **Stack-allocated structures**: Minimal heap allocations for hot paths
- **SIMD-friendly algorithms**: Vectorizable loops and data layouts
- **Cache-conscious design**: Sequential memory access patterns
- **Optional parallelization**: OpenMP for embarrassingly parallel workloads

## 📊 Mathematical Models and Algorithms

### 1. Black-Scholes Analytical Engine
**File**: `include/analytic_bs.hpp`, `src/analytic_bs.cpp`

**Mathematical Model**:
```
dS_t = rS_t dt + σS_t dW_t

European Call: C = S₀N(d₁) - Ke^(-rT)N(d₂)
European Put:  P = Ke^(-rT)N(-d₂) - S₀N(-d₁)

where:
d₁ = [ln(S₀/K) + (r + σ²/2)T] / (σ√T)
d₂ = d₁ - σ√T
```

**Implementation Features**:
- Robust numerical evaluation with edge case handling
- Optimized normal CDF computation using error functions
- Intrinsic value fallback for degenerate cases (σ ≤ 0, T ≤ 0)

**Complexity**: O(1) per evaluation

### 2. Monte Carlo GBM Engine
**File**: `include/monte_carlo_gbm.hpp`, `src/monte_carlo_gbm.cpp`

**Core Algorithm**:
```cpp
// Geometric Brownian Motion simulation
S_t = S₀ * exp((r - σ²/2)T + σ√T * Z)
where Z ~ N(0,1)
```

**Variance Reduction Techniques**:

#### Antithetic Variates
```cpp
Path 1: S₁ = S₀ * exp((r - σ²/2)T + σ√T * Z)
Path 2: S₂ = S₀ * exp((r - σ²/2)T - σ√T * Z)
Estimate: (Payoff(S₁) + Payoff(S₂)) / 2
```
- **Variance Reduction**: ~50% for path-dependent options
- **Computational Cost**: Negligible additional overhead

#### Control Variates
Two-pass implementation with optimal beta estimation:
```cpp
// Pass 1: Estimate β = Cov(Payoff, Control) / Var(Control)
// Pass 2: Adjusted_Payoff = Payoff - β(Control - E[Control])
```
- **Control Variable**: Terminal spot price vs analytical expectation
- **Variance Reduction**: 60-80% for vanilla options

#### Quasi-Monte Carlo (Halton Sequences)
```cpp
// Halton sequence generation for bases 2 and 3
H_n^(b) = Σ(d_i * b^(-i-1))  where n = Σ(d_i * b^i)
```
- **Convergence Rate**: O(log(N)²/N) vs O(1/√N) for standard MC
- **Implementation**: Scrambled Halton with random shifts for better uniformity

#### Greeks Calculation
**Pathwise Estimator (Delta)**:
```cpp
δ = ∂Payoff/∂S₀ = (∂Payoff/∂S_T) * (S_T/S₀)
```

**Likelihood Ratio Estimator (Vega)**:
```cpp
ν = Payoff * ∂ln(f)/∂σ = Payoff * [(Z²-1)/σ - Z√T]
where f is the transition density
```

**Complexity**: O(N) for N simulation paths
**Parallelization**: OpenMP reduction over independent path batches

### 3. PDE Finite Difference Solvers
**Files**: `include/pde_cn.hpp`, `include/pde_cn_american.hpp`

**Mathematical Framework**:
```
∂V/∂t + (1/2)σ²S²∂²V/∂S² + rS∂V/∂S - rV = 0

Boundary Conditions:
- S = 0: V(0,t) = max(K*e^(-r(T-t)) - 0, 0) for puts
- S → ∞: V(S,t) ≈ S - K*e^(-r(T-t)) for calls
- t = T: V(S,T) = Payoff(S)
```

#### Crank-Nicolson Discretization
**Spatial Grid**: Uniform spacing in S-direction
**Time Stepping**: Implicit-explicit θ-scheme with θ = 0.5

**Discretized Equation**:
```
[I - (Δt/2)L] V^(n+1) = [I + (Δt/2)L] V^n
```

**Tri-diagonal System Solution**:
- **Thomas Algorithm**: O(N) complexity for banded matrix inversion
- **Stability**: Unconditionally stable for θ ≥ 0.5
- **Accuracy**: O(Δt², ΔS²) truncation error

#### American Option Enhancement (Projected Crank-Nicolson)
**Early Exercise Constraint**: V(S,t) ≥ Payoff(S) ∀t

**Algorithmic Implementation**:
```cpp
for each time step:
    1. Solve European step: V_temp = CN_step(V_old)
    2. Apply constraint: V_new = max(V_temp, Payoff)
    3. Update boundary conditions
```

**Convergence**: Monotonic convergence to optimal stopping boundary

### 4. Stochastic Local Volatility (SLV) Framework
**File**: `include/slv.hpp`, `src/slv.cpp`

**Mathematical Model**:
```
dS_t = rS_t dt + L(S,t)√v_t σ_local(S,t) S_t dW_t^S
dv_t = κ(θ - v_t)dt + ξ√v_t dW_t^v

⟨dW_t^S, dW_t^v⟩ = ρ dt
```

**Components**:
1. **Heston Variance Process**: Andersen QE discretization scheme
2. **Local Volatility Function**: Pluggable interface (CEV, Smile, etc.)
3. **Leverage Function**: L(S,t) for calibration to market data

#### Andersen QE Discretization
**Variance Evolution**:
```cpp
if ψ ≤ ψ_critical:
    // Central chi-squared approximation
    v_{n+1} = a(b + Z²)
else:
    // Quadratic exponential scheme  
    v_{n+1} = a(b + Z)² for Z > ψ⁻¹, else 0
```

**Advantages**:
- **Positivity**: Ensures v_t ≥ 0 always
- **Accuracy**: Superior to Euler or Milstein schemes
- **Efficiency**: O(1) per step with pre-computed coefficients

#### Local Volatility Models

**CEV (Constant Elasticity of Variance)**:
```cpp
σ_local(S,t) = σ₀ * (S/S₀)^(β-1)
```

**Smile Model** (Parametric Local Volatility):
```cpp
σ_local(S,t) = σ_ATM * (1 + α * log(S/K) + β * log²(S/K)) * exp(-γ*t)
```

**Interface Design**:
```cpp
using LocalVolFn = std::function<double(double S, double t)>;
```

### 5. Longstaff-Schwartz Algorithm (LSM)
**File**: `include/lsm.hpp`, `src/lsm.cpp`

**American Put Pricing via Regression**:

**Algorithm Steps**:
1. **Forward Simulation**: Generate all price paths to maturity
2. **Backward Induction**: At each exercise date:
   - **Regression**: Fit continuation value on in-the-money paths
   - **Decision**: Exercise if immediate payoff > continuation value
   - **Update**: Cash flows and optimal exercise strategy

**Basis Functions**:
```cpp
Φ(S) = [1, S/K, (S/K)², ..., (S/K)ⁿ]
```

**Regression Equation**:
```cpp
Continuation(S) = Σᵢ αᵢ Φᵢ(S)
where α = (ΦᵀΦ)⁻¹ΦᵀY
```

**Complexity**: O(N×M×P) for N paths, M time steps, P basis functions

### Utilities
- File: `include/math_utils.hpp`, `include/stats.hpp`
- Contains: normal CDF, RNG, correlated Gaussians, Halton sequence with shifts, Box–Muller, stateless hash RNG for indexed sampling, and result structs with standard errors.

## SLV Calibration Notes

Goal: find leverage L(S,t) so the model-implied local variance matches a target Dupire surface σ_target(S,t).

Iterative outline (PDE or particle MC):
1. Initialize L₀(S,t) = 1 (or from a heuristic).
2. Under current L, price a thin grid of options (or estimate local variance) to get σ_model(S,t).
3. Update: L_{k+1}(S,t) = L_k(S,t) · σ_target(S,t) / max(σ_model(S,t), ε).
4. Repeat until convergence on a mesh in (S,t).

Code scaffolding: `include/dupire.hpp` (surface representation) and `include/slv_calibration.hpp` (grid and loop skeleton) are in place; the σ_model estimator is the next implementation step.

## How to Build and Run

Windows (MinGW) and Linux are supported via GNU Make:

```sh
make
make test
make clean
```

Toggle OpenMP with `OMP=1` (requires compiler support):

```sh
make OMP=1
```

## References and Concepts
- Black–Scholes (1973); Crank–Nicolson finite differences; Thomas algorithm for tri-diagonal systems.
- Control variates, antithetic sampling; Halton quasi-random sequences; Box–Muller transform.
- Heston (1993) and Andersen QE (2008) discretization.
- Dupire (1994) local volatility; SLV leverage calibration via fixed-point iteration.
