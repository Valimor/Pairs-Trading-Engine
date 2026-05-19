import sys
import os

print("Current Python version:", sys.version)

# Ensure Python can see the directory containing qr_engine_boost.pyd
sys.path.append(os.getcwd())

# Import your compiled C++ engine!
import qr_engine_boost as qr

def run_research_pipeline(csv_file_path):
    print("[Python] Initializing high-performance C++ components...")
    
    # 1. Instantiate the C++ backend infrastructure classes
    provider = qr.CSVDataProvider(csv_file_path)
    provider.load_all()
    
    total_ticks = provider.total_ticks()
    train_end = int(total_ticks * 0.6)
    
    logger = qr.TradeLogger("./data/backtest_logs/execution_run.csv")
    controller = qr.WalkForwardController(provider, logger)
    
    # 2. Hand off the heavy computing task to the C++ std::async thread-pool
    print(f"[Python] Launching parallelized parameter grid search in C++ over {train_end} ticks...")
    search_results = controller.run_parallel_search(0, train_end)
    
    # 3. Process the results back in Python
    print(f"[Python] Parsing {len(search_results)} parameter combinations...")
    
    # Find the best configuration using a standard Python list comprehension / lambda
    best_config = max(search_results, key=lambda x: x.sharpe_ratio)
    
    print("\n" + "="*40)
    print("      OPTIMAL HYPERPARAMETERS FOUND BY C++")
    print("="*40)
    print(f"GARCH Alpha   : {best_config.alpha:.4f}")
    print(f"GARCH Beta    : {best_config.beta:.4f}")
    print(f"Entry Z-Score : {best_config.entry_z:.2f}")
    print(f"Stop Loss     : {best_config.stop_loss * 100:.1f}%")
    print(f"In-Sample SR  : {best_config.sharpe_ratio:.4f}")
    print("="*40 + "\n")
    
    # 4. Out-of-sample forward verification pass back inside the C++ engine
    print("[Python] Running Out-of-Sample validation phase in C++...")
    forward_test = controller.execute_window_optimized(
        train_end, 
        total_ticks, 
        best_config.entry_z, 
        best_config.stop_loss, 
        best_config
    )
    
    print(f"[Result] Forward Test PnL      : ${forward_test.total_pnl:,.2f}")
    print(f"         Forward Test Sharpe   : {forward_test.sharpe_ratio:.4f}")
    print(f"         Total Executed Trades : {forward_test.trade_count}")

if __name__ == "__main__":
    run_research_pipeline("./data/historical/ADI_GILD_market_data.csv")
    run_research_pipeline("./data/historical/AMAT_CL_market_data.csv")