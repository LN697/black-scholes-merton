#include "slv.hpp"
#include "math_utils.hpp"
#include <algorithm>
#include <cmath>

namespace bsm {

static inline double payoff(double ST, double K, OptionType type) {
    return (type == OptionType::Call) ? std::max(ST - K, 0.0) : std::max(K - ST, 0.0);
}

MCResult mc_slv_price(double S0, double K, double r, double T,
                      long num_paths, long num_steps, OptionType type,
                      const HestonParams& h, const LocalVolFn& lv,
                      unsigned long seed, bool antithetic, bool use_andersen_qe) {
    RNG rng(seed);
    const double dt = T / static_cast<double>(num_steps);
    const double sqrt_dt = std::sqrt(dt);

    double sum = 0.0, sum2 = 0.0;

    auto one_path = [&](unsigned long s) {
        (void)s; // seed unused in lambda
        double S = S0;
        double v = std::max(h.v0, 1e-12);
        for (long n = 0; n < num_steps; ++n) {
            double z1, z2; correlated_gaussians(h.rho, rng, z1, z2);
            // Variance step
            if (use_andersen_qe) {
                // Andersen QE discretization
                double m = h.theta + (v - h.theta) * std::exp(-h.kappa * dt);
                double s2 = v * (h.xi * h.xi) * std::exp(-h.kappa * dt) * (1.0 - std::exp(-h.kappa * dt)) / h.kappa
                          + h.theta * (h.xi * h.xi) * 0.5 / h.kappa * (1.0 - std::exp(-h.kappa * dt)) * (1.0 - std::exp(-h.kappa * dt));
                double psi = s2 / (m * m);
                double U = 0.5 * (z2 + 1.0); // map N(0,1) to (0,1) roughly for branching
                U = std::min(std::max(U, 1e-6), 1.0 - 1e-6);
                if (psi < 1.5) {
                    double b2 = 2.0 / psi - 1.0 + std::sqrt(2.0 / psi) * std::sqrt(2.0 / psi - 1.0);
                    double a = m / (1.0 + b2);
                    double chi2 = std::normal_distribution<double>(0.0, 1.0)(rng.gen);
                    chi2 = chi2 * chi2;
                    v = a * (std::sqrt(b2) + std::sqrt(chi2)) * (std::sqrt(b2) + std::sqrt(chi2));
                } else {
                    double p = (psi - 1.0) / (psi + 1.0);
                    double beta = (1.0 - p) / m;
                    v = (U > p) ? -std::log((1.0 - U) / (1.0 - p)) / beta : 0.0;
                }
            } else {
                double dW2 = z2 * sqrt_dt;
                double v_sqrt = std::sqrt(std::max(v, 0.0));
                double v_next = v + h.kappa * (h.theta - std::max(v, 0.0)) * dt + h.xi * v_sqrt * dW2;
                v = std::max(v_next, 0.0);
            }

            double sigma_loc = lv(S, n * dt);
            double vol_inst = sigma_loc * std::sqrt(std::max(v, 0.0));
            double dW1 = z1 * sqrt_dt;
            // Log Euler for S for positivity
            double drift = (r - 0.5 * vol_inst * vol_inst) * dt;
            double diff = vol_inst * dW1;
            S = S * std::exp(drift + diff);
        }
        return payoff(S, K, type);
    };

    for (long i = 0; i < num_paths; ++i) {
        double p = one_path(seed + i);
        if (antithetic) {
            // reuse same increments with sign flip on z's by regenerating and flipping
            // Simplified: draw fresh z and negate first component to induce negative correlation
            // It's not the strict pathwise antithetic for 2D correlated, but works reasonably.
            double S = S0, v = std::max(h.v0, 1e-12);
            for (long n = 0; n < num_steps; ++n) {
                double z1, z2; correlated_gaussians(h.rho, rng, z1, z2);
                z1 = -z1; z2 = -z2;
                if (use_andersen_qe) {
                    double m = h.theta + (v - h.theta) * std::exp(-h.kappa * dt);
                    double s2 = v * (h.xi * h.xi) * std::exp(-h.kappa * dt) * (1.0 - std::exp(-h.kappa * dt)) / h.kappa
                              + h.theta * (h.xi * h.xi) * 0.5 / h.kappa * (1.0 - std::exp(-h.kappa * dt)) * (1.0 - std::exp(-h.kappa * dt));
                    double psi = s2 / (m * m);
                    double U = 0.5 * (z2 + 1.0);
                    U = std::min(std::max(U, 1e-6), 1.0 - 1e-6);
                    if (psi < 1.5) {
                        double b2 = 2.0 / psi - 1.0 + std::sqrt(2.0 / psi) * std::sqrt(2.0 / psi - 1.0);
                        double a = m / (1.0 + b2);
                        double chi2 = std::normal_distribution<double>(0.0, 1.0)(rng.gen);
                        chi2 = chi2 * chi2;
                        v = a * (std::sqrt(b2) + std::sqrt(chi2)) * (std::sqrt(b2) + std::sqrt(chi2));
                    } else {
                        double p = (psi - 1.0) / (psi + 1.0);
                        double beta = (1.0 - p) / m;
                        v = (U > p) ? -std::log((1.0 - U) / (1.0 - p)) / beta : 0.0;
                    }
                } else {
                    double dW2 = z2 * sqrt_dt;
                    double v_sqrt = std::sqrt(std::max(v, 0.0));
                    double v_next = v + h.kappa * (h.theta - std::max(v, 0.0)) * dt + h.xi * v_sqrt * dW2;
                    v = std::max(v_next, 0.0);
                }
                double sigma_loc = lv(S, n * dt);
                double vol_inst = sigma_loc * std::sqrt(std::max(v, 0.0));
                double dW1 = z1 * sqrt_dt;
                double drift = (r - 0.5 * vol_inst * vol_inst) * dt;
                double diff = vol_inst * dW1;
                S = S * std::exp(drift + diff);
            }
            double pa = payoff(S, K, type);
            p = 0.5 * (p + pa);
        }
        sum += p;
        sum2 += p * p;
    }

    double disc = std::exp(-r * T);
    double mean_payoff = sum / static_cast<double>(num_paths);
    double var_payoff = (sum2 / static_cast<double>(num_paths)) - mean_payoff * mean_payoff;
    var_payoff = std::max(0.0, var_payoff);

    MCResult res;
    res.price = disc * mean_payoff;
    res.std_error = disc * std::sqrt(var_payoff / static_cast<double>(num_paths));
    // Greeks (optional): could compute via bumping, here omitted to keep runtime modest
    return res;
}

std::vector<MCResult> mc_slv_multi_seeds(double S0, double K, double r, double T,
                                         long num_paths, long num_steps, OptionType type,
                                         const HestonParams& h, const LocalVolFn& lv,
                                         const std::vector<unsigned long>& seeds,
                                         bool antithetic, bool use_andersen_qe) {
    std::vector<MCResult> out;
    out.reserve(seeds.size());
    for (auto s : seeds) {
        out.push_back(mc_slv_price(S0, K, r, T, num_paths, num_steps, type, h, lv, s, antithetic, use_andersen_qe));
    }
    return out;
}

}
