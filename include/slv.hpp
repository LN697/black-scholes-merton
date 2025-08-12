#pragma once
#include <vector>
#include <cstdint>
#include <functional>
#include <cmath>
#include "option_types.hpp"
#include "stats.hpp"

namespace bsm {

struct HestonParams {
    double kappa{1.5};
    double theta{0.04};
    double xi{0.5};
    double rho{-0.7};
    double v0{0.04};
};

using LocalVolFn = std::function<double(double, double)>;

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

struct SmileLocalVol {
    double alpha{0.20};
    double beta{1.0};
    double eta{0.15};
    double zeta{0.20};
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

MCResult mc_slv_price(double S0, double K, double r, double T,
                      long num_paths, long num_steps, OptionType type,
                      const HestonParams& heston,
                      const LocalVolFn& local_vol,
                      unsigned long seed = 987654321UL,
                      bool antithetic = true,
                      bool use_andersen_qe = true);

std::vector<MCResult> mc_slv_multi_seeds(double S0, double K, double r, double T,
                                         long num_paths, long num_steps, OptionType type,
                                         const HestonParams& heston,
                                         const LocalVolFn& local_vol,
                                         const std::vector<unsigned long>& seeds,
                                         bool antithetic = true,
                                         bool use_andersen_qe = true);

}
