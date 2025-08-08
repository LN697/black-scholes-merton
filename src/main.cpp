#include <iostream>
#include <vector>
#include <iomanip>
#include <random>

#include "option_types.hpp"
#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"
#include "pde_cn.hpp"
#include "slv.hpp"
#include "stats.hpp"
#include "iv_solve.hpp"

int main() {
    using namespace bsm;

    // Base inputs
    double S0 = 100.0;
    double K = 100.0;
    double r = 0.05;
    double T = 1.0;
    double sigma = 0.20;
    OptionType type = OptionType::Call;

    long mc_paths = 500000; // GBM Monte Carlo paths
    int pde_S = 300;        // PDE S-steps
    int pde_T = 150;        // PDE T-steps

    // SLV setup
    HestonParams heston; // defaults
    // CEV local vol alternative:
    // CEVLocalVol local_vol_cev{0.25, 0.9, S0};
    // Smile/skew local vol:
    SmileLocalVol local_vol_smile{0.22, 0.95, 0.25, 0.15, S0, 0.01};
    long slv_paths = 300000;
    long slv_steps = 252; // daily

    // Model config
    bool use_smile_lv = true;      // toggle CEV vs Smile
    bool use_gbm_qmc = true;       // use QMC Halton in GBM
    bool use_gbm_cv = true;        // use control variate in GBM
    bool use_andersen_qe = true;   // use Andersen QE in SLV

    // Analytical
    double analytic = black_scholes_price(S0, K, r, T, sigma, type);

    // GBM MC
    MCResult mc = mc_gbm_price(S0, K, r, T, sigma, mc_paths, type, 12345UL, true, use_gbm_cv, use_gbm_qmc);

    // PDE
    double pde = pde_crank_nicolson(S0, K, r, T, sigma, pde_S, pde_T, type);

    // SLV MC (single run)
    auto lv_fn = use_smile_lv ? local_vol_smile.to_fn() : CEVLocalVol{0.25, 0.9, S0}.to_fn();
    MCResult slv = mc_slv_price(S0, K, r, T, slv_paths, slv_steps, type, heston, lv_fn, 77777UL, true, use_andersen_qe);

    // SLV MC multiple random datasets (different seeds)
    std::vector<unsigned long> seeds;
    std::mt19937_64 seeder(424242ULL);
    for (int i = 0; i < 5; ++i) seeds.push_back(seeder());
    auto slv_runs = mc_slv_multi_seeds(S0, K, r, T, slv_paths / 2, slv_steps, type, heston, lv_fn, seeds, true, use_andersen_qe);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "=======================================================\n";
    std::cout << "   European Option Pricing: BS, PDE, MC, and SLV       \n";
    std::cout << "=======================================================\n";
    std::cout << "Input Parameters:\n";
    std::cout << "  S0=" << S0 << ", K=" << K << ", r=" << r << ", T=" << T << ", sigma=" << sigma
              << ", type=" << (type == OptionType::Call ? "Call" : "Put") << "\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << "Analytical BS:            " << analytic << "\n";
    std::cout << "MC (GBM) price:           " << mc.price << "  (SE: " << mc.std_error << ")\n";
    std::cout << "PDE (Crank-Nicolson):     " << pde << "\n";
    std::cout << "SLV MC price:             " << slv.price << "  (SE: " << slv.std_error << ")\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << "SLV verification across seeds (paths per run=" << slv_paths/2 << "):\n";
    for (size_t i = 0; i < slv_runs.size(); ++i) {
        std::cout << "  Seed[" << i << "] price=" << slv_runs[i].price << ", SE=" << slv_runs[i].std_error << "\n";
    }

    // Comparison errors vs analytical (only apples-to-apples for GBM/PDE)
    std::cout << "-------------------------------------------------------\n";
    std::cout << "Errors vs Analytical (GBM/PDE):\n";
    std::cout << "  MC error:                " << std::abs(mc.price - analytic) << "\n";
    std::cout << "  PDE error:               " << std::abs(pde - analytic) << "\n";

    // Implied vols for MC and PDE (under GBM assumptions)
    auto bs_price_fn = [&](double sig){ return black_scholes_price(S0, K, r, T, sig, type); };
    double iv_mc = implied_vol(mc.price, bs_price_fn);
    double iv_pde = implied_vol(pde, bs_price_fn);
    std::cout << "-------------------------------------------------------\n";
    std::cout << "Implied volatilities (from GBM price):\n";
    std::cout << "  IV from MC:              " << iv_mc << "\n";
    std::cout << "  IV from PDE:             " << iv_pde << "\n";
    std::cout << "=======================================================\n";

    return 0;
}
