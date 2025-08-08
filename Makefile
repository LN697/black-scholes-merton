# Makefile for bsm (builds into build/)

# Compiler and flags
CXX ?= g++
OMP ?= 0
QMC ?= 1
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra -Wpedantic -march=native
INCLUDES := -Iinclude
# Portable mkdir
MKDIR := mkdir
ifeq ($(OMP),1)
  CXXFLAGS += -fopenmp
endif

# Directories
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj

# Targets
TARGET := $(BIN_DIR)/bsm
TEST_BIN := $(BIN_DIR)/tests

# Sources and objects
# All production sources (exclude any test mains under src)
SRCS := $(filter-out $(SRC_DIR)/tests.cpp,$(wildcard $(SRC_DIR)/*.cpp))
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(TEST_SRCS))

# Default rule
.PHONY: all
all: $(TARGET)

$(TARGET): $(filter-out $(OBJ_DIR)/tests.o,$(OBJS))
ifeq ($(OS),Windows_NT)
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
else
	@mkdir -p $(BIN_DIR)
endif
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

$(TEST_BIN): $(filter-out $(OBJ_DIR)/main.o,$(OBJS)) $(TEST_OBJS)
ifeq ($(OS),Windows_NT)
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
else
	@mkdir -p $(BIN_DIR)
endif
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# Ensure tests.o is compiled from test/tests.cpp (not src/tests.cpp)
$(OBJ_DIR)/tests.o: $(TEST_DIR)/tests.cpp
ifeq ($(OS),Windows_NT)
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
else
	@mkdir -p $(OBJ_DIR)
endif
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
ifeq ($(OS),Windows_NT)
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
else
	@mkdir -p $(OBJ_DIR)
endif
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
ifeq ($(OS),Windows_NT)
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
else
	@mkdir -p $(OBJ_DIR)
endif
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: test
test: $(TEST_BIN)
	$(TEST_BIN)

# Clean
.PHONY: clean
clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
else
	@rm -rf $(BUILD_DIR)
endif

# Optional: CMake passthrough
.PHONY: cmake
cmake:
	@echo "Generating CMake project..."
	@$(MKDIR) $(BUILD_DIR)/cmake
	@cd $(BUILD_DIR)/cmake && cmake ../..
	@echo "Run: cmake --build build/cmake"

# Windows PowerShell users without make can compile manually:
#   g++ -std=c++17 -O3 -Iinclude src/*.cpp -o bsm.exe
