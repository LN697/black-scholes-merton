@echo off
REM Enhanced build script for BSM Toolkit with UX improvements
REM Builds both traditional and enhanced UX versions

echo ======================================
echo BSM Toolkit Enhanced Build Script
echo ======================================
echo.

REM Check if we're in the right directory
if not exist "Makefile" (
    echo Error: Makefile not found. Please run this script from the project root.
    exit /b 1
)

REM Clean previous builds
echo Cleaning previous builds...
make clean
if errorlevel 1 (
    echo Error: Clean failed
    exit /b 1
)

echo.
echo Building traditional BSM toolkit...
make
if errorlevel 1 (
    echo Error: Traditional build failed
    exit /b 1
)

REM Build enhanced UI version
echo.
echo Building enhanced CLI interface...

REM Create UI output directory
if not exist "build\ui\" mkdir build\ui\

REM Copy source files
xcopy /y ui\*.* build\ui\ >nul 2>&1

REM Build enhanced executable using existing Makefile infrastructure
echo Compiling enhanced CLI components...
x86_64-w64-mingw32-g++ -std=c++17 -Wall -Wextra -Wpedantic -O3 -DNDEBUG -march=native -mtune=native -funroll-loops -fprefetch-loop-arrays -fomit-frame-pointer -flto -Iinclude -c ui/main_enhanced.cpp -o build/release/obj/main_enhanced.o
if errorlevel 1 (
    echo Error: Enhanced main compilation failed
    exit /b 1
)

echo Compiling enhanced CLI utilities...
x86_64-w64-mingw32-g++ -std=c++17 -Wall -Wextra -Wpedantic -O3 -DNDEBUG -march=native -mtune=native -funroll-loops -fprefetch-loop-arrays -fomit-frame-pointer -flto -Iinclude -c ui/cli/enhanced_cli.cpp -o build/release/obj/enhanced_cli.o
if errorlevel 1 (
    echo Error: Enhanced CLI compilation failed
    exit /b 1
)

echo Linking enhanced executable...
x86_64-w64-mingw32-g++ -std=c++17 -Wall -Wextra -Wpedantic -O3 -DNDEBUG -march=native -mtune=native -funroll-loops -fprefetch-loop-arrays -fomit-frame-pointer -flto -Iinclude build/release/obj/analytic_bs.o build/release/obj/lsm.o build/release/obj/main_enhanced.o build/release/obj/enhanced_cli.o build/release/obj/monte_carlo_gbm.o build/release/obj/pde_cn.o build/release/obj/pde_cn_american.o build/release/obj/performance_utils.o build/release/obj/slv.o build/release/obj/slv_calibration.o -o build/release/bin/bsm_enhanced.exe -flto
if errorlevel 1 (
    echo Error: Enhanced executable linking failed
    exit /b 1
)

echo ✓ Enhanced CLI interface compiled successfully
echo ✓ Enhanced executable: build\release\bin\bsm_enhanced.exe

echo ======================================
echo Build Summary:
echo ======================================
echo ✓ Core BSM library compiled successfully
echo ✓ Main executable: build\bin\bsm.exe
echo ✓ Test executable: build\bin\tests.exe
echo ✓ Enhanced UI source ready for integration
echo.

REM Display build artifacts
echo Build artifacts:
dir build\bin\*.exe /b

echo.
echo To run examples:
echo   build\bin\bsm.exe --help
echo   build\bin\tests.exe
echo   python scripts\bsm_python_demo.py
echo.

echo Next steps for UX enhancement:
echo 1. Build Python bindings (pybind11)
echo 2. Setup web dashboard (React + REST API)
echo 3. Implement real-time data connections
echo.

echo ✓ Build completed successfully!
pause
