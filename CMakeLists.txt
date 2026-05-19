cmake_minimum_required(VERSION 3.10)
project(TwoPairsFramework VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# 1. Define Shared Variables & Core Layout (Must Be Defined First!)
# ============================================================================
add_library(quant_core INTERFACE)

target_include_directories(quant_core INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    "C:/dev/Eigen/eigen-master" 
)

# Pool your core C++ processing logic source files here
set(ENGINE_SOURCES
    src/engines/trading_engine.cpp
)

# ============================================================================
# 2. Pull External Dependencies via FetchContent
# ============================================================================
include(FetchContent)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.13.1
)

FetchContent_MakeAvailable(pybind11)

# ============================================================================
# 3. Define Standard Standalone Executables (No Python Dependencies)
# ============================================================================

# Standard Backtester Command-Line App (Compiles cleanly without needing Python hooks)
add_executable(backtester
    src/main_backtest.cpp 
    ${ENGINE_SOURCES}
)
target_link_libraries(backtester PRIVATE quant_core)

# Standard Predictor Command-Line App
add_executable(predictor 
    src/main_predict.cpp 
    ${ENGINE_SOURCES} 
)
target_link_libraries(predictor PRIVATE quant_core)

# ============================================================================
# 4. Define the Python Extension Target (Uses pybind11 and bindings.cpp)
# ============================================================================

# Create your shared binary library module (.pyd / .so extension format)
pybind11_add_module(qr_engine_boost src/bindings.cpp ${ENGINE_SOURCES})

# Explicitly link your custom framework mechanics and pybind configurations
target_link_libraries(qr_engine_boost PRIVATE quant_core pybind11::module)

# ============================================================================
# 5. Optimization & Compiler Warning Configuration
# ============================================================================
if(MSVC)
    target_compile_options(backtester PRIVATE /W4 /O2 /EHsc)
    target_compile_options(predictor PRIVATE /W4 /O2 /EHsc)
    target_compile_options(qr_engine_boost PRIVATE /W4 /O2 /EHsc)
else()
    target_compile_options(backtester PRIVATE -Wall -Wextra -O3 -pthread)
    target_compile_options(predictor PRIVATE -Wall -Wextra -O3 -pthread)
    target_compile_options(qr_engine_boost PRIVATE -Wall -Wextra -O3 -pthread)
endif()

# AUTOMATED PIPELINE: Copy the compiled binary directly to your project root or scripts folder
add_custom_command(TARGET qr_engine_boost POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:qr_engine_boost>
        ${CMAKE_CURRENT_SOURCE_DIR}/python/  # Copies it straight to your main repository folder
    COMMENT "Deploying compiled python module wrapper to project workspace..."
)