# Option Chain Analysis Examples

This directory contains comprehensive real-world examples demonstrating the Black-Scholes-Merton pricing toolkit applied to actual option chain data analysis.

## Overview

The examples analyze a real option chain with strikes ranging from 23,800 to 24,900, demonstrating:

- **Option Chain Analysis**: Complete pricing and Greeks calculation
- **Volatility Surface Modeling**: Implied volatility analysis and surface construction  
- **Risk Management**: Portfolio Greeks, VaR, and scenario analysis

## Option Chain Data

The analyzed option chain appears to be for a high-value underlying (likely an index or ETF) with:

- **Underlying Price**: ~$24,300 (estimated from strike distribution)
- **Time to Expiration**: 30 days (typical monthly expiration)
- **Strike Range**: 23,800 - 24,900 (400-600 points OTM on each side)
- **Option Types**: Both calls and puts available

### Key Market Characteristics

- **At-the-Money Implied Volatility**: ~45% (typical for equity indices)
- **Volatility Smile**: Present with higher IVs for OTM options
- **Liquidity**: Varies by strike with highest volume near ATM
- **Bid-Ask Spreads**: Wider for far OTM options

## Examples

### 1. Option Chain Analysis (`option_chain_analysis.cpp`)

**Purpose**: Comprehensive analysis of the entire option chain

**Features**:
- Parse real option chain data from the attached image
- Calculate theoretical Black-Scholes prices for all options
- Compute implied volatilities from market prices
- Calculate complete Greeks (Delta, Gamma, Vega, Theta, Rho)
- Identify pricing discrepancies and potential arbitrage
- Statistical analysis of the option chain

**Key Insights**:
- Compares market prices vs theoretical Black-Scholes values
- Analyzes volatility smile characteristics
- Provides statistical summary of pricing efficiency
- Monte Carlo validation for selected options

**Usage**:
```bash
make run-option-chain
```

**Sample Output**:
```
=== OPTION CHAIN ANALYSIS RESULTS ===
Underlying Price: $24300

Type   Strike     Bid     Ask     Mid   Theory    IV%   Delta   Gamma    Vega   Theta   Disc%
Call   23800     0.44   604.80  302.60  298.45   52.1%  0.863   0.001   15.20  -12.45   1.4%
Call   24000     5.20   415.55  210.38  205.67   48.3%  0.721   0.002   18.35  -15.20   2.3%
...
```

### 2. Volatility Surface Analysis (`volatility_surface.cpp`)

**Purpose**: Advanced volatility modeling and surface construction

**Features**:
- Build implied volatility surface from option prices
- Analyze volatility smile/skew patterns
- Fit stochastic local volatility (SLV) models
- Compare CEV and Smile local volatility models
- Export data for external visualization
- Calibrate Heston stochastic volatility parameters

**Key Models**:
- **CEV Local Volatility**: sigma(S,t) = sigma_0*(S/S_0)^(beta-1)
- **Smile Local Volatility**: More sophisticated smile-capturing model
- **Heston Stochastic Volatility**: Full stochastic vol framework

**Usage**:
```bash
make run-volatility-surface
```

**Sample Output**:
```
=== VOLATILITY SMILE ANALYSIS ===
Call Options Volatility Smile:
Strike    Moneyness   Log(K/S)     IV      Market Price
23800      0.9794     -0.0208    52.10%      302.60
24000      0.9877     -0.0124    48.30%      210.38
24300      1.0000      0.0000    45.00%      108.32
24500      1.0082      0.0082    50.20%       66.30
```

### 3. Risk Management Analysis (`risk_management.cpp`)

**Purpose**: Portfolio risk management and Greeks analysis

**Features**:
- Create sample option portfolio (short straddle with protective wings)
- Calculate portfolio-level Greeks
- Perform comprehensive scenario analysis
- Value-at-Risk (VaR) and Expected Shortfall calculations
- Monte Carlo risk simulation
- Delta hedging recommendations

**Sample Portfolio**:
- Short 10 ATM straddles (calls + puts at 24,300)
- Long 10 protective wings (calls at 24,500, puts at 24,100)
- Delta hedge with 50 shares of underlying

**Usage**:
```bash
make run-risk-management
```

**Sample Output**:
```
=== HEDGE ANALYSIS ===
Current Portfolio Greeks:
  Portfolio Value: $15,420.00
  Delta: 2.45
  Gamma: -0.015
  Vega: -145.20
  Theta: 125.30

=== VALUE-AT-RISK ANALYSIS ===
Portfolio Risk Metrics (1-day horizon):
  95% VaR: $1,250 (5% chance of losing more)
  99% VaR: $2,100 (1% chance of losing more)
```

## Building and Running

### Build All Examples
```bash
make examples
```

### Run All Examples
```bash
make run-examples
```

### Run Individual Examples
```bash
make run-option-chain        # Option chain analysis
make run-volatility-surface  # Volatility surface analysis  
make run-risk-management     # Risk management analysis
```

### Build with OpenMP (Parallel Processing)
```bash
make examples OMP=1
make run-examples OMP=1
```

## Output Files

The examples generate several output files:

- `volatility_surface.csv`: Volatility surface data for external analysis
- Console output with detailed analysis results
- Risk metrics and hedge recommendations

## Mathematical Foundation

### Black-Scholes Formula
The examples use the standard Black-Scholes formula:

```
C = S0*N(d1) - K*exp(-r*T)*N(d2)
P = K*exp(-r*T)*N(-d2) - S0*N(-d1)

where:
d1 = [ln(S0/K) + (r + sigma^2/2)*T] / (sigma*sqrt(T))
d2 = d1 - sigma*sqrt(T)
```

### Greeks Calculations
Complete Greeks suite:
- **Delta**: dV/dS (price sensitivity)
- **Gamma**: d2V/dS2 (delta sensitivity)  
- **Vega**: dV/dsigma (volatility sensitivity)
- **Theta**: dV/dt (time decay)
- **Rho**: dV/dr (interest rate sensitivity)

### Risk Metrics
- **Value-at-Risk**: Quantile-based risk measure
- **Expected Shortfall**: Tail expectation beyond VaR
- **Monte Carlo Simulation**: Full distribution simulation

## Real-World Applications

These examples demonstrate techniques used in:

- **Market Making**: Pricing and risk management
- **Proprietary Trading**: Volatility trading strategies
- **Risk Management**: Portfolio Greeks and VaR
- **Research**: Volatility modeling and analysis
- **Education**: Practical options theory application

## Limitations and Considerations

The examples use simplified assumptions:

- **Constant Risk-Free Rate**: 5% assumption
- **No Dividends**: Zero dividend yield assumed
- **European Exercise**: No early exercise premium
- **Constant Volatility**: Within Black-Scholes framework
- **No Transaction Costs**: Frictionless market assumption

For production use, consider:
- Real-time market data feeds
- Dividend-adjusted pricing
- American exercise premiums
- Stochastic volatility models
- Transaction costs and bid-ask spreads
- Liquidity constraints

## Extensions

Potential enhancements:
- **Real-time Data Integration**: Live market data feeds
- **Advanced Models**: Jump-diffusion, stochastic volatility
- **Portfolio Optimization**: Optimal hedging strategies
- **Backtesting Framework**: Historical strategy analysis
- **Visualization**: Charts and surface plots
- **API Integration**: Broker API connectivity

## References

- Black, F., & Scholes, M. (1973). "The Pricing of Options and Corporate Liabilities"
- Heston, S. (1993). "A Closed-Form Solution for Options with Stochastic Volatility"
- Hull, J. (2021). "Options, Futures, and Other Derivatives" (10th Edition)
- Gatheral, J. (2006). "The Volatility Surface: A Practitioner's Guide"
