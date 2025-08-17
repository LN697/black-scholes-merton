/**
 * @file performance_utils.cpp
 * @brief Implementation of performance optimization utilities
 * 
 * @author LN697
 * @version 1.0
 */

#include "performance_utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <regex>
#include <thread>
#include <random>
#include <numeric>
#include <iterator>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

#ifdef _WIN32
#include <intrin.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#elif defined(__linux__)
#include <sys/utsname.h>
#include <cpuid.h>
#include <sched.h>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

namespace bsm {
namespace performance {

static std::chrono::high_resolution_clock::time_point profiling_start;
static size_t initial_memory = 0;
static bool profiling_active = false;

ArchitectureInfo ArchitectureOptimizer::detect_architecture() {
    ArchitectureInfo info = {};
    
    info.cpu_brand = detect_cpu_brand();
    
    info.num_physical_cores = std::thread::hardware_concurrency();
    info.num_logical_cores = info.num_physical_cores;
    
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    info.num_logical_cores = sysinfo.dwNumberOfProcessors;
    
    DWORD length = 0;
    GetLogicalProcessorInformation(nullptr, &length);
    if (length > 0) {
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
            length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(buffer.data(), &length)) {
            info.num_physical_cores = 0;
            for (const auto& proc_info : buffer) {
                if (proc_info.Relationship == RelationProcessorCore) {
                    info.num_physical_cores++;
                }
            }
        }
    }
#elif defined(__linux__)
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    int physical_cores = 0;
    while (std::getline(cpuinfo, line)) {
        if (line.find("cpu cores") != std::string::npos) {
            std::istringstream iss(line);
            std::string token;
            while (iss >> token) {
                if (std::isdigit(token[0])) {
                    physical_cores = std::stoi(token);
                    break;
                }
            }
            break;
        }
    }
    if (physical_cores > 0) {
        info.num_physical_cores = physical_cores;
    }
#endif

    detect_cache_info(info);
    detect_instruction_sets(info);
    detect_numa_topology(info);
    
#ifdef __GNUC__
    info.compiler_version = "GCC " + std::to_string(__GNUC__) + "." + 
                           std::to_string(__GNUC_MINOR__) + "." + 
                           std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    info.compiler_version = "MSVC " + std::to_string(_MSC_VER);
#elif defined(__clang__)
    info.compiler_version = "Clang " + std::to_string(__clang_major__) + "." + 
                           std::to_string(__clang_minor__);
#else
    info.compiler_version = "Unknown";
#endif

    return info;
}

std::vector<std::string> ArchitectureOptimizer::get_optimal_compiler_flags() {
    std::vector<std::string> flags;
    
    flags.push_back("-O3");
    flags.push_back("-DNDEBUG");
    flags.push_back("-flto");
    
    auto arch_info = detect_architecture();
    
    flags.push_back("-march=native");
    flags.push_back("-mtune=native");
    
    if (arch_info.has_avx2) {
        flags.push_back("-mavx2");
        flags.push_back("-mfma");
    } else if (arch_info.has_avx) {
        flags.push_back("-mavx");
    }
    
    flags.push_back("-ffast-math");
    flags.push_back("-funroll-loops");
    flags.push_back("-fprefetch-loop-arrays");
    flags.push_back("-fomit-frame-pointer");
    
#ifdef USE_OPENMP
    flags.push_back("-fopenmp");
#endif
    
    return flags;
}

bool ArchitectureOptimizer::validate_numerical_accuracy(double tolerance) {
    const double test_values[] = {1.0, 1e-10, 1e10, M_PI, M_E};
    const size_t num_tests = sizeof(test_values) / sizeof(test_values[0]);
    
    for (size_t i = 0; i < num_tests; ++i) {
        double x = test_values[i];
        
        double sum = x + x;
        double expected_sum = 2.0 * x;
        if (std::abs(sum - expected_sum) > tolerance * expected_sum) {
            return false;
        }
        
        if (x > 0) {
            double log_exp = std::log(std::exp(x));
            if (std::abs(log_exp - x) > tolerance * std::abs(x)) {
                return false;
            }
        }
        
        double sin_asin = std::sin(std::asin(std::min(x, 1.0)));
        double expected = std::min(x, 1.0);
        if (std::abs(sin_asin - expected) > tolerance * std::abs(expected)) {
            return false;
        }
    }
    
    double S = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    
    double d1 = (std::log(S/K) + (r + 0.5*sigma*sigma)*T) / (sigma*std::sqrt(T));
    
    double S_perturbed = S * (1.0 + 1e-12);
    double d1_perturbed = (std::log(S_perturbed/K) + (r + 0.5*sigma*sigma)*T) / (sigma*std::sqrt(T));
    
    double relative_change = std::abs(d1_perturbed - d1) / std::abs(d1);
    if (relative_change > 1e-10) {
        return false;
    }
    
    return true;
}

std::vector<std::string> ArchitectureOptimizer::get_optimization_recommendations() {
    std::vector<std::string> recommendations;
    auto arch_info = detect_architecture();
    
    // Thread recommendations
    if (arch_info.num_logical_cores > arch_info.num_physical_cores) {
        recommendations.push_back("Enable hyperthreading for memory-bound workloads");
        recommendations.push_back("Disable hyperthreading for CPU-intensive workloads");
    }
    
    // NUMA recommendations
    if (arch_info.has_numa && arch_info.numa_nodes > 1) {
        recommendations.push_back("Use NUMA-aware memory allocation for large datasets");
        recommendations.push_back("Consider thread affinity to minimize NUMA effects");
    }
    
    // Vectorization recommendations
    if (arch_info.has_avx2) {
        recommendations.push_back("Use AVX2 vectorization for array operations");
    }
    if (arch_info.has_fma) {
        recommendations.push_back("Enable FMA instructions for floating-point operations");
    }
    
    // Cache recommendations
    if (arch_info.l3_cache_size > 0) {
        int optimal_problem_size = arch_info.l3_cache_size * 1024 / 8; // Assume 8-byte doubles
        recommendations.push_back("Optimal problem size for cache efficiency: ~" + 
                                 std::to_string(optimal_problem_size) + " elements");
    }
    
    return recommendations;
}

std::string ArchitectureOptimizer::detect_cpu_brand() {
#ifdef _WIN32
    int cpu_info[4];
    char cpu_brand[0x40];
    __cpuid(cpu_info, 0x80000002);
    memcpy(cpu_brand, cpu_info, sizeof(cpu_info));
    __cpuid(cpu_info, 0x80000003);
    memcpy(cpu_brand + 16, cpu_info, sizeof(cpu_info));
    __cpuid(cpu_info, 0x80000004);
    memcpy(cpu_brand + 32, cpu_info, sizeof(cpu_info));
    return std::string(cpu_brand);
#elif defined(__linux__)
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                return line.substr(pos + 2);
            }
        }
    }
    return "Unknown CPU";
#else
    return "Unknown CPU";
#endif
}

void ArchitectureOptimizer::detect_cache_info(ArchitectureInfo& info) {
    info.cache_line_size = 64;
    info.l1_cache_size = 32;
    info.l2_cache_size = 256;
    info.l3_cache_size = 8192;

#ifdef _WIN32
    DWORD buffer_size = 0;
    GetLogicalProcessorInformation(nullptr, &buffer_size);
    if (buffer_size > 0) {
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
            buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(buffer.data(), &buffer_size)) {
            for (const auto& proc_info : buffer) {
                if (proc_info.Relationship == RelationCache) {
                    const auto& cache = proc_info.Cache;
                    switch (cache.Level) {
                        case 1:
                            info.l1_cache_size = cache.Size / 1024;
                            info.cache_line_size = cache.LineSize;
                            break;
                        case 2:
                            info.l2_cache_size = cache.Size / 1024;
                            break;
                        case 3:
                            info.l3_cache_size = cache.Size / 1024;
                            break;
                    }
                }
            }
        }
    }
#elif defined(__linux__)
    auto read_cache_size = [](const std::string& path) -> int {
        std::ifstream file(path);
        std::string value;
        if (std::getline(file, value)) {
            if (!value.empty() && value.back() == 'K') {
                value.pop_back();
            }
            return std::stoi(value);
        }
        return 0;
    };
    
    int l1_size = read_cache_size("/sys/devices/system/cpu/cpu0/cache/index0/size");
    int l2_size = read_cache_size("/sys/devices/system/cpu/cpu0/cache/index2/size");
    int l3_size = read_cache_size("/sys/devices/system/cpu/cpu0/cache/index3/size");
    
    if (l1_size > 0) info.l1_cache_size = l1_size;
    if (l2_size > 0) info.l2_cache_size = l2_size;
    if (l3_size > 0) info.l3_cache_size = l3_size;
#endif
}

void ArchitectureOptimizer::detect_instruction_sets(ArchitectureInfo& info) {
    info.has_avx = false;
    info.has_avx2 = false;
    info.has_fma = false;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    int cpu_info[4];
    
#ifdef _WIN32
    __cpuid(cpu_info, 1);
    info.has_avx = (cpu_info[2] & (1 << 28)) != 0;
    info.has_fma = (cpu_info[2] & (1 << 12)) != 0;
    
    __cpuid(cpu_info, 7);
    info.has_avx2 = (cpu_info[1] & (1 << 5)) != 0;
#elif defined(__GNUC__)
    unsigned int eax, ebx, ecx, edx;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    info.has_avx = (ecx & (1 << 28)) != 0;
    info.has_fma = (ecx & (1 << 12)) != 0;
    
    __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx);
    info.has_avx2 = (ebx & (1 << 5)) != 0;
#endif
#endif
}

void ArchitectureOptimizer::detect_numa_topology(ArchitectureInfo& info) {
    info.has_numa = false;
    info.numa_nodes = 1;
    info.cpu_topology.clear();

#ifdef __linux__
    // Check if NUMA is available
    if (numa_available() >= 0) {
        info.has_numa = true;
        info.numa_nodes = numa_max_node() + 1;
        
        for (int node = 0; node < info.numa_nodes; ++node) {
            info.cpu_topology.push_back(numa_node_size(node, nullptr));
        }
    }
#elif defined(_WIN32)
    // Windows NUMA detection (simplified)
    DWORD highest_node;
    if (GetNumaHighestNodeNumber(&highest_node)) {
        info.numa_nodes = highest_node + 1;
        info.has_numa = info.numa_nodes > 1;
        
        for (DWORD node = 0; node <= highest_node; ++node) {
            ULONGLONG mask;
            if (GetNumaNodeProcessorMask(node, &mask)) {
                info.cpu_topology.push_back(__popcnt64(mask));
            }
        }
    }
#endif
}

ThreadConfig ThreadManager::initialize_threading() {
    ThreadConfig config = {};
    auto arch_info = ArchitectureOptimizer::detect_architecture();
    
    // Default to physical cores for CPU-intensive work
    config.num_threads = arch_info.num_physical_cores;
    config.hyperthreading_enabled = false;
    config.numa_policy = 0; // Default policy
    config.scheduling_policy = "normal";
    
    // Get optimal CPU affinity
    config.cpu_affinity = get_optimal_cpu_affinity();
    
    // Configure OpenMP
    configure_openmp();
    
    return config;
}

bool ThreadManager::set_cpu_affinity(const std::vector<int>& cpu_list) {
#ifdef _WIN32
    DWORD_PTR mask = 0;
    for (int cpu : cpu_list) {
        if (cpu >= 0 && cpu < 64) {
            mask |= (1ULL << cpu);
        }
    }
    return SetProcessAffinityMask(GetCurrentProcess(), mask) != 0;
#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int cpu : cpu_list) {
        CPU_SET(cpu, &cpuset);
    }
    return sched_setaffinity(0, sizeof(cpuset), &cpuset) == 0;
#else
    return true; // Not implemented, but don't fail
#endif
}

bool ThreadManager::set_numa_policy(int policy, const std::vector<int>& nodes) {
#ifdef __linux__
    if (numa_available() < 0) return false;
    
    if (nodes.empty()) {
        return numa_set_localalloc() == 0;
    } else {
        struct bitmask* nodemask = numa_allocate_nodemask();
        for (int node : nodes) {
            numa_bitmask_setbit(nodemask, node);
        }
        int result = numa_set_membind(nodemask);
        numa_free_nodemask(nodemask);
        return result == 0;
    }
#elif defined(_WIN32)
    // Windows NUMA implementation
    (void)policy;  // Suppress unused parameter warning
    HANDLE process = GetCurrentProcess();
    
    if (nodes.empty()) {
        // Set local allocation policy (prefer local node)
        return SetProcessAffinityMask(process, GetProcessAffinityMask(process, nullptr, nullptr)) != 0;
    } else {
        // Set specific NUMA node affinity
        GROUP_AFFINITY affinity = {};
        USHORT node_number = static_cast<USHORT>(nodes[0]); // Use first node
        
        if (GetNumaNodeProcessorMaskEx(node_number, &affinity)) {
            return SetThreadGroupAffinity(GetCurrentThread(), &affinity, nullptr) != 0;
        }
        return false;
    }
#else
    (void)policy;  // Suppress unused parameter warning
    (void)nodes;   // Suppress unused parameter warning
    return true; // Not implemented for this platform, but don't fail
#endif
}

int ThreadManager::get_optimal_thread_count(const std::string& workload_type) {
    auto arch_info = ArchitectureOptimizer::detect_architecture();
    
    if (workload_type == "compute" || workload_type == "cpu") {
        // CPU-intensive: use physical cores
        return arch_info.num_physical_cores;
    } else if (workload_type == "memory" || workload_type == "io") {
        // Memory/IO bound: can benefit from hyperthreading
        return arch_info.num_logical_cores;
    } else if (workload_type == "mixed") {
        // Mixed workload: compromise between the two
        return (arch_info.num_physical_cores + arch_info.num_logical_cores) / 2;
    }
    
    return arch_info.num_physical_cores;
}

void ThreadManager::configure_openmp() {
#ifdef USE_OPENMP
    // Set dynamic thread adjustment
    omp_set_dynamic(1);
    
    // Set nested parallelism (usually disabled for better performance)
    omp_set_nested(0);
    
    // Set thread scheduling
    omp_set_schedule(omp_sched_guided, 0);
    
    // Set default number of threads
    auto arch_info = ArchitectureOptimizer::detect_architecture();
    omp_set_num_threads(arch_info.num_physical_cores);
#endif
}

std::map<std::string, double> ThreadManager::monitor_thread_performance() {
    std::map<std::string, double> metrics;
    
#ifdef USE_OPENMP
    metrics["max_threads"] = omp_get_max_threads();
    metrics["num_procs"] = omp_get_num_procs();
    metrics["dynamic"] = omp_get_dynamic() ? 1.0 : 0.0;
    metrics["nested"] = omp_get_nested() ? 1.0 : 0.0;
#endif
    
    auto arch_info = ArchitectureOptimizer::detect_architecture();
    metrics["physical_cores"] = arch_info.num_physical_cores;
    metrics["logical_cores"] = arch_info.num_logical_cores;
    
    return metrics;
}

bool ThreadManager::is_hyperthreading_beneficial(const std::string& workload_type) {
    return workload_type == "memory" || workload_type == "io" || workload_type == "mixed";
}

std::vector<int> ThreadManager::get_optimal_cpu_affinity() {
    std::vector<int> affinity;
    auto arch_info = ArchitectureOptimizer::detect_architecture();
    
    // For now, use all available cores
    for (int i = 0; i < arch_info.num_physical_cores; ++i) {
        affinity.push_back(i);
    }
    
    return affinity;
}

void MemoryProfiler::start_profiling() {
    profiling_start = std::chrono::high_resolution_clock::now();
    initial_memory = get_peak_memory_usage();
    profiling_active = true;
}

MemoryProfile MemoryProfiler::stop_profiling() {
    profiling_active = false;
    return get_current_usage();
}

MemoryProfile MemoryProfiler::get_current_usage() {
    MemoryProfile profile = {};
    
    // Get current memory usage
    profile.current_memory_mb = get_peak_memory_usage() / (1024 * 1024);
    profile.peak_memory_mb = profile.current_memory_mb;
    
    // Get available memory
#ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        profile.available_memory_mb = statex.ullAvailPhys / (1024 * 1024);
    }
#elif defined(__linux__)
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        profile.available_memory_mb = (sys_info.freeram * sys_info.mem_unit) / (1024 * 1024);
    }
#endif
    
    // Estimate cache performance
    profile.memory_bandwidth_gb_s = estimate_cache_performance();
    
    return profile;
}

size_t MemoryProfiler::estimate_memory_requirement(
    const std::string& method, 
    const std::map<std::string, double>& parameters) {
    
    size_t estimated_mb = 0;
    
    if (method == "monte_carlo") {
        auto it = parameters.find("paths");
        if (it != parameters.end()) {
            long paths = static_cast<long>(it->second);
            // Base: 8 bytes per path for payoff
            // Antithetic: no extra memory
            // Control variate: 2x memory for storing control variables
            // QMC: small overhead for Halton state
            estimated_mb = (paths * 8) / (1024 * 1024);
            
            // Check for variance reduction
            auto cv_it = parameters.find("control_variate");
            if (cv_it != parameters.end() && cv_it->second > 0) {
                estimated_mb *= 2;
            }
        }
    } else if (method == "pde") {
        auto s_it = parameters.find("S_steps");
        auto t_it = parameters.find("T_steps");
        if (s_it != parameters.end() && t_it != parameters.end()) {
            int S_steps = static_cast<int>(s_it->second);
            // Need 3 arrays: current, previous, workspace
            estimated_mb = (S_steps * 3 * 8) / (1024 * 1024);
        }
    } else if (method == "slv") {
        auto paths_it = parameters.find("paths");
        auto steps_it = parameters.find("steps");
        if (paths_it != parameters.end() && steps_it != parameters.end()) {
            long paths = static_cast<long>(paths_it->second);
            long steps = static_cast<long>(steps_it->second);
            // Store spot and variance paths
            estimated_mb = (paths * steps * 2 * 8) / (1024 * 1024);
        }
    }
    
    return estimated_mb;
}

void MemoryProfiler::configure_memory_allocation() {
    configure_numa_allocation();
    
    // Additional memory optimizations could go here
    // e.g., setting malloc parameters, huge pages, etc.
}

std::vector<std::string> MemoryProfiler::detect_memory_leaks() {
    std::vector<std::string> leaks;
    
    // This is a simplified leak detection
    // In a real implementation, you might use tools like Valgrind or AddressSanitizer
    
    static size_t baseline_memory = 0;
    if (baseline_memory == 0) {
        baseline_memory = get_peak_memory_usage();
        return leaks; // First call, establish baseline
    }
    
    size_t current_memory = get_peak_memory_usage();
    if (current_memory > baseline_memory * 1.1) { // 10% increase
        leaks.push_back("Potential memory leak detected: " + 
                       std::to_string((current_memory - baseline_memory) / (1024 * 1024)) + 
                       " MB increase");
    }
    
    return leaks;
}

size_t MemoryProfiler::get_peak_memory_usage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.PeakWorkingSetSize;
    }
    return 0;
#elif defined(__linux__)
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.find("VmPeak:") != std::string::npos) {
            std::istringstream iss(line);
            std::string token;
            iss >> token >> token; // Skip "VmPeak:" and get value
            return std::stoull(token) * 1024; // Convert from KB to bytes
        }
    }
    return 0;
#else
    return 0;
#endif
}

double MemoryProfiler::estimate_cache_performance() {
    // Simple memory bandwidth estimation
    const size_t test_size = 64 * 1024 * 1024; // 64MB
    std::vector<double> data(test_size / sizeof(double));
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Sequential read test
    double sum = 0.0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += data[i];
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate bandwidth in GB/s
    double bytes_transferred = test_size;
    double time_seconds = duration.count() / 1e6;
    double bandwidth_gb_s = (bytes_transferred / (1024.0 * 1024.0 * 1024.0)) / time_seconds;
    
    // Prevent compiler optimization
    volatile double sink = sum;
    (void)sink;
    
    return bandwidth_gb_s;
}

void MemoryProfiler::configure_numa_allocation() {
#ifdef __linux__
    if (numa_available() >= 0) {
        // Set default NUMA policy to local allocation
        numa_set_localalloc();
    }
#endif
}

//==============================================================================
std::vector<BenchmarkResult> PerformanceBenchmark::run_benchmark_suite() {
    std::vector<BenchmarkResult> results;
    
    // Memory bandwidth test
    auto memory_test = run_benchmark("Memory Bandwidth", []() {
        const size_t size = 1024 * 1024; // 1M doubles
        std::vector<double> data(size, 1.0);
        double sum = std::accumulate(data.begin(), data.end(), 0.0);
        volatile double sink = sum; // Prevent optimization
        (void)sink;
    });
    results.push_back(memory_test);
    
    // CPU intensive computation test
    auto cpu_test = run_benchmark("CPU Intensive", []() {
        double result = 0.0;
        for (int i = 0; i < 1000000; ++i) {
            result += std::sin(i * 0.001) * std::cos(i * 0.001);
        }
        volatile double sink = result;
        (void)sink;
    });
    results.push_back(cpu_test);
    
    // Cache access pattern test
    auto cache_test = run_benchmark("Cache Performance", []() {
        const size_t cache_size = 256 * 1024; // 256KB, typical L2 cache size
        std::vector<int> data(cache_size / sizeof(int));
        std::iota(data.begin(), data.end(), 0);
        
        // Random access pattern to stress cache
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(data.begin(), data.end(), gen);
        
        volatile int sum = 0;
        for (int val : data) {
            sum += val;
        }
        (void)sum;
    });
    results.push_back(cache_test);
    
    // Floating point operations test
    auto fp_test = run_benchmark("Floating Point Ops", []() {
        const int n = 100000;
        double result = 1.0;
        for (int i = 0; i < n; ++i) {
            result = result * 1.00001 + std::sqrt(i + 1.0) - std::log(i + 2.0);
        }
        volatile double sink = result;
        (void)sink;
    });
    results.push_back(fp_test);
    
    // Vector operations test (SIMD potential)
    auto vector_test = run_benchmark("Vector Operations", []() {
        const size_t n = 100000;
        std::vector<double> a(n, 1.5), b(n, 2.5), c(n);
        
        for (size_t i = 0; i < n; ++i) {
            c[i] = a[i] * b[i] + std::sqrt(a[i]) - b[i] / (a[i] + 1.0);
        }
        
        volatile double sum = std::accumulate(c.begin(), c.end(), 0.0);
        (void)sum;
    });
    results.push_back(vector_test);
    
    // Memory allocation/deallocation test
    auto alloc_test = run_benchmark("Memory Allocation", []() {
        const int iterations = 1000;
        for (int i = 0; i < iterations; ++i) {
            std::vector<double> temp(1024 + i); // Variable size allocation
            std::fill(temp.begin(), temp.end(), i * 0.001);
            volatile double sum = temp[0] + temp[temp.size() - 1];
            (void)sum;
        }
    });
    results.push_back(alloc_test);
    
    return results;
}

BenchmarkResult PerformanceBenchmark::run_benchmark(
    const std::string& test_name,
    std::function<void()> test_function,
    int iterations) {
    
    BenchmarkResult result;
    result.test_name = test_name;
    
    std::vector<double> times;
    MemoryProfiler profiler;
    
    for (int i = 0; i < iterations; ++i) {
        profiler.start_profiling();
        
        auto start = std::chrono::high_resolution_clock::now();
        test_function();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto profile = profiler.stop_profiling();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        times.push_back(duration.count() / 1000.0); // Convert to milliseconds
        
        result.memory_used_mb = std::max(result.memory_used_mb, profile.peak_memory_mb);
    }
    
    // Calculate statistics
    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    result.execution_time_ms = sum / times.size();
    
    // Calculate throughput (operations per second)
    result.throughput = 1000.0 / result.execution_time_ms;
    
    return result;
}

double PerformanceBenchmark::compare_to_baseline(
    const std::vector<BenchmarkResult>& current,
    const std::vector<BenchmarkResult>& baseline) {
    
    if (current.size() != baseline.size()) {
        return 0.0; // Cannot compare
    }
    
    double total_ratio = 0.0;
    for (size_t i = 0; i < current.size(); ++i) {
        if (baseline[i].execution_time_ms > 0) {
            total_ratio += current[i].execution_time_ms / baseline[i].execution_time_ms;
        }
    }
    
    return total_ratio / current.size();
}

void PerformanceBenchmark::save_benchmark_results(
    const std::vector<BenchmarkResult>& results,
    const std::string& filename) {
    
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"timestamp\": \"" << std::time(nullptr) << "\",\n";
    file << "  \"hardware_fingerprint\": \"" << generate_hardware_fingerprint() << "\",\n";
    file << "  \"results\": [\n";
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        file << "    {\n";
        file << "      \"test_name\": \"" << result.test_name << "\",\n";
        file << "      \"execution_time_ms\": " << result.execution_time_ms << ",\n";
        file << "      \"throughput\": " << result.throughput << ",\n";
        file << "      \"memory_used_mb\": " << result.memory_used_mb << "\n";
        file << "    }";
        if (i < results.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
}

std::vector<RegressionTracker> PerformanceBenchmark::load_benchmark_history(
    const std::string& filename) {
    
    std::vector<RegressionTracker> history;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        return history; // Return empty if file doesn't exist
    }
    
    // Simple JSON-like parsing for benchmark history
    // This is a basic implementation that looks for key patterns
    std::string line;
    RegressionTracker current_tracker;
    bool in_results_array = false;
    
    while (std::getline(file, line)) {
        // Remove whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.find("\"version\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":") + 1) + 1;
            size_t end = line.find("\"", start);
            if (start != std::string::npos && end != std::string::npos) {
                current_tracker.version = line.substr(start, end - start);
            }
        } else if (line.find("\"timestamp\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":") + 1) + 1;
            size_t end = line.find("\"", start);
            if (start != std::string::npos && end != std::string::npos) {
                // Simple timestamp parsing (just store as epoch time)
                try {
                    auto timestamp_str = line.substr(start, end - start);
                    std::time_t timestamp = std::stoll(timestamp_str);
                    current_tracker.timestamp = std::chrono::system_clock::from_time_t(timestamp);
                } catch (...) {
                    current_tracker.timestamp = std::chrono::system_clock::now();
                }
            }
        } else if (line.find("\"performance_score\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            std::string score_str = line.substr(start);
            score_str = score_str.substr(0, score_str.find(","));
            try {
                current_tracker.performance_score = std::stod(score_str);
            } catch (...) {
                current_tracker.performance_score = 1.0;
            }
        } else if (line.find("\"hardware_fingerprint\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":") + 1) + 1;
            size_t end = line.find("\"", start);
            if (start != std::string::npos && end != std::string::npos) {
                current_tracker.hardware_fingerprint = line.substr(start, end - start);
            }
        } else if (line.find("\"results\":") != std::string::npos) {
            in_results_array = true;
            current_tracker.results.clear();
        } else if (line.find("}") != std::string::npos && !in_results_array) {
            // End of a tracker object
            history.push_back(current_tracker);
            current_tracker = RegressionTracker{}; // Reset
        } else if (line.find("]") != std::string::npos && in_results_array) {
            in_results_array = false;
        }
    }
    
    return history;
}

std::vector<std::string> PerformanceBenchmark::detect_regressions(
    const std::vector<BenchmarkResult>& current,
    const std::vector<RegressionTracker>& history,
    double threshold) {
    
    std::vector<std::string> regressions;
    
    if (history.empty()) {
        return regressions; // No baseline to compare against
    }
    
    const auto& latest_baseline = history.back().results;
    
    for (size_t i = 0; i < current.size() && i < latest_baseline.size(); ++i) {
        double current_time = current[i].execution_time_ms;
        double baseline_time = latest_baseline[i].execution_time_ms;
        
        if (baseline_time > 0) {
            double regression = (current_time - baseline_time) / baseline_time;
            if (regression > threshold) {
                regressions.push_back(current[i].test_name + ": " + 
                                    std::to_string(regression * 100.0) + "% slower");
            }
        }
    }
    
    return regressions;
}

std::string PerformanceBenchmark::generate_hardware_fingerprint() {
    auto arch_info = ArchitectureOptimizer::detect_architecture();
    
    std::stringstream ss;
    ss << arch_info.cpu_brand << "_"
       << arch_info.num_physical_cores << "c"
       << arch_info.num_logical_cores << "t_"
       << arch_info.l3_cache_size << "kb";
    
    return ss.str();
}

double PerformanceBenchmark::calculate_performance_score(
    const std::vector<BenchmarkResult>& results) {
    
    if (results.empty()) return 0.0;
    
    double total_score = 0.0;
    for (const auto& result : results) {
        // Higher throughput = better score
        total_score += result.throughput;
    }
    
    return total_score / results.size();
}

} // namespace performance
} // namespace bsm
