#pragma once
#include <vector>
#include <cstdint>
#include <functional>
#include <cmath>
#include "option_types.hpp"
#include "stats.hpp"

namespace bsm {

struct HestonParams {
    double kappa{1.5};   // mean reversion speed
    double theta{0.04};  // long-run variance
    double xi{0.5};      // vol of vol
    double rho{-0.7};    // correlation between asset and variance
    double v0{0.04};     // initial variance
};

// Local volatility function type: sigma_loc(S, t)
using LocalVolFn = std::function<double(double, double)>;

// CEV-like local volatility (skew control via beta)
struct CEVLocalVol {
    double alpha{0.20};
    double beta{1.0};
    double Sref{100.0};
    double sigma(double S, double /*t*/) const {
        double ratio = (Sref > 0 ? S / Sref : 1.0);
        return alpha * std::pow(std::max(ratio, 1e-12), beta - 1.0);
    }
    LocalVolFn to_fn() const {
        return [*this](double S, double t) { (void)t; return this->sigma(S, t); };
    }
};

// Smile/skew local volatility: adds log-moneyness tilt and mild term structure
struct SmileLocalVol {
    double alpha{0.20};   // base level
    double beta{1.0};     // CEV skew control
    double eta{0.15};     // log-moneyness tilt strength (skew/smile)
    double zeta{0.20};    // term structure factor
    double Sref{100.0};
    double sigma_min{0.01};
    double sigma(double S, double t) const {
        double x = (Sref > 0 ? std::log(std::max(S, 1e-12) / Sref) : 0.0);
        double cev = alpha * std::pow(std::max(Sref > 0 ? S / Sref : 1.0, 1e-12), beta - 1.0);
        double smile = 1.0 + eta * x;
        double term = std::sqrt(std::max(1.0 + zeta * t, 1e-12));
        double s = std::max(sigma_min, std::abs(cev * smile) * term);
        return s;
    }
    LocalVolFn to_fn() const {
        return [*this](double S, double t) { return this->sigma(S, t); };
    }
};

// SLV Monte Carlo pricer (demonstration):
// dS = r S dt + sigma_eff(S,t,v) S dW1
// dv = kappa (theta - v) dt + xi sqrt(max(v,0)) dW2
// with corr(dW1, dW2) = rho and sigma_eff = sigma_loc(S,t) * sqrt(max(v,0))
MCResult mc_slv_price(double S0, double K, double r, double T,
                      long num_paths, long num_steps, OptionType type,
                      const HestonParams& heston,
                      const LocalVolFn& local_vol,
                      unsigned long seed = 987654321UL,
                      bool antithetic = true,
                      bool use_andersen_qe = true);

// Utility: run across multiple random seeds and return per-seed prices
std::vector<MCResult> mc_slv_multi_seeds(double S0, double K, double r, double T,
                                         long num_paths, long num_steps, OptionType type,
                                         const HestonParams& heston,
                                         const LocalVolFn& local_vol,
                                         const std::vector<unsigned long>& seeds,
                                         bool antithetic = true,
                                         bool use_andersen_qe = true);

}
