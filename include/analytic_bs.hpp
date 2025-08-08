#pragma once
#include "option_types.hpp"

namespace bsm {

// Analytical Black-Scholes price for European call/put
// Returns option price
double black_scholes_price(double S0, double K, double r, double T, double sigma, OptionType type);

}
