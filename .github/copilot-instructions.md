# AI agent instructions for this repo (standard-bsm)

Purpose: C++17 pricing toolkit with analytics, PDE, Monte Carlo (GBM), and SLV. Keep edits modular (headers in `include/`, sources in `src/`), and use the Makefile-driven workflow.

## Architecture map (what talks to what)
- `include/` public APIs → implemented in matching files under `src/`.
  - Analytics: `analytic_bs.hpp/cpp` (Black–Scholes), `iv_solve.hpp` (implied vol solver).
  - Monte Carlo GBM: `monte_carlo_gbm.hpp/cpp` → returns `MCResult` (`include/stats.hpp`). Flags: antithetic, control variate (two-pass), QMC, Greeks.
  - PDE: `pde_cn.hpp/cpp` (European CN), `pde_cn_american.hpp/cpp` (projected CN for American).
  - SLV: `slv.hpp/cpp` (Heston + local vol via `LocalVolFn`); example local vols: CEV, Smile. QE discretization is built-in. Multi-seed helper present.
  - LSM: `lsm.hpp/cpp` (Longstaff–Schwartz American put).
  - Calibration scaffolding: `dupire.hpp` (Dupire surface), `slv_calibration.hpp` (leverage grid + iterative loop scaffold).
- `test/tests.cpp` is a simple main-based unit test (no framework). Do not add tests in `src/`.
- `docs/overview.md` explains components and the SLV calibration outline.

## Build and test (use Make)
- Targets: `build/bin/bsm` and `build/bin/tests`.
- Typical workflow (Windows MinGW/MSYS or Linux/macOS):
  - Build: `make`
  - Run tests: `make test`
  - Clean: `make clean`
- Options:
  - OpenMP: `make OMP=1` (adds `-fopenmp`).
- The Makefile auto-discovers `src/*.cpp` and `test/*.cpp`. It purposely excludes any `src/tests.cpp` from the app to avoid multiple mains.

## Conventions and patterns specific to this repo
- Keep public types/APIs in `include/*` and implement in `src/*` with identical basenames.
- Monte Carlo APIs return `MCResult { price, std_error, [delta, vega, delta_se, vega_se] }` from `include/stats.hpp`.
- Option types via `include/option_types.hpp`.
- QMC uses Halton (bases 2/3) with random shifts (`Halton2D` in `math_utils.hpp`) and Box–Muller. Utilities include stateless helpers (`normals_from_index`, `halton_from_index`) for deterministic, index-based sampling.
- PDE solvers use CN with a tri-diagonal Thomas solve; American uses projected CN.
- SLV composes Heston variance (Andersen QE) with a pluggable local-vol function: pass `LocalVolFn` (e.g., from `CEVLocalVol::to_fn()` or `SmileLocalVol::to_fn()`).
- Windows-friendly Makefile: uses conditional `mkdir` and `rmdir`—don’t replace with Unix-only commands.

## Extensibility points (with examples)
- New local vol: add a small functor in `slv.hpp` exposing `.to_fn()` returning `LocalVolFn` (double S,double t)->sigma.
- New Greeks in MC: extend `MCResult` and accumulate in `src/monte_carlo_gbm.cpp` alongside existing delta (pathwise) and vega (LR).
- New tests: create `test/*.cpp` with a `main()` printing failures to stderr and returning nonzero on failure (see `test/tests.cpp`).
- SLV leverage calibration: wire `dupire.hpp` + `slv_calibration.hpp` by implementing the model-implied local variance estimator inside the iterative loop.

## Gotchas and tips
- OpenMP: the GBM loop is guarded with `#ifdef _OPENMP` reductions. If you parallelize new loops, copy this pattern. For reproducible streams across threads, prefer the stateless RNG helpers in `math_utils.hpp`.
- QMC + threads: `Halton2D` is stateful; for parallel/QMC, index-based helpers are available.
- Compiler flags: `-Wall -Wextra -Wpedantic -O3 -march=native` are enabled—fix warnings rather than suppress them.
- Binary names: app is `bsm` (not `standard-bsm`); tests are `tests` under `build/bin/`.

## Quick file guide
- Core: `include/analytic_bs.hpp`, `include/monte_carlo_gbm.hpp`, `include/pde_cn.hpp`, `include/pde_cn_american.hpp`, `include/slv.hpp`.
- Utilities: `include/math_utils.hpp`, `include/stats.hpp`, `include/option_types.hpp`.
- Calibration scaffold: `include/dupire.hpp`, `include/slv_calibration.hpp`.
- Entry/demo: `src/main.cpp` (don’t put tests here).
