import numpy as np
import pandas as pd
import yfinance as yf
import statsmodels.api as sm
from statsmodels.tsa.stattools import coint

def analyze_pairs(data_a, data_b):
    # Convert pandas Series to raw NumPy arrays to remove ticker names
    y = data_a.values
    x = data_b.values

    # Run Cointegration Test
    score, p_value, _ = coint(y, x)
    
    # Add constant for intercept
    X = sm.add_constant(x)
    
    # Run OLS
    model = sm.OLS(y, X)
    results = model.fit()
    beta = results.params[1]
    
    # Calculate the spread using the original indices
    spread = data_a - (beta * data_b)
    
    return p_value, beta, spread

def calculate_zscore(spread):
    return (spread - spread.mean()) / np.std(spread)

def download_ticker_data(tickers, start_date, end_date):
    df = yf.download(tickers, start=start_date, end=end_date)
    if 'Adj Close' in df.columns:
        data = df['Adj Close']
    else:
        data = df['Close']
    data = data.dropna()

    return data

def download_ticker_data_vix(tickers, start_date, end_date):
    tickers.append("^VIX")
    df = yf.download(tickers, start=start_date, end=end_date)
    if 'Adj Close' in df.columns:
        data = df['Adj Close']
    else:
        data = df['Close']

    data = data.rename(columns={"^VIX": "VIX"})
    data["AvgVIX"] = data["VIX"].rolling(window=20).mean()
    data = data.dropna()

    return data

def find_cointegrated_pairs(data, tickers):
    
    n = data.shape[1]
    pairs = []
    keys = tickers
    
    # 2. Iterate through all combinations
    for i in range(n):
        for j in range(i + 1, n):
            stock1 = data.iloc[:, i]
            stock2 = data.iloc[:, j]
            
            # The coint() function returns: t-stat, p-value, critical values
            score, pvalue, _ = coint(stock1, stock2)
            
            # p-value < 0.05 means 95% confidence they are cointegrated
            if pvalue < 0.05:
                pairs.append((keys[i], keys[j], pvalue))
                
    return sorted(pairs, key=lambda x: x[2])