# Standard BSM + PDE + MC + SLV (C++)

A modular C++ project that prices European and American options via:
- Analytical Black–Scholes formula
- Finite Difference PDE (Crank–Nicolson)
- Monte Carlo under GBM (antithetic, QMC, control variate)
- Stochastic Local Volatility (SLV) Monte Carlo with Heston variance and configurable local volatility (CEV or Smile) and Andersen QE variance scheme

It includes a verification harness and a basic unit-test binary.

## Layout

```
include/               # Public headers
src/                   # Sources (app and libs)
test/                  # Unit tests
build/                 # Build outputs (bin/ and obj/) after make
Makefile               # GNU Make build (MinGW/MSYS/WSL/macOS/Linux)
```

## Build

Windows options:
- MinGW/MSYS make:
  - Install a GCC toolchain (e.g., MSYS2) and run:
    ```pwsh
    make
    ```
- Without make (PowerShell):
  ```pwsh
  g++ -std=c++17 -O3 -Iinclude src/*.cpp -o bsm.exe
  .\bsm.exe
  ```

Linux/macOS:
```bash
make
./build/bin/bsm
```

If `g++` is not found, install it (e.g., `choco install mingw` on Windows, or Xcode/CommandLineTools on macOS, or `build-essential` on Ubuntu).

## Run

After building with make, run:
```bash
./build/bin/bsm
```

You will see analytical BS, MC, PDE, and SLV prices, plus per-seed SLV runs.

## Implemented and planned extensions

- Antithetic variates (GBM and SLV MC) [done]
- Standard error reporting for MC [done]
- Parameterizable local-vol models (CEV, Smile) [done]
- Control variates against BS in GBM MC [basic done]
- Quasi-Monte Carlo (Halton) [done]
- Andersen QE scheme for Heston variance [done]
- Leverage from Dupire local vol surface (approximate) [done]
- Greeks via pathwise/likelihood ratio estimators [planned]
- Implied volatility solver [done]
- American options via PDE (projected CN) [done]
- Parallel simulation via OpenMP (optional) [Make toggle]
- Optional CMake generation target [Make target]

## Tests

Build and run unit tests:
```bash
make test
```
Executable: `build/bin/tests`.

OpenMP parallel MC:
```bash
make OMP=1
```

## License

MIT
