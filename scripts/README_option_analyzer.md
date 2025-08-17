# Option Pricing Analyzer

A comprehensive Python script for analyzing option pricing data using the Black-Scholes-Merton pricing toolkit.

## Features

- **Auto-format Detection**: Automatically detects CSV format (option chain vs portfolio)
- **Black-Scholes Pricing**: Calculates theoretical prices using the BSM executable
- **Greeks Calculation**: Computes Delta, Gamma, Vega, Theta for each option
- **Portfolio Analysis**: Provides portfolio-level risk metrics and Greeks
- **Market Analysis**: Identifies over/underpriced options vs theoretical values
- **Multiple Output Formats**: Table, JSON, and CSV output formats
- **Flexible Input**: Supports NSE/BSE option chains and custom portfolio formats

## Requirements

- Python 3.6 or higher
- BSM executable (built from this project)
- CSV files with option data

## Installation

1. Build the BSM executable:
   ```bash
   make
   ```

2. Ensure Python is installed and available in PATH

## Usage

### Basic Usage

```bash
# Analyze option chain (auto-detects format)
python scripts/option_pricing_analyzer.py input/option-chain-ED-NIFTY-14-Aug-2025.csv

# Analyze portfolio
python scripts/option_pricing_analyzer.py examples/sample_portfolio.csv
```

### Advanced Usage

```bash
# Custom spot price and risk-free rate
python scripts/option_pricing_analyzer.py input/options.csv --spot 23500 --rate 0.06

# JSON output to file
python scripts/option_pricing_analyzer.py examples/sample_portfolio.csv --output json --file results.json

# CSV output for Excel analysis
python scripts/option_pricing_analyzer.py input/options.csv --output csv --file analysis.csv
```

### Command Line Options

- `csv_file`: Path to CSV file containing option data
- `--output, -o`: Output format (table, json, csv) [default: table]
- `--file, -f`: Output file path (default: stdout)
- `--spot, -s`: Spot price (auto-detected if not provided)
- `--rate, -r`: Risk-free rate [default: 0.05]
- `--bsm-executable`: Path to BSM executable (auto-detected)

## Input Formats

### Option Chain Format (NSE/BSE)
The script automatically parses NSE/BSE option chain CSV files with the following structure:
- CALLS and PUTS sections
- Strike prices in column 11
- LTP (Last Traded Price) for calls and puts
- Implied volatility percentages

### Portfolio Format
CSV files with the following columns:
```csv
symbol,position,spot,strike,expiry,volatility,option_type
AAPL,100,150.0,150.0,0.25,0.25,call
MSFT,-50,300.0,310.0,0.5,0.22,put
```

## Output Formats

### Table Format (Default)
ASCII table with option details, Greeks, and portfolio summary:
```
BLACK-SCHOLES-MERTON OPTION PRICING ANALYSIS
File: sample_portfolio.csv
Options Analyzed: 5
Spot Price: 150.00

OPTION DETAILS:
Symbol   Type Strike Days   IV     Market   Theor.   Diff%  Delta   Gamma   Vega   Theta
AAPL     call 150    91     25.0%  39.41    39.41    0.0%   0.503   0.041   156.6  -289.6
```

### JSON Format
Structured data suitable for programmatic processing:
```json
{
  "metadata": {
    "filename": "sample_portfolio.csv",
    "total_options": 5,
    "spot_price": 150.0
  },
  "options": [...],
  "summary": {
    "portfolio_metrics": {...},
    "risk_metrics": {...}
  }
}
```

### CSV Format
Comma-separated values for Excel/spreadsheet analysis:
```csv
symbol,option_type,strike,expiry_days,implied_volatility,market_price,theoretical_price,delta,gamma,vega,theta
AAPL,call,150.0,91,0.25,39.41,39.41,0.503,0.041,156.6,-289.6
```

## Analysis Features

### Portfolio Metrics
- Total portfolio value
- Net Delta, Gamma, Vega, Theta
- Delta-adjusted value (1% move impact)

### Risk Metrics
- Portfolio delta risk (1% price move)
- Portfolio gamma risk (convexity risk)
- Portfolio vega risk (1% volatility move)
- Portfolio theta decay (daily time decay)

### Market Analysis
- Options with market prices vs theoretical
- Average price difference percentage
- Count of overpriced/underpriced options

## Examples

### Example 1: NSE Option Chain Analysis
```bash
python scripts/option_pricing_analyzer.py input/option-chain-ED-NIFTY-14-Aug-2025.csv --spot 23400
```

This will:
1. Parse the NSE option chain CSV
2. Extract call and put options with their market prices
3. Calculate theoretical Black-Scholes prices
4. Show differences between market and theoretical prices
5. Provide portfolio-level Greek exposures

### Example 2: Portfolio Risk Analysis
```bash
python scripts/option_pricing_analyzer.py examples/sample_portfolio.csv --output json --file portfolio_risk.json
```

This will:
1. Calculate theoretical prices for each position
2. Compute portfolio-level Greeks
3. Assess risk metrics for the entire portfolio
4. Save detailed analysis to JSON file

### Example 3: Custom Analysis Parameters
```bash
python scripts/option_pricing_analyzer.py input/options.csv --spot 25000 --rate 0.065 --output csv --file detailed_analysis.csv
```

This will:
1. Use custom spot price of 25,000
2. Use custom risk-free rate of 6.5%
3. Output results in CSV format for further analysis

## Error Handling

The script handles various error conditions:
- Missing or malformed CSV files
- BSM executable not found
- Invalid option parameters
- Calculation timeouts

## Performance

- Processes large option chains efficiently
- Parallel calculation support (when available)
- Optimized CSV parsing
- Memory-efficient for large datasets

## Integration

The script integrates seamlessly with:
- Excel/Google Sheets (CSV output)
- Data analysis tools (JSON output)
- Trading systems (programmatic API)
- Risk management systems (portfolio metrics)

## Troubleshooting

### BSM Executable Not Found
Ensure the project is built:
```bash
make clean && make
```

### Python Import Errors
Ensure you're using Python 3.6+ and standard library modules are available.

### CSV Parsing Errors
Check that your CSV file format matches one of the supported formats:
- Option chain: NSE/BSE format with CALLS/PUTS sections
- Portfolio: symbol,position,spot,strike,expiry,volatility,option_type

## Support

For issues or feature requests, please refer to the main project documentation or create an issue in the project repository.
