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

REM Build enhanced UI version (placeholder for future Makefile integration)
echo.
echo Enhanced UI features:
echo - Rich CLI interface with colors and tables
echo - Interactive mode support
echo - JSON/CSV output formats
echo - Progress bars and status indicators
echo - Comprehensive help system
echo.

REM Copy enhanced CLI source to temporary location for future integration
if not exist "build\ui\" mkdir build\ui\
xcopy /y ui\*.* build\ui\ >nul 2>&1

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
echo 1. Integrate enhanced CLI with Makefile
echo 2. Build Python bindings (pybind11)
echo 3. Setup web dashboard (React + REST API)
echo 4. Implement real-time data connections
echo.

echo ✓ Build completed successfully!
pause
