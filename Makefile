# Black-Scholes-Merton Pricing Toolkit Makefile
# 
# Build Configuration:
#   make                    # Standard optimized build
#   make DEBUG=1           # Debug build with symbols
#   make OMP=1             # Enable OpenMP parallelization
#   make PROFILE=1         # Profile-guided optimization build
#   make COVERAGE=1        # Code coverage analysis build
#   make STATIC=1          # Static linking
#   make test              # Build and run tests
#   make clean             # Clean build artifacts
#   make help              # Show available targets

# Compiler and default flags
CXX ?= g++
AR ?= ar

# Build configuration variables
DEBUG ?= 0
OMP ?= 0
QMC ?= 1
PROFILE ?= 0
COVERAGE ?= 0
STATIC ?= 0
BENCHMARK ?= 0

# Base compiler flags
CXXFLAGS_BASE := -std=c++17 -Wall -Wextra -Wpedantic
INCLUDES := -Iinclude

# Optimization flags
ifeq ($(DEBUG),1)
    CXXFLAGS_OPT := -O0 -g -DDEBUG -fno-omit-frame-pointer
    BUILD_TYPE := debug
else
    CXXFLAGS_OPT := -O3 -DNDEBUG -march=native -mtune=native
    BUILD_TYPE := release
endif

# Link-time optimization
ifneq ($(DEBUG),1)
    CXXFLAGS_OPT += -flto
    LDFLAGS += -flto
endif

# OpenMP support
ifeq ($(OMP),1)
    CXXFLAGS_OPT += -fopenmp -DUSE_OPENMP
    LDFLAGS += -fopenmp
endif

# Profile-guided optimization
ifeq ($(PROFILE),1)
    CXXFLAGS_OPT += -fprofile-generate
    LDFLAGS += -fprofile-generate
endif

# Code coverage
ifeq ($(COVERAGE),1)
    CXXFLAGS_OPT += --coverage -fno-inline -fno-inline-small-functions
    LDFLAGS += --coverage
endif

# Static linking
ifeq ($(STATIC),1)
    LDFLAGS += -static
endif

# Benchmark mode with timing
ifeq ($(BENCHMARK),1)
    CXXFLAGS_OPT += -DBENCHMARK_MODE
endif

# Platform-specific optimizations
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
ifeq ($(UNAME_S),Linux)
    CXXFLAGS_OPT += -pthread
    LDFLAGS += -pthread
endif

# Combine all flags
CXXFLAGS := $(CXXFLAGS_BASE) $(CXXFLAGS_OPT) $(INCLUDES)

# Directories
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build/$(BUILD_TYPE)
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj

# Targets
TARGET := $(BIN_DIR)/bsm
TEST_BIN := $(BIN_DIR)/tests

# Sources and objects
SRCS := $(filter-out $(SRC_DIR)/tests.cpp,$(wildcard $(SRC_DIR)/*.cpp))
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(TEST_SRCS))

# Header dependencies
HEADERS := $(wildcard include/*.hpp)

# Default target
.PHONY: all
all: $(TARGET)

# Main executable
$(TARGET): $(filter-out $(OBJ_DIR)/tests.o,$(OBJS)) | $(BIN_DIR)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Test executable  
$(TEST_BIN): $(filter-out $(OBJ_DIR)/main.o,$(OBJS)) $(TEST_OBJS) | $(BIN_DIR)
	@echo "Linking $(TEST_BIN)..."
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Object file compilation with header dependencies
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp $(HEADERS) | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Directory creation
$(BIN_DIR) $(OBJ_DIR):
	@echo "Creating directory $@..."
ifeq ($(OS),Windows_NT)
	@if not exist "$@" mkdir "$@" >nul 2>&1
else
	@mkdir -p $@
endif
# Build and run tests
.PHONY: test
test: $(TEST_BIN)
	@echo "Running tests..."
	$(TEST_BIN)

# Run with different configurations
.PHONY: test-all
test-all:
	@echo "Running comprehensive test suite..."
	@$(MAKE) clean && $(MAKE) test
	@$(MAKE) clean && $(MAKE) test OMP=1
	@$(MAKE) clean && $(MAKE) test DEBUG=1

# Performance benchmark
.PHONY: benchmark
benchmark: 
	@echo "Running performance benchmarks..."
	@$(MAKE) clean && $(MAKE) BENCHMARK=1
	@echo "=== Single-threaded benchmark ==="
	@time $(TARGET)
ifeq ($(OMP),1)
	@echo "=== Multi-threaded benchmark ==="
	@export OMP_NUM_THREADS=4 && time $(TARGET)
endif

# Profile-guided optimization build
.PHONY: pgo
pgo:
	@echo "Building with profile-guided optimization..."
	@$(MAKE) clean
	@$(MAKE) PROFILE=1
	@echo "Generating profile data..."
	@$(TARGET) > /dev/null
	@$(MAKE) clean
	@$(MAKE) CXXFLAGS_OPT="-O3 -march=native -fprofile-use"
	@echo "PGO build complete"

# Code coverage analysis
.PHONY: coverage
coverage:
	@echo "Generating code coverage report..."
	@$(MAKE) clean && $(MAKE) test COVERAGE=1
	@gcov -r $(OBJS)
	@echo "Coverage files generated: *.gcov"

# Static analysis with cppcheck (if available)
.PHONY: static-analysis
static-analysis:
	@echo "Running static analysis..."
	@which cppcheck > /dev/null && cppcheck --enable=all --std=c++17 $(SRC_DIR)/ $(TEST_DIR)/ include/ || echo "cppcheck not found"

# Code formatting with clang-format (if available)
.PHONY: format
format:
	@echo "Formatting code..."
	@which clang-format > /dev/null && find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i || echo "clang-format not found"

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
ifeq ($(OS),Windows_NT)
	@if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)" >nul 2>&1
	@if exist "*.gcov" del /Q "*.gcov" >nul 2>&1
	@if exist "*.gcda" del /Q "*.gcda" >nul 2>&1
	@if exist "*.gcno" del /Q "*.gcno" >nul 2>&1
else
	@rm -rf $(BUILD_DIR) *.gcov *.gcda *.gcno
endif

# Deep clean (including profile data)
.PHONY: distclean
distclean: clean
	@echo "Deep cleaning..."
ifeq ($(OS),Windows_NT)
	@if exist "build" rmdir /S /Q "build" >nul 2>&1
else
	@rm -rf build/
endif

# Install (copy to system directories - requires sudo on Unix)
.PHONY: install
install: $(TARGET)
	@echo "Installing to system directories..."
ifeq ($(OS),Windows_NT)
	@echo "Manual installation required on Windows"
else
	@sudo cp $(TARGET) /usr/local/bin/
	@sudo mkdir -p /usr/local/include/bsm
	@sudo cp include/*.hpp /usr/local/include/bsm/
	@echo "Installed to /usr/local/bin and /usr/local/include/bsm"
endif

# Create source package
.PHONY: package
package:
	@echo "Creating source package..."
	@tar czf bsm-pricing-toolkit-$(shell date +%Y%m%d).tar.gz \
		--exclude=build --exclude=*.gcov --exclude=*.gcda --exclude=*.gcno \
		--exclude=.git --exclude=*.tar.gz \
		Makefile README.md include/ src/ test/ docs/

# Generate compile_commands.json for IDE support
.PHONY: compile-commands
compile-commands:
	@echo "Generating compile_commands.json..."
	@$(MAKE) clean
	@bear -- $(MAKE) all test

# Show configuration
.PHONY: config
config:
	@echo "=== Build Configuration ==="
	@echo "CXX:          $(CXX)"
	@echo "CXXFLAGS:     $(CXXFLAGS)"
	@echo "LDFLAGS:      $(LDFLAGS)"
	@echo "DEBUG:        $(DEBUG)"
	@echo "OMP:          $(OMP)"
	@echo "PROFILE:      $(PROFILE)"
	@echo "COVERAGE:     $(COVERAGE)"
	@echo "STATIC:       $(STATIC)"
	@echo "BENCHMARK:    $(BENCHMARK)"
	@echo "BUILD_TYPE:   $(BUILD_TYPE)"
	@echo "TARGET:       $(TARGET)"
	@echo "TEST_BIN:     $(TEST_BIN)"

# Examples targets
EXAMPLES_DIR := examples
EXAMPLES_SRC := $(wildcard $(EXAMPLES_DIR)/*.cpp)
EXAMPLES_TARGETS := $(patsubst $(EXAMPLES_DIR)/%.cpp,$(BIN_DIR)/%,$(EXAMPLES_SRC))

# Example executables
.PHONY: examples
examples: $(EXAMPLES_TARGETS)

# Individual example targets
.PHONY: option-chain-analysis
option-chain-analysis: $(BIN_DIR)/option_chain_analysis

.PHONY: volatility-surface  
volatility-surface: $(BIN_DIR)/volatility_surface

.PHONY: risk-management
risk-management: $(BIN_DIR)/risk_management

# Build example executables
$(BIN_DIR)/option_chain_analysis: $(EXAMPLES_DIR)/option_chain_analysis.cpp $(filter-out $(OBJ_DIR)/main.o $(OBJ_DIR)/tests.o,$(OBJS)) | $(BIN_DIR)
	@echo "Building option chain analysis..."
	$(CXX) $(CXXFLAGS) $(EXAMPLES_DIR)/option_chain_analysis.cpp $(filter-out $(OBJ_DIR)/main.o $(OBJ_DIR)/tests.o,$(OBJS)) -o $@ $(LDFLAGS)

$(BIN_DIR)/volatility_surface: $(EXAMPLES_DIR)/volatility_surface.cpp $(filter-out $(OBJ_DIR)/main.o $(OBJ_DIR)/tests.o,$(OBJS)) | $(BIN_DIR)
	@echo "Building volatility surface analysis..."
	$(CXX) $(CXXFLAGS) $(EXAMPLES_DIR)/volatility_surface.cpp $(filter-out $(OBJ_DIR)/main.o $(OBJ_DIR)/tests.o,$(OBJS)) -o $@ $(LDFLAGS)

$(BIN_DIR)/risk_management: $(EXAMPLES_DIR)/risk_management.cpp $(filter-out $(OBJ_DIR)/main.o $(OBJ_DIR)/tests.o,$(OBJS)) | $(BIN_DIR)
	@echo "Building risk management analysis..."
	$(CXX) $(CXXFLAGS) $(EXAMPLES_DIR)/risk_management.cpp $(filter-out $(OBJ_DIR)/main.o $(OBJ_DIR)/tests.o,$(OBJS)) -o $@ $(LDFLAGS)

# Run all examples
.PHONY: run-examples
run-examples: examples
	@echo ""
	@echo "=== Running Option Chain Analysis ==="
	$(BIN_DIR)/option_chain_analysis
	@echo ""
	@echo "=== Running Volatility Surface Analysis ==="
	$(BIN_DIR)/volatility_surface
	@echo ""
	@echo "=== Running Risk Management Analysis ==="
	$(BIN_DIR)/risk_management

# Run individual examples
.PHONY: run-option-chain
run-option-chain: $(BIN_DIR)/option_chain_analysis
	@echo "Running Option Chain Analysis..."
	$(BIN_DIR)/option_chain_analysis

.PHONY: run-volatility-surface
run-volatility-surface: $(BIN_DIR)/volatility_surface  
	@echo "Running Volatility Surface Analysis..."
	$(BIN_DIR)/volatility_surface

.PHONY: run-risk-management
run-risk-management: $(BIN_DIR)/risk_management
	@echo "Running Risk Management Analysis..."
	$(BIN_DIR)/risk_management

# Clean examples
.PHONY: clean-examples
clean-examples:
	@echo "Cleaning example executables..."
ifeq ($(OS),Windows_NT)
	@if exist "$(BIN_DIR)\option_chain_analysis.exe" del "$(BIN_DIR)\option_chain_analysis.exe" >nul 2>&1
	@if exist "$(BIN_DIR)\volatility_surface.exe" del "$(BIN_DIR)\volatility_surface.exe" >nul 2>&1  
	@if exist "$(BIN_DIR)\risk_management.exe" del "$(BIN_DIR)\risk_management.exe" >nul 2>&1
	@if exist "$(EXAMPLES_DIR)\volatility_surface.csv" del "$(EXAMPLES_DIR)\volatility_surface.csv" >nul 2>&1
else
	@rm -f $(BIN_DIR)/option_chain_analysis $(BIN_DIR)/volatility_surface $(BIN_DIR)/risk_management
	@rm -f $(EXAMPLES_DIR)/volatility_surface.csv
endif

# Update clean target to include examples
clean: clean-examples

# Help target
.PHONY: help
help:
	@echo "=== Black-Scholes-Merton Pricing Toolkit Build System ==="
	@echo ""
	@echo "Available targets:"
	@echo "  all               Build main executable (default)"
	@echo "  test              Build and run unit tests"
	@echo "  test-all          Run comprehensive test suite"
	@echo "  examples          Build all example programs"
	@echo "  run-examples      Build and run all example programs"
	@echo "  option-chain-analysis    Build option chain analysis example"
	@echo "  volatility-surface       Build volatility surface analysis example" 
	@echo "  risk-management          Build risk management analysis example"
	@echo "  run-option-chain         Run option chain analysis"
	@echo "  run-volatility-surface   Run volatility surface analysis"
	@echo "  run-risk-management      Run risk management analysis"
	@echo "  benchmark         Run performance benchmarks"
	@echo "  pgo               Profile-guided optimization build"
	@echo "  coverage          Generate code coverage report"
	@echo "  static-analysis   Run static code analysis"
	@echo "  format            Format code with clang-format"
	@echo "  clean             Clean build artifacts"
	@echo "  clean-examples    Clean example executables"
	@echo "  distclean         Deep clean including all build directories"
	@echo "  install           Install to system directories"
	@echo "  package           Create source package"
	@echo "  compile-commands  Generate compile_commands.json"
	@echo "  config            Show current build configuration"
	@echo "  help              Show this help message"
	@echo ""
	@echo "Build options (use as: make OPTION=1 target):"
	@echo "  DEBUG=1           Enable debug build with symbols"
	@echo "  OMP=1             Enable OpenMP parallelization"
	@echo "  PROFILE=1         Enable profile-guided optimization"
	@echo "  COVERAGE=1        Enable code coverage analysis"
	@echo "  STATIC=1          Enable static linking"
	@echo "  BENCHMARK=1       Enable benchmark timing"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Standard optimized build"
	@echo "  make test OMP=1         # Test with OpenMP enabled"
	@echo "  make examples           # Build all example programs"
	@echo "  make run-examples       # Build and run all examples"
	@echo "  make run-option-chain   # Run option chain analysis"
	@echo "  make DEBUG=1            # Debug build"
	@echo "  make benchmark          # Performance benchmarks"

# CMake integration (optional)
.PHONY: cmake
cmake:
	@echo "Generating CMake build system..."
	@mkdir -p $(BUILD_DIR)/cmake
	@echo 'cmake_minimum_required(VERSION 3.15)' > $(BUILD_DIR)/cmake/CMakeLists.txt
	@echo 'project(BSMPricingToolkit VERSION 1.0.0 LANGUAGES CXX)' >> $(BUILD_DIR)/cmake/CMakeLists.txt
	@echo 'set(CMAKE_CXX_STANDARD 17)' >> $(BUILD_DIR)/cmake/CMakeLists.txt
	@echo 'set(CMAKE_CXX_STANDARD_REQUIRED ON)' >> $(BUILD_DIR)/cmake/CMakeLists.txt
	@echo 'file(GLOB_RECURSE SOURCES "../../src/*.cpp")' >> $(BUILD_DIR)/cmake/CMakeLists.txt
	@echo 'list(REMOVE_ITEM SOURCES "../../src/tests.cpp")' >> $(BUILD_DIR)/cmake/CMakeLists.txt
	@echo 'add_executable(bsm $${SOURCES})' >> $(BUILD_DIR)/cmake/CMakeLists.txt
	@echo 'target_include_directories(bsm PRIVATE "../../include")' >> $(BUILD_DIR)/cmake/CMakeLists.txt
	@echo 'target_compile_options(bsm PRIVATE -O3 -march=native)' >> $(BUILD_DIR)/cmake/CMakeLists.txt
	@echo "CMake project generated in $(BUILD_DIR)/cmake/"
	@echo "Usage: cd $(BUILD_DIR)/cmake && cmake . && make"

# Windows-specific manual compilation helper
.PHONY: windows-manual
windows-manual:
	@echo "Manual compilation command for Windows (without make):"
	@echo ""
	@echo "g++ -std=c++17 -O3 -march=native -DNDEBUG -Iinclude src/*.cpp -o bsm.exe"
	@echo ""
	@echo "With OpenMP:"
	@echo "g++ -std=c++17 -O3 -march=native -fopenmp -DNDEBUG -DUSE_OPENMP -Iinclude src/*.cpp -o bsm.exe"
