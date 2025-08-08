#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>
#include "slv.hpp"

namespace bsm {

struct DupireSurface {
    // Simple rectangular grid storage: times t[0..Nt-1], spots S[0..Ns-1], and sigma_loc[t][s]
    std::vector<double> t;
    std::vector<double> S;
    std::vector<std::vector<double>> sigma; // size Nt x Ns

    double at(size_t it, size_t is) const { return sigma[it][is]; }

    double bilinear(double St, double tt) const {
        if (t.empty() || S.empty()) return 0.0;
        if (tt <= t.front()) {
            if (St <= S.front()) return sigma.front().front();
            if (St >= S.back()) return sigma.front().back();
            auto itS = std::upper_bound(S.begin(), S.end(), St) - S.begin();
            size_t i1 = std::max<size_t>(1, itS) - 1, i2 = std::min(i1 + 1, S.size() - 1);
            double w = (St - S[i1]) / std::max(1e-12, S[i2] - S[i1]);
            return (1 - w) * sigma.front()[i1] + w * sigma.front()[i2];
        }
        if (tt >= t.back()) {
            if (St <= S.front()) return sigma.back().front();
            if (St >= S.back()) return sigma.back().back();
            auto itS = std::upper_bound(S.begin(), S.end(), St) - S.begin();
            size_t i1 = std::max<size_t>(1, itS) - 1, i2 = std::min(i1 + 1, S.size() - 1);
            double w = (St - S[i1]) / std::max(1e-12, S[i2] - S[i1]);
            return (1 - w) * sigma.back()[i1] + w * sigma.back()[i2];
        }
        auto itT = std::upper_bound(t.begin(), t.end(), tt) - t.begin();
        size_t j1 = std::max<size_t>(1, itT) - 1, j2 = std::min(j1 + 1, t.size() - 1);
        double wt = (tt - t[j1]) / std::max(1e-12, t[j2] - t[j1]);
        auto itS = std::upper_bound(S.begin(), S.end(), St) - S.begin();
        size_t i1 = std::max<size_t>(1, itS) - 1, i2 = std::min(i1 + 1, S.size() - 1);
        double ws = (St - S[i1]) / std::max(1e-12, S[i2] - S[i1]);
        double v11 = sigma[j1][i1], v12 = sigma[j1][i2], v21 = sigma[j2][i1], v22 = sigma[j2][i2];
        double v1 = (1 - ws) * v11 + ws * v12;
        double v2 = (1 - ws) * v21 + ws * v22;
        return (1 - wt) * v1 + wt * v2;
    }
};

// Intentionally no shortcut leverage approximation in production; use an iterative calibration loop.

}
