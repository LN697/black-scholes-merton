#include "lsm.hpp"
#include <random>

namespace bsm {

double lsm_american_put(double S0, double K, double r, double T, double sigma, const LSMParams& p) {
    int N = p.steps;
    long M = p.paths;
    double dt = T / N;
    std::mt19937_64 gen(p.seed);
    std::normal_distribution<double> nd(0.0, 1.0);

    // Simulate paths
    std::vector<std::vector<double>> S(N + 1, std::vector<double>(M));
    for (long m = 0; m < M; ++m) S[0][m] = S0;
    double drift = (r - 0.5 * sigma * sigma) * dt;
    double vol = sigma * std::sqrt(dt);
    for (int n = 1; n <= N; ++n) {
        for (long m = 0; m < M; ++m) {
            double Z = nd(gen);
            S[n][m] = S[n - 1][m] * std::exp(drift + vol * Z);
        }
    }

    // Initialize cashflows as immediate put payoff at maturity
    std::vector<double> CF(M);
    for (long m = 0; m < M; ++m) CF[m] = std::max(K - S[N][m], 0.0);

    // Backward induction using polynomial basis in S
    for (int n = N - 1; n >= 1; --n) {
        std::vector<long> itm;
        itm.reserve(M);
        for (long m = 0; m < M; ++m) if (K - S[n][m] > 0.0) itm.push_back(m);
        if (itm.size() < 5) continue;

        int d = std::max(1, p.poly_degree);
        int cols = d + 1;
        // Build X^T X and X^T y for regression: basis {1, S, S^2, ...}
        std::vector<double> XtX(cols * cols, 0.0), Xty(cols, 0.0);
        double disc = std::exp(-r * dt);
        for (long idx : itm) {
            double s = S[n][idx];
            double y = CF[idx] * disc; // discounted from n+1 to n
            std::vector<double> phi(cols, 1.0);
            for (int k = 1; k < cols; ++k) phi[k] = phi[k - 1] * s;
            for (int i = 0; i < cols; ++i) {
                Xty[i] += phi[i] * y;
                for (int j = 0; j < cols; ++j) XtX[i * cols + j] += phi[i] * phi[j];
            }
        }
        // Solve XtX * beta = Xty (Gaussian elimination)
        for (int i = 0; i < cols; ++i) {
            // Pivot
            int piv = i;
            for (int rrow = i + 1; rrow < cols; ++rrow) if (std::abs(XtX[rrow * cols + i]) > std::abs(XtX[piv * cols + i])) piv = rrow;
            if (piv != i) {
                for (int c = 0; c < cols; ++c) std::swap(XtX[i * cols + c], XtX[piv * cols + c]);
                std::swap(Xty[i], Xty[piv]);
            }
            double diag = XtX[i * cols + i];
            if (std::abs(diag) < 1e-14) continue;
            double invd = 1.0 / diag;
            for (int c = i; c < cols; ++c) XtX[i * cols + c] *= invd;
            Xty[i] *= invd;
            for (int rrow = 0; rrow < cols; ++rrow) if (rrow != i) {
                double f = XtX[rrow * cols + i];
                for (int c = i; c < cols; ++c) XtX[rrow * cols + c] -= f * XtX[i * cols + c];
                Xty[rrow] -= f * Xty[i];
            }
        }
        std::vector<double> beta(cols);
        for (int i = 0; i < cols; ++i) beta[i] = Xty[i];

        // Exercise decision
        for (long m = 0; m < M; ++m) {
            double payoff = std::max(K - S[n][m], 0.0);
            if (payoff <= 0) continue;
            double s = S[n][m];
            double cont = 0.0; double pow = 1.0;
            for (int k = 0; k < cols; ++k) { cont += beta[k] * pow; pow *= s; }
            if (payoff > cont) CF[m] = payoff; else CF[m] *= std::exp(-r * dt);
        }
    }

    // Discount remaining cashflows to t=0 (already discounted stepwise in loop)
    double price = 0.0;
    for (long m = 0; m < M; ++m) {
        double c = CF[m];
        price += c;
    }
    price /= static_cast<double>(M);
    return price;
}

}
