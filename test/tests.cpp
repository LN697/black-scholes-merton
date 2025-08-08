#include <cassert>
#include <cmath>
#include <iostream>

#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"
#include "pde_cn.hpp"
#include "pde_cn_american.hpp"
#include "slv.hpp"

using namespace bsm;

static bool approx(double a, double b, double tol) { return std::abs(a - b) <= tol; }

int main() {
    double S0 = 100.0, K = 100.0, r = 0.05, T = 1.0, sigma = 0.2;
    OptionType type = OptionType::Call;

    // Analytic vs PDE
    double analytic = black_scholes_price(S0, K, r, T, sigma, type);
    double pde = pde_crank_nicolson(S0, K, r, T, sigma, 200, 100, type);
    if (!approx(analytic, pde, 0.05)) {
        std::cerr << "PDE mismatch: analytic=" << analytic << " pde=" << pde << "\n";
        return 1;
    }

    // MC GBM vs analytic (loose bound)
    MCResult mc = mc_gbm_price(S0, K, r, T, sigma, 200000, type, 42UL, true, true, true, true, true);
    if (!approx(analytic, mc.price, 0.08)) {
        std::cerr << "MC mismatch: analytic=" << analytic << " mc=" << mc.price << "\n";
        return 1;
    }

    // American put > European put
    double euro_put = black_scholes_price(S0, K, r, T, sigma, OptionType::Put);
    double amer_put = pde_crank_nicolson_american(S0, K, r, T, sigma, 200, 100, OptionType::Put);
    if (!(amer_put >= euro_put)) {
        std::cerr << "American put not above European: amer=" << amer_put << " euro=" << euro_put << "\n";
        return 1;
    }

    // SLV runs sanity (price should be finite and positive for call)
    HestonParams h; SmileLocalVol lv; auto lv_fn = lv.to_fn();
    MCResult slv = mc_slv_price(S0, K, r, T, 100000, 128, OptionType::Call, h, lv_fn, 777UL, true, true);
    if (!(slv.price > 0 && std::isfinite(slv.price))) {
        std::cerr << "SLV price invalid: " << slv.price << "\n"; return 1; }

    std::cout << "All tests passed\n";
    return 0;
}
