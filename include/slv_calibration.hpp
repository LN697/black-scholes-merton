#pragma once
#include <vector>
#include <functional>
#include "dupire.hpp"
#include "slv.hpp"

namespace bsm {

struct LeverageGrid {
    std::vector<double> t;
    std::vector<double> S;
    std::vector<std::vector<double>> L; // same shape as DupireSurface.sigma
};

// Skeleton iterative calibration: updates L on a grid by comparing model-implied local vol to target Dupire
// NOTE: For brevity, this is a stub. In production, compute sigma_model(S,t) via PDE or particle estimator.
inline void calibrate_leverage_iterative(const DupireSurface& target, const HestonParams& h,
                                         LeverageGrid& lev, int iterations = 5) {
    (void)h;
    for (int iter = 0; iter < iterations; ++iter) {
        for (size_t j = 0; j < lev.t.size(); ++j) {
            for (size_t i = 0; i < lev.S.size(); ++i) {
                double sig_target = target.sigma[j][i];
                double sig_model = sig_target; // placeholder; assume perfect match per iter
                double eps = 1e-6;
                lev.L[j][i] = lev.L[j][i] * sig_target / std::max(sig_model, eps);
            }
        }
    }
}

}
