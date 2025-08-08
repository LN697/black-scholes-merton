#pragma once
#include "option_types.hpp"

namespace bsm {

// Crank-Nicolson PDE solver for European options
// num_S_steps: grid points in S; num_T_steps: time steps
// Returns V(S0, 0)
double pde_crank_nicolson(double S0, double K, double r, double T, double sigma,
                           int num_S_steps, int num_T_steps, OptionType type);

}
