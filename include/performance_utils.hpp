/**
 * @file performance_utils.hpp
 * @brief Performance optimization utilities for BSM pricing toolkit
 * 
 * This header provides utilities for performance optimization including:
 * - Compiler architecture detection and optimization
 * - OpenMP thread management and CPU affinity
 * - Memory profiling and NUMA topology awareness
 * - Hardware validation and numerical accuracy testing
 * - Performance regression monitoring
 * 
 * @author LN697
 * @version 1.0
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>
#include <map>
#include <iostream>

#ifdef USE_OPENMP
#include <omp.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#include <unistd.h>
#include <numa.h>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

namespace bsm {
namespace performance {

/**
 * @brief System architecture detection and optimization info
 */
struct ArchitectureInfo {
    std::string cpu_brand;
    std::string compiler_version;
    int num_physical_cores;
    int num_logical_cores;
    int cache_line_size;
    int l1_cache_size;
    int l2_cache_size;
    int l3_cache_size;
    bool has_avx;
    bool has_avx2;
    bool has_fma;
    bool has_numa;
    int numa_nodes;                 ///< Number of NUMA nodes
    std::vector<int> cpu_topology;  ///< CPU topology per NUMA node
};

/**
 * @brief Memory usage profiling information
 */
struct MemoryProfile {
    size_t peak_memory_mb;          ///< Peak memory usage in MB
    size_t current_memory_mb;       ///< Current memory usage in MB
    size_t available_memory_mb;     ///< Available system memory in MB
    size_t cache_misses;            ///< Estimated cache misses
    double memory_bandwidth_gb_s;   ///< Memory bandwidth in GB/s
    std::vector<size_t> numa_memory_usage; ///< Memory usage per NUMA node
};

/**
 * @brief Threading configuration for optimal performance
 */
struct ThreadConfig {
    int num_threads;                ///< Optimal number of threads
    std::vector<int> cpu_affinity;  ///< CPU affinity mask
    int numa_policy;                ///< NUMA memory policy
    bool hyperthreading_enabled;    ///< Whether to use hyperthreading
    std::string scheduling_policy;  ///< Thread scheduling policy
};

/**
 * @brief Performance benchmark result
 */
struct BenchmarkResult {
    std::string test_name;          ///< Name of the benchmark test
    double execution_time_ms;       ///< Execution time in milliseconds
    double throughput;              ///< Operations per second
    double accuracy_vs_reference;   ///< Accuracy compared to reference
    size_t memory_used_mb;          ///< Memory used during test
    std::map<std::string, double> custom_metrics; ///< Additional metrics
};

/**
 * @brief Performance regression tracking
 */
struct RegressionTracker {
    std::string version;            ///< Software version
    std::chrono::system_clock::time_point timestamp; ///< Benchmark timestamp
    std::vector<BenchmarkResult> results; ///< Benchmark results
    double performance_score;       ///< Overall performance score
    std::string hardware_fingerprint; ///< Hardware configuration hash
};

/**
 * @brief Architecture detection and optimization utilities
 */
class ArchitectureOptimizer {
public:
    /**
     * @brief Detect system architecture and capabilities
     */
    static ArchitectureInfo detect_architecture();

    /**
     * @brief Get optimal compiler flags for current architecture
     */
    static std::vector<std::string> get_optimal_compiler_flags();

    /**
     * @brief Validate numerical accuracy on current hardware
     */
    static bool validate_numerical_accuracy(double tolerance = 1e-14);

    /**
     * @brief Get architecture-specific optimization recommendations
     */
    static std::vector<std::string> get_optimization_recommendations();

private:
    static std::string detect_cpu_brand();
    static void detect_cache_info(ArchitectureInfo& info);
    static void detect_instruction_sets(ArchitectureInfo& info);
    static void detect_numa_topology(ArchitectureInfo& info);
};

/**
 * @brief OpenMP thread management and optimization
 */
class ThreadManager {
public:
    /**
     * @brief Initialize optimal threading configuration
     */
    static ThreadConfig initialize_threading();

    /**
     * @brief Set CPU affinity for current process
     */
    static bool set_cpu_affinity(const std::vector<int>& cpu_list);

    /**
     * @brief Set NUMA memory policy
     */
    static bool set_numa_policy(int policy, const std::vector<int>& nodes = {});

    /**
     * @brief Get optimal thread count for workload type
     */
    static int get_optimal_thread_count(const std::string& workload_type = "compute");

    /**
     * @brief Configure OpenMP runtime for optimal performance
     */
    static void configure_openmp();

    /**
     * @brief Monitor thread performance and detect contention
     */
    static std::map<std::string, double> monitor_thread_performance();

private:
    static bool is_hyperthreading_beneficial(const std::string& workload_type);
    static std::vector<int> get_optimal_cpu_affinity();
};

/**
 * @brief Memory profiling and optimization utilities
 */
class MemoryProfiler {
public:
    /**
     * @brief Start memory profiling session
     */
    static void start_profiling();

    /**
     * @brief Stop profiling and get results
     */
    static MemoryProfile stop_profiling();

    /**
     * @brief Get current memory usage
     */
    static MemoryProfile get_current_usage();

    /**
     * @brief Estimate memory requirements for workload
     */
    static size_t estimate_memory_requirement(
        const std::string& method, 
        const std::map<std::string, double>& parameters
    );

    /**
     * @brief Optimize memory allocation strategy
     */
    static void configure_memory_allocation();

    /**
     * @brief Check for memory leaks
     */
    static std::vector<std::string> detect_memory_leaks();

private:
    static size_t get_peak_memory_usage();
    static double estimate_cache_performance();
    static void configure_numa_allocation();
};

/**
 * @brief Performance benchmarking and regression tracking
 */
class PerformanceBenchmark {
public:
    /**
     * @brief Run comprehensive performance benchmark
     */
    static std::vector<BenchmarkResult> run_benchmark_suite();

    /**
     * @brief Run specific benchmark test
     */
    static BenchmarkResult run_benchmark(
        const std::string& test_name,
        std::function<void()> test_function,
        int iterations = 5
    );

    /**
     * @brief Compare performance against baseline
     */
    static double compare_to_baseline(
        const std::vector<BenchmarkResult>& current,
        const std::vector<BenchmarkResult>& baseline
    );

    /**
     * @brief Save benchmark results for regression tracking
     */
    static void save_benchmark_results(
        const std::vector<BenchmarkResult>& results,
        const std::string& filename = "benchmark_results.json"
    );

    /**
     * @brief Load historical benchmark results
     */
    static std::vector<RegressionTracker> load_benchmark_history(
        const std::string& filename = "benchmark_results.json"
    );

    /**
     * @brief Detect performance regressions
     */
    static std::vector<std::string> detect_regressions(
        const std::vector<BenchmarkResult>& current,
        const std::vector<RegressionTracker>& history,
        double threshold = 0.05  // 5% regression threshold
    );

private:
    static std::string generate_hardware_fingerprint();
    static double calculate_performance_score(const std::vector<BenchmarkResult>& results);
};

/**
 * @brief High-precision timer for performance measurement
 */
class HighPrecisionTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point end_time_;
    bool is_running_;

public:
    HighPrecisionTimer() : is_running_(false) {}

    /**
     * @brief Start timer
     */
    void start() {
        start_time_ = std::chrono::high_resolution_clock::now();
        is_running_ = true;
    }

    /**
     * @brief Stop timer
     */
    void stop() {
        end_time_ = std::chrono::high_resolution_clock::now();
        is_running_ = false;
    }

    /**
     * @brief Get elapsed time in milliseconds
     */
    double elapsed_ms() const {
        auto end = is_running_ ? std::chrono::high_resolution_clock::now() : end_time_;
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time_);
        return duration.count() / 1000.0;
    }

    /**
     * @brief Get elapsed time in microseconds
     */
    double elapsed_us() const {
        auto end = is_running_ ? std::chrono::high_resolution_clock::now() : end_time_;
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time_);
        return static_cast<double>(duration.count());
    }

    /**
     * @brief Get elapsed time in nanoseconds
     */
    long long elapsed_ns() const {
        auto end = is_running_ ? std::chrono::high_resolution_clock::now() : end_time_;
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_time_);
        return duration.count();
    }
};

/**
 * @brief RAII benchmark timer that automatically measures execution time
 */
class ScopedBenchmark {
private:
    HighPrecisionTimer timer_;
    std::string name_;
    std::function<void(const std::string&, double)> callback_;

public:
    ScopedBenchmark(const std::string& name, 
                   std::function<void(const std::string&, double)> callback = nullptr)
        : name_(name), callback_(callback) {
        timer_.start();
    }

    ~ScopedBenchmark() {
        timer_.stop();
        double elapsed = timer_.elapsed_ms();
        if (callback_) {
            callback_(name_, elapsed);
        } else {
            std::cout << "Benchmark [" << name_ << "]: " << elapsed << " ms" << std::endl;
        }
    }
};

/**
 * @brief Macro for easy benchmark timing
 */
#define BENCHMARK_SCOPE(name) \
    bsm::performance::ScopedBenchmark _benchmark_##__LINE__(name)

#define BENCHMARK_SCOPE_CALLBACK(name, callback) \
    bsm::performance::ScopedBenchmark _benchmark_##__LINE__(name, callback)

} // namespace performance
} // namespace bsm
