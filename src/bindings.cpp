#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <quant/core/types.hpp>
#include <quant/core/csv_data_provider.hpp>
#include <quant/core/trade_logger.hpp>
#include <quant/engines/walk_forward_controller.hpp>

namespace py = pybind11;

PYBIND11_MODULE(qr_engine_boost, m) {
    m.doc() = "High-Performance GARCH Pairs Trading Engine Core SDK";

    // ========================================================================
    // 1. Expose qr_core::BacktestResult Struct to Python
    // ========================================================================
    py::class_<qr_core::BacktestResult>(m, "BacktestResult")
        .def(py::init<>())
        .def_readwrite("total_pnl", &qr_core::BacktestResult::total_pnl)
        .def_readwrite("sharpe_ratio", &qr_core::BacktestResult::sharpe_ratio)
        .def_readwrite("max_drawdown", &qr_core::BacktestResult::max_drawdown)
        .def_readwrite("trade_count", &qr_core::BacktestResult::trade_count)
        .def_readwrite("applied_entry_z", &qr_core::BacktestResult::applied_entry_z)
        .def_readwrite("applied_stop", &qr_core::BacktestResult::applied_stop);

    // Expose the structural parameters block 
    py::class_<qr_math::GarchParameters>(m, "GarchParameters")
        .def(py::init<>())
        .def_readwrite("alpha", &qr_math::GarchParameters::alpha)
        .def_readwrite("beta", &qr_math::GarchParameters::beta)
        .def_readwrite("omega", &qr_math::GarchParameters::omega)
        .def_readwrite("gamma", &qr_math::GarchParameters::gamma);

    // Expose the structural execution calibrator engine 
    py::class_<qr_math::GarchCalibrator>(m, "GarchCalibrator")
        .def(py::init<>())
        .def("fit", &qr_math::GarchCalibrator::fit, py::arg("spreads"), py::arg("vix"));

    // ========================================================================
    // 2. Expose Core Support Prerequisites (Interfaces)
    // ========================================================================
    // Expose base provider so pybind understands the controller constructor mapping
    py::class_<qr_core::IDataProvider, std::shared_ptr<qr_core::IDataProvider>>(m, "IDataProvider");
    
    py::class_<qr_core::CSVDataProvider, qr_core::IDataProvider, std::shared_ptr<qr_core::CSVDataProvider>>(m, "CSVDataProvider")
        .def(py::init<const std::string&>())
        .def("load_all", &qr_core::CSVDataProvider::load_all)
        .def("total_ticks", &qr_core::CSVDataProvider::total_ticks);

    py::class_<qr_core::TradeLogger>(m, "TradeLogger")
        .def(py::init<const std::string&>());

    // ========================================================================
    // 3. Expose WalkForwardController with Updated Mappings
    // ========================================================================
    py::class_<qr_engine::WalkForwardController>(m, "WalkForwardController")
        // Constructor mapping bound cleanly to the IDataProvider interface reference
        .def(py::init<qr_core::IDataProvider&, qr_core::TradeLogger&>(), 
             py::keep_alive<1, 2>(), py::keep_alive<1, 3>())
        
        // Data extraction utilities mapped to return Native Numpy arrays automatically via Eigen
        .def("extract_historical_spreads", &qr_engine::WalkForwardController::extract_historical_spreads,
             py::arg("start_idx"), py::arg("end_idx"))
             
        .def("extract_historical_vix", &qr_engine::WalkForwardController::extract_historical_vix,
             py::arg("start_idx"), py::arg("end_idx"))

        // Search engine optimization layer (Now returns a native Python list of BacktestResult instances)
        .def("run_parallel_search", &qr_engine::WalkForwardController::run_parallel_search,
             py::arg("start_idx"), py::arg("end_idx"))

        // Single combinations 
        .def("evaluate_combination", &qr_engine::WalkForwardController::evaluate_combination,
             py::arg("train_start"), py::arg("train_end"), py::arg("entry_z"), py::arg("stop_loss"), py::arg("garch_params"))

        // Forward simulation validation step
        .def("execute_forward_window", &qr_engine::WalkForwardController::execute_forward_window,
             py::arg("start_idx"), py::arg("end_idx"), py::arg("entry_z"), py::arg("stop_loss"), py::arg("garch_params"));
}