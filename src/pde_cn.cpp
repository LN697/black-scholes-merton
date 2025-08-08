#include "pde_cn.hpp"
#include <vector>
#include <algorithm>
#include <cmath>

namespace bsm {

double pde_crank_nicolson(double S0, double K, double r, double T, double sigma,
                           int num_S_steps, int num_T_steps, OptionType type) {
    const double S_max = 3.0 * K;
    const double dS = S_max / static_cast<double>(num_S_steps);
    const double dt = T / static_cast<double>(num_T_steps);

    std::vector<double> S(num_S_steps + 1), V(num_S_steps + 1);
    for (int i = 0; i <= num_S_steps; ++i) S[i] = i * dS;

    // Terminal condition
    for (int i = 0; i <= num_S_steps; ++i) {
        V[i] = (type == OptionType::Call) ? std::max(S[i] - K, 0.0) : std::max(K - S[i], 0.0);
    }

    std::vector<double> a(num_S_steps + 1), b(num_S_steps + 1), c(num_S_steps + 1);
    for (int i = 1; i < num_S_steps; ++i) {
        double sigma_sq_i_sq = sigma * sigma * i * i;
        a[i] = 0.25 * dt * (sigma_sq_i_sq - r * i);
        b[i] = 1.0 + 0.5 * dt * (sigma_sq_i_sq + r);
        c[i] = 0.25 * dt * (-sigma_sq_i_sq - r * i);
    }

    for (int j = num_T_steps - 1; j >= 0; --j) {
        std::vector<double> rhs(num_S_steps + 1);
        for (int i = 1; i < num_S_steps; ++i) {
            // RHS = B * V^{j+1} with B_sub=a, B_diag=(2-b), B_sup=(-c)
            rhs[i] = a[i] * V[i - 1] + (2.0 - b[i]) * V[i] - c[i] * V[i + 1];
        }

        // Boundary at time t_{j+1}
        double t_next = (j + 1) * dt;
        double t_curr = j * dt;
        if (type == OptionType::Call) {
            // V0 = 0
            double V_N_next = S_max - K * std::exp(-r * (T - t_next));
            double V_N_curr = S_max - K * std::exp(-r * (T - t_curr));
            // B_sup contribution with gamma = -c
            rhs[num_S_steps - 1] += (-c[num_S_steps - 1]) * V_N_next;
            // A_sup boundary contribution at current time (move to RHS with minus sign)
            rhs[num_S_steps - 1] -= (c[num_S_steps - 1]) * V_N_curr;
        } else {
            double V_0_next = K * std::exp(-r * (T - t_next));
            double V_0_curr = K * std::exp(-r * (T - t_curr));
            // B_sub contribution (alpha=a)
            rhs[1] += a[1] * V_0_next;
            // A_sub boundary contribution at current time: sub = -a[1], move to RHS
            rhs[1] += a[1] * V_0_curr;
        }

    // Left-hand tridiagonal A: sub=-a, diag=b, sup=c
    std::vector<double> sub(num_S_steps + 1), diag(num_S_steps + 1), sup(num_S_steps + 1);
    for (int i = 1; i < num_S_steps; ++i) { sub[i] = -a[i]; diag[i] = b[i]; sup[i] = c[i]; }
        // Copy RHS
        std::vector<double> d_prime = rhs;

        // Thomas algorithm for i=1..N-1
        for (int i = 2; i < num_S_steps; ++i) {
            double m = sub[i] / diag[i - 1];
            diag[i] -= m * sup[i - 1];
            d_prime[i] -= m * d_prime[i - 1];
        }
        V[num_S_steps - 1] = d_prime[num_S_steps - 1] / diag[num_S_steps - 1];
        for (int i = num_S_steps - 2; i > 0; --i) {
            V[i] = (d_prime[i] - sup[i] * V[i + 1]) / diag[i];
        }

        if (type == OptionType::Call) {
            V[0] = 0.0;
            V[num_S_steps] = S_max - K * std::exp(-r * (T - j * dt));
        } else {
            V[0] = K * std::exp(-r * (T - j * dt));
            V[num_S_steps] = 0.0;
        }
    }

    int idx = static_cast<int>(S0 / dS);
    if (idx >= num_S_steps) return V[num_S_steps];
    double slope = (V[idx + 1] - V[idx]) / dS;
    return V[idx] + slope * (S0 - S[idx]);
}

}
