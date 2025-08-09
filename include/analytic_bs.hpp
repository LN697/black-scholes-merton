#pragma once

/**
 * @file analytic_bs.hpp
 * @brief Analytical Black-Scholes option pricing formulas
 * 
 * This module provides closed-form solutions for European option pricing
 * under the Black-Scholes-Merton model with constant volatility and risk-free rate.
 * 
 * Mathematical Model:
 *   dS_t = rS_t dt + σS_t dW_t
 * 
 * Pricing Formulas:
 *   Call: C = S₀N(d₁) - Ke^(-rT)N(d₂)
 *   Put:  P = Ke^(-rT)N(-d₂) - S₀N(-d₁)
 * 
 * Where:
 *   d₁ = [ln(S₀/K) + (r + σ²/2)T] / (σ√T)
 *   d₂ = d₁ - σ√T
 *   N(x) = Standard normal CDF
 * 
 * @author LN697
 * @version 1.0
 */

#include "option_types.hpp"

namespace bsm {

/**
 * @brief Compute European option price using Black-Scholes formula
 * 
 * Provides exact analytical pricing for European call and put options
 * under constant volatility geometric Brownian motion.
 * 
 * @param S0 Current underlying asset price (> 0)
 * @param K Strike price (> 0)  
 * @param r Risk-free interest rate (continuously compounded)
 * @param T Time to expiration in years (≥ 0)
 * @param sigma Volatility (annualized, ≥ 0)
 * @param type Option type (Call or Put)
 * 
 * @return Option price
 * 
 * @note For edge cases:
 *       - If T ≤ 0 or σ ≤ 0: returns discounted intrinsic value
 *       - Handles extreme parameter values gracefully
 * 
 * @par Example:
 * @code
 * double call_price = black_scholes_price(100.0, 105.0, 0.05, 0.25, 0.2, OptionType::Call);
 * // Result: ~2.32 for 3-month 5% OTM call with 20% volatility
 * @endcode
 * 
 * @par Complexity: O(1)
 * @par Thread Safety: Yes (pure function)
 */
double black_scholes_price(double S0, double K, double r, double T, double sigma, OptionType type);

/**
 * @brief Compute Black-Scholes option delta (price sensitivity to underlying)
 * 
 * Delta measures the rate of change of option price with respect to
 * the underlying asset price.
 * 
 * @param S0 Current underlying asset price
 * @param K Strike price
 * @param r Risk-free interest rate  
 * @param T Time to expiration in years
 * @param sigma Volatility (annualized)
 * @param type Option type (Call or Put)
 * 
 * @return Delta (∂Price/∂S₀)
 *         - Call delta: [0, 1]
 *         - Put delta: [-1, 0]
 * 
 * @par Mathematical Formula:
 *      - Call: Δ = N(d₁)
 *      - Put:  Δ = N(d₁) - 1
 */
double black_scholes_delta(double S0, double K, double r, double T, double sigma, OptionType type);

/**
 * @brief Compute Black-Scholes option gamma (delta sensitivity to underlying)
 * 
 * Gamma measures the rate of change of delta with respect to the underlying
 * asset price. Same for both calls and puts.
 * 
 * @param S0 Current underlying asset price
 * @param K Strike price
 * @param r Risk-free interest rate
 * @param T Time to expiration in years
 * @param sigma Volatility (annualized)
 * 
 * @return Gamma (∂²Price/∂S₀²)
 * 
 * @par Mathematical Formula: Γ = φ(d₁)/(S₀σ√T)
 *      where φ(x) is the standard normal PDF
 */
double black_scholes_gamma(double S0, double K, double r, double T, double sigma);

/**
 * @brief Compute Black-Scholes option vega (price sensitivity to volatility)
 * 
 * Vega measures the rate of change of option price with respect to
 * the volatility. Same for both calls and puts.
 * 
 * @param S0 Current underlying asset price
 * @param K Strike price
 * @param r Risk-free interest rate
 * @param T Time to expiration in years
 * @param sigma Volatility (annualized)
 * 
 * @return Vega (∂Price/∂σ)
 * 
 * @par Mathematical Formula: ν = S₀φ(d₁)√T
 */
double black_scholes_vega(double S0, double K, double r, double T, double sigma);

/**
 * @brief Compute Black-Scholes option theta (time decay)
 * 
 * Theta measures the rate of change of option price with respect to time.
 * Usually negative for long positions (time decay).
 * 
 * @param S0 Current underlying asset price
 * @param K Strike price
 * @param r Risk-free interest rate
 * @param T Time to expiration in years
 * @param sigma Volatility (annualized)
 * @param type Option type (Call or Put)
 * 
 * @return Theta (∂Price/∂T)
 */
double black_scholes_theta(double S0, double K, double r, double T, double sigma, OptionType type);

/**
 * @brief Compute Black-Scholes option rho (interest rate sensitivity)
 * 
 * Rho measures the rate of change of option price with respect to
 * the risk-free interest rate.
 * 
 * @param S0 Current underlying asset price
 * @param K Strike price
 * @param r Risk-free interest rate
 * @param T Time to expiration in years
 * @param sigma Volatility (annualized)
 * @param type Option type (Call or Put)
 * 
 * @return Rho (∂Price/∂r)
 */
double black_scholes_rho(double S0, double K, double r, double T, double sigma, OptionType type);

} // namespace bsm
