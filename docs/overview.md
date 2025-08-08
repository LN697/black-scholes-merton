# Project Overview

This project provides a compact, modular option pricing toolkit with numerical and Monte Carlo engines:

- Black–Scholes analytics and implied volatility solver
- PDE Crank–Nicolson for European options and projected CN for American options
- GBM Monte Carlo with antithetics, two-pass control variate, Halton QMC, and Greeks (pathwise delta, LR vega)
- Stochastic Local Volatility (SLV) with Heston variance (Andersen QE scheme) and configurable local vol (CEV/Smile)
- Longstaff–Schwartz MC (LSM) American put pricer

## Architecture

- Headers in `include/`, sources in `src/`, tests in `test/`, docs in `docs/`.
- `Makefile` builds to `build/bin` and `build/obj`; run `make` for app, `make test` for tests, `make clean` to clean.
- Executables:
	- `bsm`: demo/driver that runs analytics, MC, PDE, and SLV snippets.
	- `tests`: unit tests covering PDE vs analytic, MC vs analytic, American ≥ European, and SLV sanity.

## Components

### Analytics (Black–Scholes)
- File: `include/analytic_bs.hpp`, `src/analytic_bs.cpp`
- Features: Call/Put pricing; implied volatility root solver in `include/iv_solve.hpp`.

### Monte Carlo GBM
- File: `include/monte_carlo_gbm.hpp`, `src/monte_carlo_gbm.cpp`
- Features:
	- Antithetic variates
	- Control variate vs terminal spot with two-pass beta estimation
	- Halton QMC sequence (bases 2,3) with random shifts; Box–Muller transform
	- Greeks: pathwise delta, likelihood-ratio (LR) vega
	- Optional OpenMP parallel reduction hooks

### PDE: Crank–Nicolson (European) and Projected CN (American)
- File: `include/pde_cn.hpp`, `src/pde_cn.cpp`; `include/pde_cn_american.hpp`, `src/pde_cn_american.cpp`
- Features: robust tri-diagonal solver, proper boundary handling, and early exercise projection for American puts.

### Stochastic Local Volatility (SLV)
- File: `include/slv.hpp`, `src/slv.cpp`
- Model: dS_t = r S_t dt + L(S,t) √v_t σ_loc(S,t) S_t dW_t^S; dv_t = κ(θ − v_t)dt + ξ√v_t dW_t^v with corr ρ.
- Implementation:
	- Heston variance via Andersen QE discretization
	- Local vol as a pluggable function: CEV and Smile examples (`LocalVolFn`)
	- Antithetics and QMC support via shared utilities
- Calibration: see below for L(S,t) iterative loop (scaffold present in `include/slv_calibration.hpp`).

### Longstaff–Schwartz (LSM) American Put
- File: `include/lsm.hpp`, `src/lsm.cpp`
- Features: polynomial basis regression on in-the-money paths, Gaussian elimination solver, stepwise discounting.

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
