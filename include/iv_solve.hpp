#pragma once
#include <functional>
#include <cmath>

namespace bsm {

inline double implied_vol(double target_price, std::function<double(double)> price_fn,
                          double lo = 1e-6, double hi = 5.0, int max_iter = 100, double tol = 1e-8) {
    double f_lo = price_fn(lo) - target_price;
    double f_hi = price_fn(hi) - target_price;
    if (std::isnan(f_lo) || std::isnan(f_hi)) return std::numeric_limits<double>::quiet_NaN();
    if (f_lo * f_hi > 0.0) {
        // Not bracketed; simple expand
        for (int k = 0; k < 10 && f_lo * f_hi > 0.0; ++k) {
            hi *= 2.0; f_hi = price_fn(hi) - target_price;
        }
        if (f_lo * f_hi > 0.0) return std::numeric_limits<double>::quiet_NaN();
    }
    double a = lo, b = hi, fa = f_lo, fb = f_hi;
    for (int it = 0; it < max_iter; ++it) {
        double c = (std::abs(fb - fa) > 1e-14) ? (b - fb * (b - a) / (fb - fa)) : 0.5 * (a + b);
        if (!(c > 0.0)) c = 0.5 * (a + b);
        double fc = price_fn(c) - target_price;
        if (std::abs(fc) < tol) return c;
        // Keep bracket
        if (fa * fc <= 0.0) { b = c; fb = fc; }
        else { a = c; fa = fc; }
        // Fallback to bisection occasionally
        if (it % 5 == 0) {
            double mid = 0.5 * (a + b);
            double fm = price_fn(mid) - target_price;
            if (fa * fm <= 0.0) { b = mid; fb = fm; } else { a = mid; fa = fm; }
            if (std::abs(b - a) < tol * (1.0 + std::abs(mid))) return 0.5 * (a + b);
        }
    }
    return 0.5 * (a + b);
}

}
