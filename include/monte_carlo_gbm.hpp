#pragma once
#include "option_types.hpp"
#include "stats.hpp"

namespace bsm {

// Monte Carlo pricer under GBM with optional antithetics, control variate, and QMC (Halton)
MCResult mc_gbm_price(double S0, double K, double r, double T, double sigma,
                      long num_paths, OptionType type, unsigned long seed = 12345,
                      bool antithetic = true, bool control_variate = true, bool use_qmc = false,
                      bool two_pass_cv = true, bool compute_greeks = true);

}
