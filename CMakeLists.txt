cmake_minimum_required(VERSION 3.10)
project(TwoPairsFramework VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# 1. Define Core Include Layout (Header-Only Interface)
# ============================================================================
add_library(quant_core INTERFACE)

target_include_directories(quant_core INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    "C:/dev/Eigen/eigen-master" 
)

# ============================================================================
# 2. Define Shared Source Files
# ============================================================================
# Instead of duplicating source lists across multiple executables, 
# we pool our core processing logic into a reusable variable.
set(ENGINE_SOURCES
    src/engines/trading_engine.cpp          # Fixed typo from 'strading_engine'
)

# ============================================================================
# 3. Define Targets & Executables
# ============================================================================

# Backtester Build Target
add_executable(backtester
    src/main_backtest.cpp 
    ${ENGINE_SOURCES}
)
target_link_libraries(backtester PRIVATE quant_core)

# Predictor Build Target
add_executable(predictor 
    src/main_predict.cpp 
    ${ENGINE_SOURCES} # Shared engine math logic compiled here as well
)
target_link_libraries(predictor PRIVATE quant_core)

# ============================================================================
# 4. Optimization & Compiler Warning Configuration
# ============================================================================
if(MSVC)
    # Windows Developer Optimization Flags
    target_compile_options(backtester PRIVATE /W4 /O2 /EHsc)
    target_compile_options(predictor PRIVATE /W4 /O2 /EHsc)
else()
    # Linux / macOS Optimization Flags (-O3 optimizes vectorized Eigen operations)
    target_compile_options(backtester PRIVATE -Wall -Wextra -O3 -pthread)
    target_compile_options(predictor PRIVATE -Wall -Wextra -O3 -pthread)
endif()