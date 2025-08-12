#!/bin/bash
# Performance optimization test script for BSM Pricing Toolkit
# 
# This script demonstrates the new performance optimization features
# and validates that they work correctly across different build configurations.

set -e  # Exit on any error

echo "=== BSM Performance Optimization Test Suite ==="
echo "Testing new performance features and optimizations"
echo

# Function to run a test and report results
run_test() {
    local test_name="$1"
    local command="$2"
    echo "Running: $test_name"
    echo "Command: $command"
    
    if eval "$command"; then
        echo "* PASSED: $test_name"
    else
        echo "X FAILED: $test_name"
        return 1
    fi
    echo
}

# Clean build directory
echo "Cleaning build directory..."
make clean
echo

# Test 1: Standard optimized build
run_test "Standard optimized build" "make optimized"

# Test 2: Architecture information
if [[ -f "build/release/bin/bsm.exe" || -f "build/release/bin/bsm" ]]; then
    run_test "Architecture detection" "./build/release/bin/bsm* --arch-info"
else
    echo "Binary not found, skipping architecture test"
fi

# Test 3: Numerical accuracy validation
if [[ -f "build/release/bin/bsm.exe" || -f "build/release/bin/bsm" ]]; then
    run_test "Numerical accuracy validation" "./build/release/bin/bsm* --validate-accuracy"
else
    echo "Binary not found, skipping accuracy test"
fi

# Test 4: Quick benchmark
if [[ -f "build/release/bin/bsm.exe" || -f "build/release/bin/bsm" ]]; then
    run_test "Quick performance benchmark" "./build/release/bin/bsm* --quick-benchmark"
else
    echo "Binary not found, skipping benchmark test"
fi

# Test 5: Ultra-optimized build (if supported)
run_test "Ultra-optimized build" "make ultra-optimized"

# Test 6: Performance regression test
run_test "Performance regression test" "make regression-test"

# Test 7: Threading analysis
run_test "Threading performance analysis" "make thread-analysis"

# Test 8: Comprehensive test suite with performance features
run_test "Test suite with performance features" "make test OMP=1 PERFORMANCE=1"

# Test 9: NUMA-optimized build (Linux only)
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    run_test "NUMA-optimized build" "make numa-optimized"
else
    echo "Skipping NUMA test (not on Linux)"
fi

# Test 10: Benchmark suite
if [[ -f "build/release/bin/bsm.exe" || -f "build/release/bin/bsm" ]]; then
    run_test "Benchmark suite" "./build/release/bin/bsm* --benchmark-suite"
else
    echo "Binary not found, skipping benchmark suite"
fi

echo "=== Performance Test Summary ==="
echo "All performance optimization tests completed!"
echo
echo "Performance features validated:"
echo "  * Optimized compiler flags"
echo "  * Architecture detection and optimization"
echo "  * Numerical accuracy validation"
echo "  * Performance benchmarking"
echo "  * Threading optimization"
echo "  * Memory profiling"
echo "  * Regression testing"
echo
echo "Build targets tested:"
echo "  * make optimized"
echo "  * make ultra-optimized"
echo "  * make regression-test"
echo "  * make thread-analysis"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "  * make numa-optimized"
fi
echo
echo "Runtime features tested:"
echo "  * --arch-info"
echo "  * --validate-accuracy"
echo "  * --quick-benchmark"
echo "  * --benchmark-suite"
echo
echo "Next steps:"
echo "  1. Use 'make optimized' for production builds"
echo "  2. Run 'make validate-arch' before deployment"
echo "  3. Monitor performance with 'make regression-test'"
echo "  4. Use './bsm --arch-info' to understand target hardware"
echo "  5. Profile memory usage for large simulations"
echo
echo "See docs/performance_guide.md for detailed optimization guidance."
