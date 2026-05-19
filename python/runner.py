import os
import subprocess
import pandas as pd
import qr_engine_boost as qr

def run_research_pipeline(csv_file_path, train_test_split = 0.6): 
    # use cpp to load the data
    provider = qr.CSVDataProvider(csv_file_path)
    provider.load_all()

    # split the data
    total_ticks = provider.total_ticks()
    train_end = int(total_ticks * train_test_split)
    
    logger = qr.TradeLogger("./data/backtest_logs/execution_run.csv")
    controller = qr.WalkForwardController(provider, logger)
    
    # Hand off the heavy computing task to the C++ std::async thread-pool
    search_results = controller.run_parallel_search(0, train_end)
    
    # Find the best configuration using a standard Python list comprehension / lambda
    best_config = max(search_results, key=lambda x: x.sharpe_ratio)

    forward_test = controller.execute_window_optimized(
        train_end, 
        total_ticks, 
        best_config.entry_z, 
        best_config.stop_loss, 
        best_config
    )

    result_dict = {
        'Alpha':best_config.alpha,
        'Beta':best_config.beta,
        'Entry Z-Score':best_config.entry_z,
        'Stop Loss':best_config.stop_loss * 100,
        'In-Sample SR':best_config.sharpe_ratio,
        'Forward Test PnL':forward_test.total_pnl,
        'Forward Test Sharpe':forward_test.sharpe_ratio,
        'Total Executed Trades':forward_test.trade_count
    }

    return result_dict

def run_batch_backtest():
    # 1. Config
    regime_folder = "./data/historical/"
    executable = "./build/backtester.exe"
    output_csv = "./data/backtest_logs/batch_results_summary.csv"
    output_list = []
    
    # Get all CSV files in the folder
    files = [f for f in os.listdir(regime_folder) if f.endswith('.csv')]
    
    if not files:
        print(f"No CSV files found in {regime_folder}")
        return

    for filename in files:
        file_path = os.path.join(regime_folder, filename)
        print(f"Processing: {file_path}")

        # 2. Execute backtest.exe with the file path as an argument
        # capture_output=True grabs the std::cout from your C++ code
        result = run_research_pipeline(file_path)
        output_list.append(result)

    outputs = pd.DataFrame(output_list)

    outputs.to_csv(output_csv)
    print(f"\nBatch complete! Results written to {output_csv}")

    print(f"\nDescription:\n{outputs.describe()}")


run_batch_backtest()