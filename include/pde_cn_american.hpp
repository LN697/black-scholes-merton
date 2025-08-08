#pragma once
#include "option_types.hpp"

namespace bsm {

// Crank-Nicolson with early exercise (projected).
double pde_crank_nicolson_american(double S0, double K, double r, double T, double sigma,
                                   int num_S_steps, int num_T_steps, OptionType type);

}
