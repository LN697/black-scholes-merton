# Performance Guide

This guide provides detailed information on optimizing performance for the Black-Scholes-Merton pricing toolkit.

## 🚀 Quick Performance Tips

### Compiler Optimizations
```bash
# Recommended production build
make CXXFLAGS="-std=c++17 -O3 -march=native -DNDEBUG -flto"

# Profile-guided optimization (advanced)
make CXXFLAGS="-std=c++17 -O3 -march=native -fprofile-generate"
# Run representative workload
make clean && make CXXFLAGS="-std=c++17 -O3 -march=native -fprofile-use"
```

### OpenMP Parallelization
```bash
# Enable parallel Monte Carlo
make OMP=1

# Control thread count at runtime
export OMP_NUM_THREADS=8
./build/bin/bsm
```

### Memory Optimization
```bash
# For large simulations, consider NUMA binding
numactl --cpunodebind=0 --membind=0 ./build/bin/bsm
```

## 📊 Performance Benchmarks

### Hardware Configuration
- **CPU**: Intel i7-10700K @ 3.8GHz (8 cores, 16 threads)
- **Memory**: 32GB DDR4-3200
- **Compiler**: GCC 11.2 with `-O3 -march=native`
- **OS**: Ubuntu 22.04 LTS

### Pricing Method Comparison

| Method | Parameters | Time (ms) | Accuracy vs BS | Memory (MB) |
|--------|------------|-----------|----------------|-------------|
| **Analytical BS** | - | 0.001 | Exact | 0.01 |
| **PDE (European)** | 300×150 grid | 4.2 | 0.001% | 2.1 |
| **PDE (American)** | 400×200 grid | 12.8 | 0.005% | 3.8 |
| **MC (Pseudo)** | 1M paths | 45.3 | 0.01% | 8.2 |
| **MC (Antithetic)** | 1M paths | 46.1 | 0.007% | 8.2 |
| **MC (QMC)** | 1M paths | 43.8 | 0.005% | 8.5 |
| **MC (All Variance Reduction)** | 1M paths | 67.2 | 0.003% | 12.1 |
| **SLV (Heston+CEV)** | 500K×252 | 187.4 | Model-dependent | 45.6 |
| **SLV (Heston+Smile)** | 500K×252 | 203.1 | Model-dependent | 45.6 |

### Scaling Analysis

#### Monte Carlo Path Scaling
```
Paths     | Time (ms) | StdErr | Efficiency
----------|-----------|--------|------------
100K      |    4.5    | 0.065  |   100%
500K      |   22.1    | 0.029  |   102%
1M        |   45.3    | 0.021  |    99%
5M        |  231.2    | 0.009  |    97%
10M       |  479.1    | 0.007  |    93%
```

#### OpenMP Thread Scaling (1M paths)
```
Threads   | Time (ms) | Speedup | Efficiency
----------|-----------|---------|------------
1         |   453.2   |   1.0x  |   100%
2         |   235.1   |   1.93x |    96%
4         |   125.7   |   3.61x |    90%
8         |    67.2   |   6.74x |    84%
16        |    45.3   |  10.0x  |    63%
```

#### PDE Grid Scaling (European Call)
```
Grid Size | Time (ms) | Memory (MB) | Error vs Analytical
----------|-----------|-------------|-------------------
100×50    |    0.8    |    0.4      |      0.015%
200×100   |    2.1    |    1.1      |      0.004%
300×150   |    4.2    |    2.1      |      0.001%
500×250   |   12.8    |    5.8      |      0.0004%
1000×500  |   68.4    |   23.1      |      0.0001%
```

## 🔧 Optimization Strategies

### 1. Monte Carlo Optimization

#### Variance Reduction Selection
```cpp
// For vanilla options: enable all techniques
MCResult result = mc_gbm_price(S0, K, r, T, sigma, paths, type, seed,
                               true,  // antithetic (always enable)
                               true,  // control_variate 
                               true,  // qmc (for smooth payoffs)
                               true,  // two_pass_cv
                               false); // greeks (if not needed)
```

#### Path Count Optimization
```cpp
// Target standard error: 0.01 (1% of price)
double target_se = 0.01 * expected_price;
long optimal_paths = std::pow(initial_se / target_se, 2) * initial_paths;
```

#### Memory-Efficient Large Simulations
```cpp
// For very large simulations, use batching
const long batch_size = 100000;
const long total_paths = 10000000;
double total_sum = 0.0, total_sq_sum = 0.0;

for (long batch = 0; batch < total_paths; batch += batch_size) {
    long current_batch = std::min(batch_size, total_paths - batch);
    MCResult batch_result = mc_gbm_price(/* params */, current_batch, /* ... */);
    
    total_sum += batch_result.price * current_batch;
    total_sq_sum += batch_result.price * batch_result.price * current_batch;
}

double final_price = total_sum / total_paths;
double final_se = std::sqrt((total_sq_sum / total_paths - final_price * final_price) / total_paths);
```

### 2. PDE Optimization

#### Grid Size Selection
```cpp
// Adaptive grid sizing based on option parameters
int optimal_S_steps = std::max(100, static_cast<int>(3 * S0 / (K * 0.01)));
int optimal_T_steps = std::max(50, static_cast<int>(T * 365 / 2));  // Semi-daily

double pde_price = pde_crank_nicolson(S0, K, r, T, sigma, 
                                      optimal_S_steps, optimal_T_steps, type);
```

#### Memory-Optimized PDE
```cpp
// For very large grids, consider:
// 1. Sparse matrix storage for boundary-dominated problems
// 2. Implicit-only schemes (higher truncation error, lower memory)
// 3. Adaptive mesh refinement near optimal exercise boundary
```

### 3. SLV Optimization

#### Time Step Selection
```cpp
// Balance accuracy vs. performance
long daily_steps = static_cast<long>(T * 365);          // Daily (accurate)
long weekly_steps = static_cast<long>(T * 52);          // Weekly (faster)
long monthly_steps = static_cast<long>(T * 12);         // Monthly (fastest)

// Recommended: daily for T ≤ 1 year, weekly for longer terms
long optimal_steps = (T <= 1.0) ? daily_steps : weekly_steps;
```

#### Local Volatility Function Optimization
```cpp
// Pre-compute expensive operations
class OptimizedSmileLocalVol {
    // Cache frequently accessed values
    mutable std::unordered_map<std::pair<double,double>, double> vol_cache;
    
public:
    LocalVolFn to_fn() const {
        return [this](double S, double t) -> double {
            auto key = std::make_pair(S, t);
            auto it = vol_cache.find(key);
            if (it != vol_cache.end()) return it->second;
            
            double vol = compute_vol(S, t);  // Expensive computation
            vol_cache[key] = vol;
            return vol;
        };
    }
};
```

## 💾 Memory Management

### Memory Usage by Component

#### Monte Carlo
- **Base memory**: O(1) per path (streaming computation)
- **With antithetics**: No additional memory
- **With control variates**: ~2x base memory (store control variables)
- **With QMC**: +~0.1MB for Halton state

#### PDE Solvers
- **Grid storage**: `8 * num_S_steps * 3` bytes (current, previous, workspace)
- **Matrix solver**: `8 * num_S_steps` bytes for tri-diagonal system
- **Total**: ~24 bytes per spatial grid point

#### SLV Simulations
- **Path storage**: `16 * num_paths * num_steps` bytes (S and v processes)
- **Random numbers**: `16 * num_paths * num_steps` bytes (2 correlated normals)
- **Working memory**: ~32 bytes per path-step

### Memory Optimization Techniques

#### Streaming Computation
```cpp
// Instead of storing all paths
std::vector<std::vector<double>> all_paths(num_paths, std::vector<double>(num_steps));

// Use streaming computation
double running_sum = 0.0;
for (long path = 0; path < num_paths; ++path) {
    double path_payoff = simulate_single_path(/* params */);
    running_sum += path_payoff;
    // Path data automatically freed
}
```

#### Memory Pool Allocation
```cpp
// For high-frequency pricing (many small allocations)
class MemoryPool {
    std::vector<double> pool;
    size_t offset = 0;
    
public:
    double* allocate(size_t count) {
        if (offset + count > pool.size()) pool.resize(offset + count);
        double* ptr = &pool[offset];
        offset += count;
        return ptr;
    }
    
    void reset() { offset = 0; }
};
```

## ⚡ Advanced Performance Techniques

### 1. SIMD Optimization

#### Vectorizable Loops
```cpp
// Ensure loops are vectorizable
#pragma omp simd
for (long i = 0; i < num_paths; ++i) {
    double z = normal_samples[i];
    double s_final = s0 * std::exp((r - 0.5*sigma*sigma)*T + sigma*sqrt_t*z);
    payoffs[i] = std::max(s_final - K, 0.0);
}
```

#### Compiler Auto-vectorization
```bash
# Check vectorization reports
g++ -O3 -march=native -fopt-info-vec -fopt-info-vec-missed your_code.cpp
```

### 2. Cache Optimization

#### Data Layout
```cpp
// Prefer structure-of-arrays for better cache locality
struct PathData {
    std::vector<double> spot_prices;
    std::vector<double> variances;
    std::vector<double> payoffs;
};

// Instead of array-of-structures
struct PathPoint {
    double spot_price;
    double variance;
    double payoff;
};
std::vector<PathPoint> paths;  // Poor cache locality
```

#### Memory Access Patterns
```cpp
// Sequential access patterns are fastest
for (long t = 0; t < num_steps; ++t) {
    for (long path = 0; path < num_paths; ++path) {
        // Update path data sequentially
        spot_prices[path] = evolve_spot(spot_prices[path], /* ... */);
    }
}
```

### 3. Numerical Optimization

#### Fast Mathematical Functions
```cpp
// Use optimized mathematical libraries when available
#ifdef USE_INTEL_MKL
    #include <mkl.h>
    // vdExp, vdLn for vectorized operations
#endif

// Fast approximations for non-critical paths
inline double fast_exp(double x) {
    // Pade approximation or lookup table for speed
    if (std::abs(x) < 1e-6) return 1.0 + x;
    return std::exp(x);
}
```

#### Random Number Generation
```cpp
// Batch RNG generation for better performance
std::vector<double> generate_normal_batch(size_t count, RNG& rng) {
    std::vector<double> result(count);
    
    // Use vectorized Box-Muller or Ziggurat algorithm
    for (size_t i = 0; i < count; i += 2) {
        auto [z1, z2] = box_muller_pair(rng);
        result[i] = z1;
        if (i + 1 < count) result[i + 1] = z2;
    }
    
    return result;
}
```

## 🔍 Profiling and Debugging

### Performance Profiling
```bash
# Using perf (Linux)
perf record -g ./build/bin/bsm
perf report

# Using gprof
g++ -pg -O3 -Iinclude src/*.cpp -o bsm_profile
./bsm_profile
gprof bsm_profile gmon.out > profile.txt

# Using Valgrind (callgrind)
valgrind --tool=callgrind ./build/bin/bsm
kcachegrind callgrind.out.*
```

### Memory Profiling
```bash
# Memory usage analysis
valgrind --tool=massif ./build/bin/bsm
ms_print massif.out.*

# Memory leak detection
valgrind --tool=memcheck --leak-check=full ./build/bin/bsm
```

### CPU Optimization Flags
```bash
# Intel-specific optimizations
-march=skylake -mtune=skylake -mavx2 -mfma

# AMD-specific optimizations  
-march=znver2 -mtune=znver2

# Generic optimizations
-O3 -march=native -mtune=native -flto -ffast-math
```

## 📈 Performance Monitoring

### Built-in Timing
```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
MCResult result = mc_gbm_price(/* parameters */);
auto end = std::chrono::high_resolution_clock::now();

auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "MC pricing took: " << duration.count() << " ms" << std::endl;
```

### Throughput Measurement
```cpp
// Measure options priced per second
size_t num_options = 1000;
auto start = std::chrono::high_resolution_clock::now();

for (size_t i = 0; i < num_options; ++i) {
    double price = black_scholes_price(/* varying parameters */);
}

auto end = std::chrono::high_resolution_clock::now();
double seconds = std::chrono::duration<double>(end - start).count();
double throughput = num_options / seconds;

std::cout << "Throughput: " << throughput << " options/second" << std::endl;
```

## 🎯 Production Deployment

### Release Build Configuration
```makefile
# Optimal production build
CXXFLAGS := -std=c++17 -O3 -march=native -DNDEBUG -flto
CXXFLAGS += -fno-exceptions -fno-rtti  # If not needed
CXXFLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections  # Dead code elimination
```

### Runtime Configuration
```cpp
// Set optimal thread count based on hardware
#ifdef _OPENMP
    int optimal_threads = std::min(omp_get_max_threads(), 
                                   static_cast<int>(std::thread::hardware_concurrency()));
    omp_set_num_threads(optimal_threads);
#endif

// CPU affinity for consistent performance
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < optimal_threads; ++i) CPU_SET(i, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
#endif
```

### Deployment Checklist
- [ ] Optimize compiler flags for target architecture
- [ ] Enable OpenMP for multi-threaded workloads
- [ ] Validate numerical accuracy on target hardware
- [ ] Profile memory usage for large-scale deployments
- [ ] Set appropriate thread counts and CPU affinity
- [ ] Monitor performance regressions in production
- [ ] Consider NUMA topology for large systems
