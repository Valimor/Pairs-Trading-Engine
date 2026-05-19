Suggested Execution Order

    Parallelize: Write the C++ std::async engine optimization loop to speed up the hyperparameter grid search.
    
    Fix the Core Orchestrator: Complete the structural logic that isolates MLE parameter fitting from out-of-sample execution.

    Add Frictions: Inject basic transaction cost deductions into your PnL math so your data reflects financial realities.

    Mathematical Upgrades: Implement the GARCH-X or Student's t-distribution math layer inside Eigen.

    Expand your GARCH variance equation to include an external signal:$$\sigma^2_t = \omega + \alpha \epsilon^2_{t-1} + \beta \sigma^2_{t-1} + \gamma \text{VIX}_{t-1}$$

    maybe move code from the walk_forwardcontroller.hpp to a .cpp in engines?