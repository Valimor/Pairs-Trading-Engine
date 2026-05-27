import os
import subprocess
import pandas as pd
import qr_engine_boost as qr

def run_research_pipeline(csv_file_path, train_test_split=0.6): 
    # Use C++ layer to load and parse market ticks
    provider = qr.CSVDataProvider(csv_file_path)
    provider.load_all()

    # Define operational windows
    total_ticks = provider.total_ticks()
    train_end = int(total_ticks * train_test_split)
    
    logger = qr.TradeLogger("./data/backtest_logs/execution_run.csv")
    controller = qr.WalkForwardController(provider, logger)
    
    # 1. Hand off the hyperparameter grid-search to the C++ parallel execution engine
    # This now returns a native Python list of our updated BacktestResult structs
    search_results = controller.run_parallel_search(0, train_end)
    
    if not search_results:
        print(f"Warning: No valid trades generated during grid-search for {csv_file_path}")
        return None

    # 2. Identify the optimal hyperparameter configuration based on maximizing Sharpe Ratio
    best_config = max(search_results, key=lambda x: x.sharpe_ratio)

    # 3. Extract the underlying GARCH coefficients for this historical slice to pass forward.
    # We re-run the calibration or extract the parameters directly to prevent structure-clipping.
    spreads = controller.extract_historical_spreads(0, train_end)
    vix = controller.extract_historical_vix(0, train_end)


    calibrator = qr.GarchCalibrator()
    fitted_garch = calibrator.fit(spreads, vix)

    # 4. Out-of-Sample Forward Validation Run using the flattened field names
    forward_test = controller.execute_forward_window(
        train_end, 
        total_ticks, 
        best_config.applied_entry_z, 
        best_config.applied_stop, 
        fitted_garch
    )

    # Map the decoupled variables into our data frame summary layout
    result_dict = {
        'File': os.path.basename(csv_file_path),
        'GARCH_Alpha': fitted_garch.alpha,
        'GARCH_Beta': fitted_garch.beta,
        'GARCH_Omega': fitted_garch.omega,
        'Optimal Entry Z': best_config.applied_entry_z,
        'Optimal Stop Pct': best_config.applied_stop * 100.0,
        'In-Sample Sharpe': best_config.sharpe_ratio,
        'Forward PnL ($)': forward_test.total_pnl,
        'Forward Sharpe': forward_test.sharpe_ratio,
        'Forward Max DD Pct': forward_test.max_drawdown * 100.0,
        'Executed Trades': forward_test.trade_count,
    }

    return result_dict

def run_batch_backtest():
    # Configuration
    regime_folder = "./data/historical/"
    output_csv = "./data/backtest_logs/batch_results_summary.csv"
    output_list = []
    
    if not os.path.exists(os.path.dirname(output_csv)):
        os.makedirs(os.path.dirname(output_csv))
        
    # Gather targeted data tracking regimes
    files = [f for f in os.listdir(regime_folder) if f.endswith('.csv')]
    
    if not files:
        print(f"No CSV files found in {regime_folder}")
        return

    for filename in files:
        file_path = os.path.join(regime_folder, filename)
        print(f"Processing Regime: {filename}")

        try:
            result = run_research_pipeline(file_path)
            if result is not None:
                output_list.append(result)
        except Exception as e:
            print(f"Pipeline execution failed on {filename}: {str(e)}")

    if not output_list:
        print("No successful backtests to compile.")
        return

    # Convert to Pandas DataFrame for high-level matrix aggregation
    outputs = pd.DataFrame(output_list)
    outputs.to_csv(output_csv, index=False)
    
    print(f"\n========================================================")
    print(f"Batch complete! Results written to {output_csv}")
    print(f"========================================================\n")
    print(outputs.describe().to_string())

if __name__ == "__main__":
    run_batch_backtest()