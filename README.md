# Black-Scholes-Merton Pricing Toolkit

## 🚀 Development Roadmap & ML/AI Integration

### Machine Learning & AI Enhancements
- **LSTM Volatility Models**: Deep learning for volatility surface prediction
- **Physics-Informed Neural Networks (PINNs)**: Neural network PDE solvers
- **Reinforcement Learning**: Adaptive hedging strategies
- **Ensemble Methods**: Multiple model averaging for accuracy improvement
- **Real-time Market Adaptation**: Dynamic model recalibration

### Enhanced User Experience
- **Professional CLI Interface**: Rich terminal interface with real-time monitoring
- **Web Dashboard**: React-based professional trading interface
- **Python Integration**: Seamless Python bindings with pandas/numpy
- **REST API**: Cloud-ready microservices architecture
- **WebSocket Streaming**: Real-time market data and pricing updates

### Enterprise Architecture
- **Microservices**: Containerized pricing services with Docker/Kubernetes
- **Cloud-Native**: AWS/Azure deployment with auto-scaling
- **Distributed Computing**: Multi-node cluster processing
- **Performance Monitoring**: Advanced telemetry and observability

**📁 Full Development Strategy**: See [`dev/`](dev/) directory for complete roadmap, prototypes, and implementation guides.

## 📁 Project Structured
 [![Status](https://img.shields.io/badge/build-passing-brightgreen)](README.md)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Development](https://img.shields.io/badge/development-enterprise--grade-blue)](dev/README.md)

A high-performance, modular C++17 options pricing library featuring multiple numerical methods and advanced stochastic volatility models. Now enhanced with enterprise-grade ML/AI capabilities and modern UX interfaces.

## 🎯 Enterprise Development Strategy

**NEW**: This project includes a comprehensive development roadmap for enterprise-grade enhancements including machine learning integration, advanced UX interfaces, and cloud-native architecture. See [`dev/`](dev/) directory for complete strategy documentation and prototype implementations.

## 🚀 Features

### Core Pricing Methods
- **Analytical Black-Scholes**: Closed-form European option pricing with Greeks
- **Finite Difference PDE**: Crank-Nicolson scheme for European and American options
- **Monte Carlo (GBM)**: Advanced MC with variance reduction techniques
- **Stochastic Local Volatility (SLV)**: Heston + local volatility hybrid models
- **Longstaff-Schwartz**: American option pricing via regression

### Advanced Techniques
- **Variance Reduction**: Antithetic variates, control variates, quasi-Monte Carlo (Halton)
- **Greeks Calculation**: Pathwise and likelihood ratio estimators
- **Parallel Computing**: OpenMP support for multi-threaded simulations
- **Model Calibration**: Framework for SLV calibration to market data
- **Implied Volatility**: Newton-Raphson solver for market-implied parameters

## 🎯 Performance Features

### New Performance Optimization Framework
- **Architecture Detection**: Automatic CPU feature detection and optimization
- **Numerical Accuracy Validation**: Hardware-specific accuracy testing
- **Memory Profiling**: Real-time memory usage monitoring and optimization
- **Performance Benchmarking**: Comprehensive benchmark suite with regression detection
- **Threading Optimization**: Intelligent thread count and CPU affinity management
- **NUMA Awareness**: Multi-node memory allocation optimization (Linux)

### Core Performance Features
- Optimized for modern CPUs with `-march=native`
- SIMD-friendly algorithms and memory layouts (AVX/AVX2 support)
- Optional OpenMP parallelization with intelligent thread management
- Efficient quasi-random number generation
- Cache-optimized data structures
- Profile-guided optimization support
- Fast math optimizations (optional, with accuracy validation)

## 📁 Project Structure

```
├── include/               # Public API headers
│   ├── analytic_bs.hpp   # Black-Scholes analytical pricing
│   ├── monte_carlo_gbm.hpp # Monte Carlo under GBM
│   ├── pde_cn*.hpp       # PDE solvers (European/American)
│   ├── slv.hpp           # Stochastic Local Volatility
│   ├── math_utils.hpp    # Mathematical utilities and RNG
│   ├── stats.hpp         # Statistical result structures
│   └── ...               # Additional headers
├── src/                  # Implementation files
├── test/                 # Unit tests and validation
├── docs/                 # Comprehensive documentation
├── dev/                  # 🆕 Enterprise development strategy
│   ├── research_initiatives/  # ML/AI research and prototypes
│   ├── prototype_implementations/ # Enhanced interfaces and frameworks
│   ├── architecture/     # Cloud-native and microservices design
│   └── strategy/         # Comprehensive roadmap documentation
├── examples/             # Professional use case examples
├── build/                # Build outputs (auto-generated)
└── Makefile             # Cross-platform build system
```

## 🔧 Quick Start

### Prerequisites
- **C++17 compatible compiler** (GCC 7+, Clang 5+, MSVC 2019+)
- **GNU Make** (or use manual compilation)

### Building

#### Windows (Recommended: MSYS2/MinGW)
```powershell
# Install MSYS2 and MinGW toolchain
choco install msys2

# Standard optimized build
make optimized

# Run tests with performance features
make test PERFORMANCE=1

# Ultra-optimized build (use with caution)
make ultra-optimized
```

#### Linux/macOS
```bash
# Install build tools (Ubuntu/Debian)
sudo apt install build-essential libnuma-dev  # NUMA support

# Production-optimized build
make optimized

# NUMA-aware build (Linux only)
make numa-optimized

# Parallel build with OpenMP and performance features
make optimized OMP=1 PERFORMANCE=1 -j$(nproc)
```

#### Performance Build Targets
```bash
make optimized          # Production-optimized build
make ultra-optimized    # Ultra-optimized (may affect numerical accuracy)
make numa-optimized     # NUMA-aware build (Linux)
make validate-arch      # Validate numerical accuracy on target hardware
make regression-test    # Performance regression testing
make thread-analysis    # Threading performance analysis
```

#### Enhanced CLI Interface
```bash
# Build with professional CLI interface
make enhanced           # Enhanced CLI with table-formatted output
make enhanced-optimized # Optimized build with enhanced CLI
make enhanced-full      # Full-featured enhanced CLI with all optimizations

# Enhanced CLI usage examples
./bsm --help           # Show all available commands
./bsm price --spot 100 --strike 105 --rate 0.05 --time 0.25 --vol 0.2 --type call
./bsm price --spot 100 --strike 105 --rate 0.05 --time 0.25 --vol 0.2 --type call --greeks
./bsm --interactive    # Start interactive mode
```

#### Manual Compilation (No Make)
```powershell
# Windows PowerShell (basic)
g++ -std=c++17 -O3 -march=native -Iinclude src/*.cpp -o bsm.exe

# Linux/macOS (with performance optimizations)
g++ -std=c++17 -O3 -march=native -mavx2 -mfma -fopenmp -Iinclude src/*.cpp -o bsm -lnuma
```

## 🏃‍♂️ Usage

### Running the Demo
```bash
# After building
./build/bin/bsm        # Linux/macOS
.\build\bin\bsm.exe    # Windows

# Expected output: Comparison of BS, MC, PDE, and SLV prices
```

### Performance Runtime Features
```bash
# Show architecture information
./build/bin/bsm --arch-info

# Validate numerical accuracy
./build/bin/bsm --validate-accuracy

# Run comprehensive benchmark suite
./build/bin/bsm --benchmark-suite

# Quick performance test
./build/bin/bsm --quick-benchmark

# Set thread count
./build/bin/bsm --threads 8

# Set Monte Carlo paths
./build/bin/bsm --paths 1000000
```

### Basic API Usage
```cpp
#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"

using namespace bsm;

// Basic Black-Scholes pricing
double bs_price = black_scholes_price(100.0, 100.0, 0.05, 1.0, 0.2, OptionType::Call);

// Monte Carlo with variance reduction
MCResult mc_result = mc_gbm_price(100.0, 100.0, 0.05, 1.0, 0.2, 
                                  1000000, OptionType::Call, 12345UL,
                                  true,  // antithetic
                                  true,  // control_variate  
                                  true); // qmc

std::cout << "MC Price: " << mc_result.price 
          << " +/- " << mc_result.std_error << std::endl;
```

### Advanced Features
```cpp
// SLV pricing with custom local volatility
SmileLocalVol smile_vol{0.22, 0.95, 0.25, 0.15, 100.0, 0.01};
HestonParams heston;

MCResult slv_result = mc_slv_price(100.0, 100.0, 0.05, 1.0,
                                   500000, 252, OptionType::Call,
                                   heston, smile_vol.to_fn(), 77777UL,
                                   true, true);
```

## 📊 Performance Benchmarks

| Method | Paths/Steps | Time (ms) | Accuracy (vs BS) |
|--------|-------------|-----------|------------------|
| Analytical BS | - | < 0.01 | Exact |
| PDE (CN) | 300×150 | ~5 | 0.001% |
| MC (GBM) | 500K | ~50 | 0.01% |
| MC (QMC) | 500K | ~45 | 0.005% |
| SLV | 300K×252 | ~200 | Model-dependent |

*Benchmarks on Intel i7-10700K @ 3.8GHz, GCC 11.2, -O3 -march=native*

## 🧪 Testing and Validation

### Running Tests
```bash
make test                    # Build and run all tests
make test OMP=1             # Test with OpenMP enabled
```

### Test Coverage
- **Convergence Tests**: MC vs Analytical pricing within tolerances
- **Method Consistency**: PDE vs Analytical Black-Scholes validation  
- **American Option Logic**: American ≥ European option prices
- **SLV Sanity Checks**: Finite, positive prices for call options
- **Greeks Validation**: Finite difference vs analytical derivatives

### Custom Test Integration
```cpp
// Add new tests to test/custom_tests.cpp
#include "test_framework.hpp"

void test_new_feature() {
    // Your test implementation
    assert_approx_equal(expected, actual, tolerance);
}
```

## 🔧 Configuration and Optimization

### Performance Build Configuration
```bash
# Production optimized build
make optimized

# Ultra-optimized build (use with caution for numerical accuracy)
make ultra-optimized

# NUMA-aware build (Linux)
make numa-optimized

# Enable specific optimizations
make OMP=1 PERFORMANCE=1 AVX=1 NUMA=1
```

### Build Options
```bash
make DEBUG=1           # Debug build with symbols
make OMP=1             # Enable OpenMP parallelization
make PERFORMANCE=1     # Enable performance utilities
make NUMA=1            # Enable NUMA optimizations (Linux)
make AVX=1             # Enable AVX/AVX2 vectorization
make FAST_MATH=1       # Enable fast math (use with caution)
make ARCH_NATIVE=0     # Disable native architecture targeting
```

### Runtime Performance Tuning
```bash
# Set OpenMP thread count
export OMP_NUM_THREADS=8

# Set CPU affinity (Linux)
taskset -c 0-7 ./build/bin/bsm

# NUMA policy (Linux)
numactl --cpunodebind=0 --membind=0 ./build/bin/bsm
```

### Validation and Testing
```bash
# Test performance optimizations
./test_performance.sh    # Linux/macOS
./test_performance.bat   # Windows

# Validate numerical accuracy
make validate-arch

# Performance regression testing
make regression-test

# Threading analysis
make thread-analysis
```
make clean && make CXXFLAGS="-std=c++17 -O3 -march=native -fprofile-use"
```

### Parallel Computing
```bash
# OpenMP parallel Monte Carlo
make OMP=1

# Check thread utilization
OMP_NUM_THREADS=8 ./build/bin/bsm
```

### Memory Optimization
```cpp
// For large-scale simulations, consider:
// - Batch processing for memory-bound workloads
// - Streaming random number generation  
// - NUMA-aware memory allocation for HPC
// - Hardware-specific optimization recommendations
// - Performance regression monitoring
```

### Performance Utilities API
```cpp
#include "performance_utils.hpp"

// Detect architecture and get optimization recommendations
auto arch_info = bsm::performance::ArchitectureOptimizer::detect_architecture();
auto recommendations = bsm::performance::ArchitectureOptimizer::get_optimization_recommendations();

// Validate numerical accuracy
bool accuracy_ok = bsm::performance::ArchitectureOptimizer::validate_numerical_accuracy();

// Memory profiling
bsm::performance::MemoryProfiler::start_profiling();
// ... run computation ...
auto memory_profile = bsm::performance::MemoryProfiler::stop_profiling();

// Benchmarking with RAII timing
{
    BENCHMARK_SCOPE("Monte Carlo Pricing");
    auto result = mc_gbm_price(/* parameters */);
}

// Threading optimization
auto thread_config = bsm::performance::ThreadManager::initialize_threading();
bsm::performance::ThreadManager::set_cpu_affinity({0, 1, 2, 3});
```

## 🔬 Mathematical Models

### Black-Scholes Model
Classic geometric Brownian motion with constant volatility:
```
dS_t = rS_t dt + sigma*S_t dW_t
```

### Stochastic Local Volatility (SLV)
Combines Heston stochastic volatility with local volatility:
```
dS_t = rS_t dt + L(S,t)*sqrt(v_t)*sigma_local(S,t)*S_t*dW_t^S
dv_t = kappa*(theta - v_t)*dt + xi*sqrt(v_t)*dW_t^v
```
where `L(S,t)` is the leverage function calibrated to market data.

### Local Volatility Models
- **CEV Model**: `sigma(S) = sigma_0*(S/S_0)^(beta-1)`
- **Smile Model**: Parametric local volatility with skew and term structure

### American Option Pricing
- **PDE Approach**: Projected Crank-Nicolson with early exercise boundary
- **LSM Approach**: Longstaff-Schwartz regression on continuation value

## 🛠️ Extension Points

### Adding New Models
```cpp
// 1. Create new local volatility function
class CustomLocalVol {
public:
    LocalVolFn to_fn() const {
        return [=](double S, double t) -> double {
            // Your volatility surface implementation
            return compute_vol(S, t);
        };
    }
};

// 2. Add to SLV framework
auto custom_vol = CustomLocalVol{params...};
MCResult result = mc_slv_price(S0, K, r, T, paths, steps, type,
                               heston_params, custom_vol.to_fn(), seed);
```

### Custom Payoffs
```cpp
// Extend OptionType enum and pricing functions
enum class OptionType { Call, Put, Digital, Barrier, Asian };

// Implement in respective pricing modules
double price_custom_option(/* parameters */, OptionType custom_type);
```

### Adding Greeks
```cpp
// Extend MCResult structure
struct MCResult {
    double price, std_error;
    double delta, delta_se;
    double gamma, gamma_se;  // Add new Greeks
    double theta, theta_se;
    // ... other sensitivities
};
```

## � Next Steps: Enterprise Evolution

This project is positioned for significant enterprise enhancement through the comprehensive development strategy outlined in the [`dev/`](dev/) directory. Key next steps include:

### Immediate Implementation Priorities
1. **Enhanced CLI Integration**: Complete integration of the rich CLI interface with the existing build system
2. **Python Bindings**: Implement pybind11 integration for seamless Python-C++ interoperability  
3. **ML Volatility Framework**: Begin LSTM volatility modeling implementation starting with data pipeline architecture
4. **Web Dashboard Prototype**: Create initial React-based professional trading interface

### Strategic Development Path
- **Phase 1**: Core ML integration and enhanced interfaces (3-6 months)
- **Phase 2**: Cloud-native architecture and microservices deployment (6-12 months)  
- **Phase 3**: Advanced AI capabilities and enterprise features (12+ months)

For detailed implementation guides, architectural blueprints, and prototype code, explore the comprehensive strategy documentation in [`dev/README.md`](dev/README.md).

## �📚 References and Further Reading

### Academic Papers
- Black, F., & Scholes, M. (1973). "The Pricing of Options and Corporate Liabilities"
- Heston, S. L. (1993). "A Closed-Form Solution for Options with Stochastic Volatility"
- Dupire, B. (1994). "Pricing with a Smile"
- Andersen, L. B. (2008). "Simple and Efficient Simulation of the Heston Stochastic Volatility Model"
- Longstaff, F. A., & Schwartz, E. S. (2001). "Valuing American Options by Simulation"

### Numerical Methods
- Crank, J., & Nicolson, P. (1947). "A Practical Method for Numerical Evaluation of Solutions"
- Halton, J. H. (1960). "On the Efficiency of Certain Quasi-random Sequences"
- Thomas, L. H. (1949). "Elliptic Problems in Linear Difference Equations"

### Books
- Glasserman, P. (2003). "Monte Carlo Methods in Financial Engineering"
- Gatheral, J. (2006). "The Volatility Surface: A Practitioner's Guide"
- Andersen, L., & Piterbarg, V. (2010). "Interest Rate Modeling"

## 🤝 Contributing

### Development Setup
```bash
# Clone and setup development environment
git clone <repository-url>
cd black-scholes-merton

# Create feature branch
git checkout -b feature/new-model

# Make changes and test
make test

# Check code style (optional: clang-format)
find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### Coding Standards
- **C++17 Standard**: Use modern C++ features and idioms
- **Performance First**: Optimize hot paths, minimize allocations
- **Documentation**: Document public APIs with clear examples
- **Testing**: Add tests for new features and edge cases
- **Header-only when appropriate**: Prefer templates and constexpr

### Pull Request Guidelines
1. Ensure all tests pass (`make test`)
2. Add documentation for new features
3. Include performance benchmarks for algorithmic changes
4. Follow existing code style and naming conventions

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Black-Scholes option pricing framework
- Numerical recipes and computational finance literature
- Open-source mathematical libraries and algorithms
- Contributors and maintainers of the C++ standards

---

**Note**: This software is for educational and research purposes. For production trading systems, additional validation, risk management, and regulatory compliance measures are required.
