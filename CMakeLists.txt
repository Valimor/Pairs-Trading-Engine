cmake_minimum_required(VERSION 3.10)
project(TwoPairsFramework VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Define the Core Library (Header-only or source)
# This contains your math_utils, GARCH, VIX logic, etc.
add_library(quant_core INTERFACE)
target_include_directories(quant_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 2. Define the Backtester Executable
add_executable(backtester 
    src/main_backtest.cpp 
#    src/strategy.cpp
)
target_link_libraries(backtester PRIVATE quant_core)

# 3. Define the Predictor Executable (The new "Forward" version)
add_executable(predictor 
    src/main_predict.cpp 
#    src/signal_generator.cpp
)
target_link_libraries(predictor PRIVATE quant_core)

# Optional: Add compiler flags for performance
if(MSVC)
    target_compile_options(backtester PRIVATE /W4 /O2)
    target_compile_options(predictor PRIVATE /W4 /O2)
else()
    target_compile_options(backtester PRIVATE -Wall -Wextra -O3)
    target_compile_options(predictor PRIVATE -Wall -Wextra -O3)
endif()