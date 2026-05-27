import os
import pandas as pd
import qr_engine_boost as qr

def run_research_pipeline(csv_file_path, train_test_split=0.6): 
    provider = qr.CSVDataProvider(csv_file_path)
    provider.load_all()

    total_ticks = provider.total_ticks()
    train_end = int(total_ticks * train_test_split)
    
    logger = qr.TradeLogger("./data/backtest_logs/execution_run.csv")
    controller = qr.WalkForwardController(provider, logger)
    
    # 1. In-Sample optimization matrix sweep (C++ multithreaded layer)
    search_results = controller.run_parallel_search(0, train_end)
    
    if not search_results:
        print(f"Warning: No valid trades generated during grid-search for {csv_file_path}")
        return None

    # 2. Extract optimal hyperparameters
    best_config = max(search_results, key=lambda x: x.sharpe_ratio)

    # 3. Out-of-Sample Forward Validation 
    # (Letting C++ handle calibration internally prevents cross-bridge data copy overhead)
    forward_test = controller.execute_forward_window(
        0,
        train_end, 
        train_end,
        total_ticks, 
        best_config.applied_entry_z, 
        best_config.applied_stop
    )

    return {
        'File': os.path.basename(csv_file_path),
        'Optimal Entry Z': best_config.applied_entry_z,
        'Optimal Stop Pct': best_config.applied_stop * 100.0,
        'In-Sample Sharpe': best_config.sharpe_ratio,
        'Forward PnL ($)': forward_test.total_pnl,
        'Forward Sharpe': forward_test.sharpe_ratio,
        'Forward Max DD Pct': forward_test.max_drawdown * 100.0,
        'Executed Trades': forward_test.trade_count,
    }

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