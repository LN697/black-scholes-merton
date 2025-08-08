#include "monte_carlo_gbm.hpp"
#include "math_utils.hpp"
#include <algorithm>
#include <cmath>

namespace bsm {

MCResult mc_gbm_price(double S0, double K, double r, double T, double sigma,
                      long num_paths, OptionType type, unsigned long seed, bool antithetic,
                      bool control_variate, bool use_qmc, bool two_pass_cv, bool compute_greeks) {
    RNG rng(seed);
    Halton2D hal(seed + 17);
    const double drift = (r - 0.5 * sigma * sigma) * T;
    const double volT = sigma * std::sqrt(T);

    double sum = 0.0, sum2 = 0.0;

    auto payoff = [&](double ST) {
        return (type == OptionType::Call) ? std::max(ST - K, 0.0) : std::max(K - ST, 0.0);
    };
    auto delta_pathwise = [&](double ST){
        if (type == OptionType::Call) return (ST > K) ? (ST / S0) : 0.0;
        else return (ST < K) ? -(ST / S0) : 0.0;
    };
    auto vega_lrm = [&](double Z, double ST){
        // Likelihood ratio for GBM with lognormal terminal distribution
        // d ln f / d sigma = (Z * sqrt(T) - sigma*T)
        double dlogf = Z * std::sqrt(T) - sigma * T;
        return payoff(ST) * dlogf;
    };

    // Optional two-pass beta estimation for control variate (X=payoff, Y=ST)
    double beta = 0.0;
    double Ey = S0 * std::exp(r * T);
    if (control_variate && two_pass_cv) {
        double sumX=0.0, sumY=0.0, sumXY=0.0, sumYY=0.0;
        for (long i = 0; i < std::min<long>(num_paths, 200000); ++i) {
            double Z;
            if (use_qmc) { double u1,u2,z1,z2; hal.next(u1,u2); box_muller(u1,u2,z1,z2); Z=z1; }
            else { Z = rng.gauss(); }
            double ST = S0 * std::exp(drift + volT * Z);
            double X = payoff(ST);
            double Y = ST;
            sumX += X; sumY += Y; sumXY += X*Y; sumYY += Y*Y;
        }
        double n = static_cast<double>(std::min<long>(num_paths, 200000));
        double covXY = sumXY/n - (sumX/n)*(sumY/n);
        double varY = sumYY/n - (sumY/n)*(sumY/n);
        if (varY > 1e-14) beta = covXY/varY;
    }

    // Accumulators for Greeks and CV
    double sum_delta=0.0, sum_delta2=0.0;
    double sum_vega=0.0, sum_vega2=0.0;

    #ifdef _OPENMP
    #pragma omp parallel for reduction(+:sum,sum2,sum_delta,sum_delta2,sum_vega,sum_vega2)
    #endif
    for (long i = 0; i < num_paths; ++i) {
        double Z;
        if (use_qmc) {
            double u1, u2, z1, z2; hal.next(u1, u2); box_muller(u1, u2, z1, z2); Z = z1;
        } else {
            Z = rng.gauss();
        }
        double ST = S0 * std::exp(drift + volT * Z);
        double p = payoff(ST);
        if (antithetic) {
            double STa = S0 * std::exp(drift - volT * Z);
            p = 0.5 * (p + payoff(STa));
        }
        if (control_variate) {
            double adj = p - beta * (ST - Ey);
            p = adj;
        }
        sum += p;
        sum2 += p * p;

        if (compute_greeks) {
            double d = delta_pathwise(ST);
            double v = vega_lrm(Z, ST);
            sum_delta += d; sum_delta2 += d*d;
            sum_vega  += v; sum_vega2  += v*v;
        }
    }

    double disc = std::exp(-r * T);
    double mean_payoff = sum / static_cast<double>(num_paths);
    double var_payoff = (sum2 / static_cast<double>(num_paths)) - mean_payoff * mean_payoff;
    var_payoff = std::max(0.0, var_payoff);

    MCResult res;
    res.price = disc * mean_payoff;
    res.std_error = disc * std::sqrt(var_payoff / static_cast<double>(num_paths));
    if (compute_greeks) {
        double n = static_cast<double>(num_paths);
        double mD = sum_delta/n, vD = std::max(0.0, sum_delta2/n - mD*mD);
        double mV = sum_vega/n,  vV = std::max(0.0, sum_vega2/n  - mV*mV);
        res.delta = mD;
        res.delta_se = std::sqrt(vD/n);
        res.vega = disc * mV; // LR vega already scales to price sensitivity
        res.vega_se = disc * std::sqrt(vV/n);
    }
    return res;
}

}
