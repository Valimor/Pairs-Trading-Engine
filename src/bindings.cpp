#include <pybind11/pybind11.h>
#include <pybind11/stl.h>   // Required to automatically convert C++ std::vector to Python lists
#include <pybind11/eigen.h> // Magic header for NumPy <-> Eigen conversion

// Include your exact engine headers
#include <quant/engines/walk_forward_controller.hpp>
#include <quant/core/csv_data_provider.hpp>
#include <quant/core/trade_logger.hpp> // Added to support your TradeLogger class

namespace py = pybind11;

PYBIND11_MODULE(qr_engine_boost, m) {
    m.doc() = "High-performance GARCH Pairs Trading Engine";

    // ========================================================================
    // 1. OptimizationResult Structure Bindings
    // ========================================================================
    // We expose ALL member fields so your Python script can parse hyper-parameters,
    // calculate max Sharpe Ratios, and read validation pass results.
    py::class_<qr_core::OptimizationResult>(m, "OptimizationResult")
        .def(py::init<>()) // Expose default constructor if needed for creation in Python
        .def_readwrite("alpha", &qr_core::OptimizationResult::alpha)
        .def_readwrite("beta", &qr_core::OptimizationResult::beta)
        .def_readwrite("entry_z", &qr_core::OptimizationResult::entry_z)       // Added for step 3
        .def_readwrite("stop_loss", &qr_core::OptimizationResult::stop_loss)   // Added for step 3
        .def_readwrite("sharpe_ratio", &qr_core::OptimizationResult::sharpe_ratio)
        .def_readwrite("total_pnl", &qr_core::OptimizationResult::total_pnl)
        .def_readwrite("trade_count", &qr_core::OptimizationResult::trade_count); // Added for step 4

    // ========================================================================
    // 2. Data Provider Interface & Implementations
    // ========================================================================
    // Because WalkForwardController takes IDataProvider as a constructor reference,
    // pybind11 needs to hold the base class definition to safely pass instances down.
    py::class_<qr_core::IDataProvider>(m, "IDataProvider");

    py::class_<qr_core::CSVDataProvider, qr_core::IDataProvider>(m, "CSVDataProvider")
        .def(py::init<const std::string&>())
        .def("load_all", &qr_core::CSVDataProvider::load_all)
        .def("total_ticks", &qr_core::CSVDataProvider::total_ticks); // Added for training window step

    // ========================================================================
    // 3. Trade Logger Bindings
    // ========================================================================
    // CRITICAL FIX: Added this binding block so your Python script can spin up 
    // execution logs directly onto your disk storage.
    py::class_<qr_core::TradeLogger>(m, "TradeLogger")
        .def(py::init<const std::string&>());

    // ========================================================================
    // 4. WalkForwardController Execution Hub Bindings
    // ========================================================================
    py::class_<qr_engine::WalkForwardController>(m, "WalkForwardController")
        .def(py::init<qr_core::IDataProvider&, qr_core::TradeLogger&>())
        .def("run_parallel_search", &qr_engine::WalkForwardController::run_parallel_search)
        .def("execute_window_optimized", &qr_engine::WalkForwardController::execute_window_optimized);
}