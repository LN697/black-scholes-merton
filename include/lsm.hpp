#pragma once
#include <vector>
#include <functional>
#include <algorithm>
#include <cmath>
#include "option_types.hpp"

namespace bsm {

struct LSMParams {
    int steps{50};
    long paths{200000};
    int poly_degree{2};
    unsigned long seed{1234};
};

double lsm_american_put(double S0, double K, double r, double T, double sigma, const LSMParams& p);

}
