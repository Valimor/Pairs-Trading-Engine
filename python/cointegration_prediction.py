import yfinance as yf
import numpy as np
import pandas as pd
from analysis import *

write_to_csv = True

universe = ['XOM', 'CVX', 'BP', 'SHEL', 'JPM', 'BAC', 'WFC', 'MS', 'KO', 'PEP', 'PG', 'GOOGL', 
            'META', 'KRE', 'KBE', 'AVGO', 'QCOM', 'KLAC', 'AMAT', 'LRCX', 'ASML', 'INTC', 'AMD',
            'CL', 'MNST', 'KDP', 'MS', 'V', 'XLF', 'NEE', 'XLU', 'XOM', 'CVX', 'UPS', 'FDX', 'HD',
            'LOW', 'TXN', 'ADI', 'GILD', 'BIIB']

# START BY CHECKING EXISTING STOCKS

old_data = download_ticker_data_vix(universe, '2022-01-01', '2024-01-01')
results = find_cointegrated_pairs(old_data, universe)

# RUN THE TRADES ON NEW DATA

data = download_ticker_data_vix(universe, '2024-01-01', '2026-05-01')

print("--- Top Cointegrated Pairs ---")
for p in results:
    print(f"{p[0]} & {p[1]} | historical p-value: {p[2]:.4f}")

    if write_to_csv:
        # get the spread and p val (spread is all we care about)
        p_val, beta, spread = analyze_pairs(data[p[0]], data[p[1]])

        # slice off the part of the df we want
        combined_df = data.copy()
        combined_df = combined_df[[p[0], p[1], "VIX", "AvgVIX"]]
        combined_df['zscore'] = calculate_zscore(spread) # this actually goes entirely unused, but it's fun to have
        
        # export the data
        combined_df.to_csv(f'data/regimes/{p[0]}_{p[1]}_market_data.csv')

if write_to_csv:
    print(f"Data exported to data/regimes/")