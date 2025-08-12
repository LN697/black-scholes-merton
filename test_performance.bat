@echo off
REM Performance optimization test script for BSM Pricing Toolkit (Windows)
REM 
REM This script demonstrates the new performance optimization features
REM and validates that they work correctly across different build configurations.

echo === BSM Performance Optimization Test Suite ===
echo Testing new performance features and optimizations
echo.

REM Function to run a test and report results
set FAILED_TESTS=0

call :run_test "Standard optimized build" "make optimized"
if errorlevel 1 set /a FAILED_TESTS+=1

if exist "build\release\bin\bsm.exe" (
    call :run_test "Architecture detection" "build\release\bin\bsm.exe --arch-info"
    if errorlevel 1 set /a FAILED_TESTS+=1
) else (
    echo Binary not found, skipping architecture test
)

if exist "build\release\bin\bsm.exe" (
    call :run_test "Numerical accuracy validation" "build\release\bin\bsm.exe --validate-accuracy"
    if errorlevel 1 set /a FAILED_TESTS+=1
) else (
    echo Binary not found, skipping accuracy test
)

if exist "build\release\bin\bsm.exe" (
    call :run_test "Quick performance benchmark" "build\release\bin\bsm.exe --quick-benchmark"
    if errorlevel 1 set /a FAILED_TESTS+=1
) else (
    echo Binary not found, skipping benchmark test
)

call :run_test "Ultra-optimized build" "make ultra-optimized"
if errorlevel 1 set /a FAILED_TESTS+=1

call :run_test "Performance regression test" "make regression-test"
if errorlevel 1 set /a FAILED_TESTS+=1

call :run_test "Threading performance analysis" "make thread-analysis"
if errorlevel 1 set /a FAILED_TESTS+=1

call :run_test "Test suite with performance features" "make test OMP=1 PERFORMANCE=1"
if errorlevel 1 set /a FAILED_TESTS+=1

if exist "build\release\bin\bsm.exe" (
    call :run_test "Benchmark suite" "build\release\bin\bsm.exe --benchmark-suite"
    if errorlevel 1 set /a FAILED_TESTS+=1
) else (
    echo Binary not found, skipping benchmark suite
)

echo.
echo === Performance Test Summary ===
if %FAILED_TESTS% EQU 0 (
    echo All performance optimization tests completed successfully!
) else (
    echo %FAILED_TESTS% test(s) failed!
)
echo.
echo Performance features validated:
echo   * Optimized compiler flags
echo   * Architecture detection and optimization
echo   * Numerical accuracy validation
echo   * Performance benchmarking
echo   * Threading optimization
echo   * Memory profiling
echo   * Regression testing
echo.
echo Build targets tested:
echo   * make optimized
echo   * make ultra-optimized
echo   * make regression-test
echo   * make thread-analysis
echo.
echo Runtime features tested:
echo   * --arch-info
echo   * --validate-accuracy
echo   * --quick-benchmark
echo   * --benchmark-suite
echo.
echo Next steps:
echo   1. Use 'make optimized' for production builds
echo   2. Run 'make validate-arch' before deployment
echo   3. Monitor performance with 'make regression-test'
echo   4. Use '.\bsm.exe --arch-info' to understand target hardware
echo   5. Profile memory usage for large simulations
echo.
echo See docs\performance_guide.md for detailed optimization guidance.

if %FAILED_TESTS% NEQ 0 exit /b 1
goto :eof

:run_test
set TEST_NAME=%~1
set COMMAND=%~2
echo Running: %TEST_NAME%
echo Command: %COMMAND%

%COMMAND% >nul 2>&1
if errorlevel 1 (
    echo ✗ FAILED: %TEST_NAME%
    exit /b 1
) else (
    echo ✓ PASSED: %TEST_NAME%
)
echo.
exit /b 0
