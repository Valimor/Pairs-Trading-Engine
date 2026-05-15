import os
import subprocess
import pandas as pd

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
        result = subprocess.run(
            [executable, file_path], 
            capture_output=True, 
            text=True, 
            check=True
        )
        
        substrings = filename.split("_")
        ticker1 = substrings[0]
        ticker2 = substrings[1]
        results = result.stdout.split(",")
        pnl = float(results[0])
        alpha = float(results[1])
        beta = float(results[2])
        output_list.append({'Ticker1':ticker1, 'Ticker2':ticker2, 'PnL':pnl, 'Alpha':alpha, 'Beta':beta})

    outputs = pd.DataFrame(output_list)
    outputs = outputs.sort_values(by='PnL')

    outputs.to_csv(output_csv)
    print(f"\nBatch complete! Results written to {output_csv}")

    print(f"\nDescription:\n{outputs.describe()}")

if __name__ == "__main__":
    run_batch_backtest()