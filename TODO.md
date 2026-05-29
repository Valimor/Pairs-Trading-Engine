---
title: "TODO"
date: 2026-05-26
author: Max McCune
---

Suggested Execution Order

~~Fix the Core Orchestrator: Complete the structural logic that isolates MLE parameter fitting from out-of-sample execution.~~

~~Clean up the comments so it's less clearly AI. also read so i understand~~
NOTE: WILL NEED TO REDO THIS EVERY SO OFTEN ^^^^^

~~Implement the python efficiency idea that gemini suggested~~

Add Frictions: Inject basic transaction cost deductions into your PnL math so your data reflects financial realities.

Mathematical Upgrades: Implement the Student's t-distribution math layer inside Eigen.

Look into OpenMP

~~Expand your GARCH variance equation to include an external signal:$$\sigma^2_t = \omega + \alpha \epsilon^2_{t-1} + \beta \sigma^2_{t-1} + \gamma \text{VIX}_{t-1}$$~~

Gemini to do list for ML
- [ ] Implement PCA on a 1-year window of daily stock returns to extract asset factor loadings.
- [ ] Pipe PCA components into an OPTICS clustering script to generate isolated asset pairs.
- [ ] Train a 2-state Hidden Markov Model (HMM) in Python to categorize historical spread regimes.
- [ ] Export the HMM transition matrices to your C++ backend to pause trading during "Trending" states.
- [ ] Build an XGBoost feature-engineering pipeline in Python utilizing order book depth and volume metrics.
- [ ] Integrate ONNX Runtime into your C++ backend to run real-time inference on the tree-based classifier.
- [ ] Design a Reinforcement Learning environment using Gym/Stable-Baselines to train an execution policy.
- [ ] Embed the trained RL policy network into your C++ thread loop for optimized microsecond entry decisions.