cmake_minimum_required(VERSION 3.10)
project(TwoPairsFramework VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Define the Core Library
add_library(quant_core INTERFACE)

# Force the include paths directly. 
# Make sure "C:/dev/Eigen" contains the actual subfolder named "Eigen"
target_include_directories(quant_core INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    "C:/dev/Eigen/eigen-master" 
)

# 2. Define the Backtester Executable
add_executable(backtester 
    src/main_backtest.cpp 
)
target_link_libraries(backtester PRIVATE quant_core)

# 3. Define the Predictor Executable
add_executable(predictor 
    src/main_predict.cpp 
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