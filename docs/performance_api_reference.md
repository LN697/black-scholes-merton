# Performance Utilities API Reference

This document provides detailed API reference for the performance optimization utilities in the Black-Scholes-Merton Pricing Toolkit.

## Table of Contents

1. [Architecture Optimizer](#architecture-optimizer)
2. [Thread Manager](#thread-manager)
3. [Memory Profiler](#memory-profiler)
4. [Performance Benchmark](#performance-benchmark)
5. [High Precision Timer](#high-precision-timer)
6. [Usage Examples](#usage-examples)

## Architecture Optimizer

The `ArchitectureOptimizer` class provides automatic hardware detection and optimization recommendations.

### Header
```cpp
#include "performance_utils.hpp"
```

### Classes and Structures

#### `ArchitectureInfo`
Structure containing system architecture information.

```cpp
struct ArchitectureInfo {
    std::string cpu_brand;           // CPU brand and model
    std::string compiler_version;    // Compiler version used
    int num_physical_cores;         // Number of physical CPU cores
    int num_logical_cores;          // Number of logical CPU cores (with HT)
    int cache_line_size;            // CPU cache line size in bytes
    int l1_cache_size;              // L1 cache size in KB
    int l2_cache_size;              // L2 cache size in KB
    int l3_cache_size;              // L3 cache size in KB
    bool has_avx;                   // AVX instruction set support
    bool has_avx2;                  // AVX2 instruction set support
    bool has_fma;                   // FMA instruction set support
    bool has_numa;                  // NUMA architecture support
    int numa_nodes;                 // Number of NUMA nodes
    std::vector<int> cpu_topology;  // CPU topology per NUMA node
};
```

#### `ArchitectureOptimizer` Methods

##### `detect_architecture()`
```cpp
static ArchitectureInfo detect_architecture();
```
Detects the current system architecture and returns detailed information.

**Returns:** `ArchitectureInfo` structure with system details.

**Example:**
```cpp
auto arch = bsm::performance::ArchitectureOptimizer::detect_architecture();
std::cout << "CPU: " << arch.cpu_brand << std::endl;
std::cout << "Cores: " << arch.num_physical_cores << " physical" << std::endl;
```

##### `get_optimal_compiler_flags()`
```cpp
static std::vector<std::string> get_optimal_compiler_flags();
```
Returns optimal compiler flags for the current architecture.

**Returns:** Vector of recommended compiler flags.

**Example:**
```cpp
auto flags = bsm::performance::ArchitectureOptimizer::get_optimal_compiler_flags();
for (const auto& flag : flags) {
    std::cout << flag << " ";
}
```

##### `validate_numerical_accuracy()`
```cpp
static bool validate_numerical_accuracy(double tolerance = 1e-14);
```
Validates numerical accuracy on the current hardware configuration.

**Parameters:**
- `tolerance`: Acceptable numerical error tolerance

**Returns:** `true` if numerical accuracy is acceptable, `false` otherwise.

**Example:**
```cpp
bool accuracy_ok = bsm::performance::ArchitectureOptimizer::validate_numerical_accuracy();
if (!accuracy_ok) {
    std::cerr << "Numerical accuracy issues detected!" << std::endl;
}
```

##### `get_optimization_recommendations()`
```cpp
static std::vector<std::string> get_optimization_recommendations();
```
Returns architecture-specific optimization recommendations.

**Returns:** Vector of optimization recommendations.

**Example:**
```cpp
auto recommendations = bsm::performance::ArchitectureOptimizer::get_optimization_recommendations();
for (const auto& rec : recommendations) {
    std::cout << "- " << rec << std::endl;
}
```

## Thread Manager

The `ThreadManager` class handles OpenMP thread configuration and CPU affinity management.

### Classes and Structures

#### `ThreadConfig`
Structure containing optimal threading configuration.

```cpp
struct ThreadConfig {
    int num_threads;                // Optimal number of threads
    std::vector<int> cpu_affinity;  // CPU affinity mask
    int numa_policy;                // NUMA memory policy
    bool hyperthreading_enabled;    // Whether to use hyperthreading
    std::string scheduling_policy;  // Thread scheduling policy
};
```

#### `ThreadManager` Methods

##### `initialize_threading()`
```cpp
static ThreadConfig initialize_threading();
```
Initializes optimal threading configuration and configures OpenMP.

**Returns:** `ThreadConfig` with optimal settings.

**Example:**
```cpp
auto config = bsm::performance::ThreadManager::initialize_threading();
std::cout << "Optimal threads: " << config.num_threads << std::endl;
```

##### `set_cpu_affinity()`
```cpp
static bool set_cpu_affinity(const std::vector<int>& cpu_list);
```
Sets CPU affinity for the current process.

**Parameters:**
- `cpu_list`: List of CPU cores to bind to

**Returns:** `true` if successful, `false` otherwise.

**Example:**
```cpp
std::vector<int> cpus = {0, 1, 2, 3};
bool success = bsm::performance::ThreadManager::set_cpu_affinity(cpus);
```

##### `set_numa_policy()`
```cpp
static bool set_numa_policy(int policy, const std::vector<int>& nodes = {});
```
Sets NUMA memory allocation policy (Linux only).

**Parameters:**
- `policy`: NUMA policy type
- `nodes`: NUMA nodes to bind to (optional)

**Returns:** `true` if successful, `false` otherwise.

##### `get_optimal_thread_count()`
```cpp
static int get_optimal_thread_count(const std::string& workload_type = "compute");
```
Gets optimal thread count for specific workload type.

**Parameters:**
- `workload_type`: Type of workload ("compute", "memory", "io", "mixed")

**Returns:** Optimal number of threads.

**Example:**
```cpp
int compute_threads = bsm::performance::ThreadManager::get_optimal_thread_count("compute");
int memory_threads = bsm::performance::ThreadManager::get_optimal_thread_count("memory");
```

##### `monitor_thread_performance()`
```cpp
static std::map<std::string, double> monitor_thread_performance();
```
Monitors current thread performance metrics.

**Returns:** Map of performance metrics.

## Memory Profiler

The `MemoryProfiler` class provides memory usage monitoring and optimization.

### Classes and Structures

#### `MemoryProfile`
Structure containing memory usage information.

```cpp
struct MemoryProfile {
    size_t peak_memory_mb;          // Peak memory usage in MB
    size_t current_memory_mb;       // Current memory usage in MB
    size_t available_memory_mb;     // Available system memory in MB
    size_t cache_misses;            // Estimated cache misses
    double memory_bandwidth_gb_s;   // Memory bandwidth in GB/s
    std::vector<size_t> numa_memory_usage; // Memory usage per NUMA node
};
```

#### `MemoryProfiler` Methods

##### `start_profiling()`
```cpp
static void start_profiling();
```
Starts memory profiling session.

##### `stop_profiling()`
```cpp
static MemoryProfile stop_profiling();
```
Stops profiling and returns memory usage profile.

**Returns:** `MemoryProfile` with usage statistics.

**Example:**
```cpp
bsm::performance::MemoryProfiler::start_profiling();
// ... run computation ...
auto profile = bsm::performance::MemoryProfiler::stop_profiling();
std::cout << "Peak memory: " << profile.peak_memory_mb << " MB" << std::endl;
```

##### `get_current_usage()`
```cpp
static MemoryProfile get_current_usage();
```
Gets current memory usage without stopping profiling.

##### `estimate_memory_requirement()`
```cpp
static size_t estimate_memory_requirement(
    const std::string& method, 
    const std::map<std::string, double>& parameters
);
```
Estimates memory requirements for a computation.

**Parameters:**
- `method`: Computation method ("monte_carlo", "pde", "slv")
- `parameters`: Method-specific parameters

**Returns:** Estimated memory requirement in MB.

**Example:**
```cpp
std::map<std::string, double> params = {
    {"paths", 1000000},
    {"control_variate", 1}
};
size_t estimated = bsm::performance::MemoryProfiler::estimate_memory_requirement(
    "monte_carlo", params);
std::cout << "Estimated memory: " << estimated << " MB" << std::endl;
```

## Performance Benchmark

The `PerformanceBenchmark` class provides benchmarking and regression detection.

### Classes and Structures

#### `BenchmarkResult`
Structure containing benchmark results.

```cpp
struct BenchmarkResult {
    std::string test_name;          // Name of the benchmark test
    double execution_time_ms;       // Execution time in milliseconds
    double throughput;              // Operations per second
    double accuracy_vs_reference;   // Accuracy compared to reference
    size_t memory_used_mb;          // Memory used during test
    std::map<std::string, double> custom_metrics; // Additional metrics
};
```

#### `PerformanceBenchmark` Methods

##### `run_benchmark_suite()`
```cpp
static std::vector<BenchmarkResult> run_benchmark_suite();
```
Runs comprehensive performance benchmark suite.

**Returns:** Vector of benchmark results.

##### `run_benchmark()`
```cpp
static BenchmarkResult run_benchmark(
    const std::string& test_name,
    std::function<void()> test_function,
    int iterations = 5
);
```
Runs a specific benchmark test.

**Parameters:**
- `test_name`: Name of the test
- `test_function`: Function to benchmark
- `iterations`: Number of iterations to run

**Returns:** `BenchmarkResult` with timing and performance metrics.

**Example:**
```cpp
auto result = bsm::performance::PerformanceBenchmark::run_benchmark(
    "Black-Scholes Pricing",
    []() {
        double price = black_scholes_price(100, 100, 0.05, 1.0, 0.2, OptionType::Call);
        volatile double sink = price;  // Prevent optimization
    },
    10
);
std::cout << "Execution time: " << result.execution_time_ms << " ms" << std::endl;
```

##### `save_benchmark_results()`
```cpp
static void save_benchmark_results(
    const std::vector<BenchmarkResult>& results,
    const std::string& filename = "benchmark_results.json"
);
```
Saves benchmark results to file for regression tracking.

##### `detect_regressions()`
```cpp
static std::vector<std::string> detect_regressions(
    const std::vector<BenchmarkResult>& current,
    const std::vector<RegressionTracker>& history,
    double threshold = 0.05
);
```
Detects performance regressions compared to historical data.

**Parameters:**
- `current`: Current benchmark results
- `history`: Historical benchmark data
- `threshold`: Regression threshold (default 5%)

**Returns:** Vector of detected regressions.

## High Precision Timer

The `HighPrecisionTimer` class provides high-resolution timing capabilities.

### Methods

##### `start()`
```cpp
void start();
```
Starts the timer.

##### `stop()`
```cpp
void stop();
```
Stops the timer.

##### `elapsed_ms()`
```cpp
double elapsed_ms() const;
```
Returns elapsed time in milliseconds.

##### `elapsed_us()`
```cpp
double elapsed_us() const;
```
Returns elapsed time in microseconds.

##### `elapsed_ns()`
```cpp
long long elapsed_ns() const;
```
Returns elapsed time in nanoseconds.

**Example:**
```cpp
bsm::performance::HighPrecisionTimer timer;
timer.start();
// ... computation ...
timer.stop();
std::cout << "Elapsed: " << timer.elapsed_ms() << " ms" << std::endl;
```

### Scoped Benchmarking

#### `BENCHMARK_SCOPE(name)`
Macro for automatic timing with RAII.

```cpp
{
    BENCHMARK_SCOPE("Monte Carlo Pricing");
    auto result = mc_gbm_price(/* parameters */);
    // Timing automatically printed when scope exits
}
```

#### `BENCHMARK_SCOPE_CALLBACK(name, callback)`
Macro for automatic timing with custom callback.

```cpp
{
    BENCHMARK_SCOPE_CALLBACK("PDE Pricing", [](const std::string& name, double time) {
        std::cout << "Custom: " << name << " took " << time << " ms" << std::endl;
    });
    auto result = pde_crank_nicolson(/* parameters */);
}
```

## Usage Examples

### Complete Performance Analysis
```cpp
#include "performance_utils.hpp"
#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"

int main() {
    // 1. Detect architecture
    auto arch_info = bsm::performance::ArchitectureOptimizer::detect_architecture();
    std::cout << "Running on: " << arch_info.cpu_brand << std::endl;
    
    // 2. Validate numerical accuracy
    if (!bsm::performance::ArchitectureOptimizer::validate_numerical_accuracy()) {
        std::cerr << "Warning: Numerical accuracy issues detected!" << std::endl;
        return 1;
    }
    
    // 3. Initialize optimal threading
    auto thread_config = bsm::performance::ThreadManager::initialize_threading();
    std::cout << "Using " << thread_config.num_threads << " threads" << std::endl;
    
    // 4. Set CPU affinity for consistent performance
    bsm::performance::ThreadManager::set_cpu_affinity({0, 1, 2, 3});
    
    // 5. Start memory profiling
    bsm::performance::MemoryProfiler::start_profiling();
    
    // 6. Run benchmarks with scoped timing
    std::vector<bsm::performance::BenchmarkResult> results;
    
    {
        BENCHMARK_SCOPE("Analytical Black-Scholes");
        double price = black_scholes_price(100, 100, 0.05, 1.0, 0.2, OptionType::Call);
        volatile double sink = price;
    }
    
    {
        BENCHMARK_SCOPE("Monte Carlo");
        auto mc_result = mc_gbm_price(100, 100, 0.05, 1.0, 0.2, 100000, 
                                      OptionType::Call, 12345UL);
        volatile double sink = mc_result.price;
    }
    
    // 7. Get memory usage
    auto memory_profile = bsm::performance::MemoryProfiler::stop_profiling();
    std::cout << "Peak memory usage: " << memory_profile.peak_memory_mb << " MB" << std::endl;
    std::cout << "Memory bandwidth: " << memory_profile.memory_bandwidth_gb_s << " GB/s" << std::endl;
    
    // 8. Run comprehensive benchmark suite
    auto benchmark_results = bsm::performance::PerformanceBenchmark::run_benchmark_suite();
    
    // 9. Save results for regression tracking
    bsm::performance::PerformanceBenchmark::save_benchmark_results(benchmark_results);
    
    return 0;
}
```

### Production Deployment Checklist
```cpp
// 1. Architecture validation
auto arch_info = bsm::performance::ArchitectureOptimizer::detect_architecture();
auto recommendations = bsm::performance::ArchitectureOptimizer::get_optimization_recommendations();

// 2. Numerical accuracy validation
assert(bsm::performance::ArchitectureOptimizer::validate_numerical_accuracy());

// 3. Memory requirement estimation
std::map<std::string, double> params = {{"paths", 10000000}};
size_t required_mb = bsm::performance::MemoryProfiler::estimate_memory_requirement("monte_carlo", params);

// 4. Threading configuration
auto thread_config = bsm::performance::ThreadManager::initialize_threading();
bsm::performance::ThreadManager::set_cpu_affinity(thread_config.cpu_affinity);

// 5. NUMA optimization (Linux)
#ifdef __linux__
if (arch_info.has_numa) {
    bsm::performance::ThreadManager::set_numa_policy(0, {0});
}
#endif

// 6. Performance baseline establishment
auto baseline_results = bsm::performance::PerformanceBenchmark::run_benchmark_suite();
bsm::performance::PerformanceBenchmark::save_benchmark_results(baseline_results, "production_baseline.json");
```

This API reference provides comprehensive documentation for all performance optimization utilities. For additional examples and best practices, see the [Performance Guide](performance_guide.md).
