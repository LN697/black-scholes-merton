@echo off
REM Demo script for Option Pricing Analyzer
REM This script shows how to use the Python option pricing analyzer

echo ========================================
echo  BSM Option Pricing Analyzer Demo
echo ========================================
echo.

echo Prerequisites:
echo - Python 3.6+ must be installed
echo - BSM executable must be built (run 'make' first)
echo.

echo Available input files:
dir /b input\*.csv
dir /b examples\*.csv
echo.

echo Example Usage:
echo.
echo 1. Analyze option chain (NSE/BSE format):
echo    python scripts\option_pricing_analyzer.py input\option-chain-ED-NIFTY-14-Aug-2025.csv
echo.
echo 2. Analyze portfolio with JSON output:
echo    python scripts\option_pricing_analyzer.py examples\sample_portfolio.csv --output json --file results.json
echo.
echo 3. Custom spot price and risk-free rate:
echo    python scripts\option_pricing_analyzer.py input\option-chain-ED-NIFTY-14-Aug-2025.csv --spot 23500 --rate 0.06
echo.
echo 4. CSV output format:
echo    python scripts\option_pricing_analyzer.py examples\sample_portfolio.csv --output csv --file portfolio_analysis.csv
echo.

echo Output Formats:
echo - table: Formatted ASCII table (default)
echo - json:  JSON format for programmatic use
echo - csv:   CSV format for Excel/analysis
echo.

echo Script Features:
echo - Auto-detects CSV format (option chain vs portfolio)
echo - Estimates spot price from option chain data
echo - Calculates Black-Scholes prices and Greeks
echo - Provides portfolio-level risk metrics
echo - Identifies over/underpriced options
echo - Handles multiple option formats (NSE, BSE, custom)
echo.

pause
