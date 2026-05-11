import yfinance as yf
import numpy as np
import pandas as pd
from analysis import *

# TODO: take a bunch of tickers and find the best pairs

# 1. Fetch Data
# PAIRS: NVDA SMM (2024-01-01 to 2026-01-01)
#       KRE KBE (2023-01-01 to 2026-01-01)
#       KO PEP (2014-01-01 to 2020-01-01)
#


print("fetching data...")
tickers = ['AMAT', 'XOM']
# Setting group_by='column' is the default, but we'll specify the price type clearly
df = yf.download(tickers, start="2024-01-01", end="2026-05-01")

# If 'Adj Close' is missing, fallback to 'Close'
print("checking for Adj Close")
if 'Adj Close' in df.columns:
    data = df['Adj Close']
else:
    data = df['Close']

# Ensure we have the data for our specific tickers
print("checking our tickers are present")
data = data[tickers].dropna()

# 2. Run Analysis
print("running analysis...")
p_val, beta, spread = analyze_pairs(data[tickers[0]], data[tickers[1]])
print(f"P value: {p_val}")

if p_val < 0.05:
    print(f"Pairs are cointegrated (p={p_val:.4f}). Hedge Ratio: {beta:.4f}")
    
    # 3. Prep data for C++
    combined_df = data.copy()
    combined_df['zscore'] = calculate_zscore(spread)
    
    # Save to CSV for the C++ backtester
    combined_df.to_csv(f'data/regimes/{tickers[0]}_{tickers[1]}_market_data.csv')
    print(f"Data exported to data/regimes/{tickers[0]}_{tickers[1]}_market_data.csv")
else:
    print("Pairs are not cointegrated. Trading not recommended.")