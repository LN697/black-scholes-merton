#!/usr/bin/env python3
"""
Option Pricing Analyzer for Black-Scholes-Merton Toolkit

This script processes CSV files containing option data and calculates
Black-Scholes prices, Greeks, and risk metrics using the BSM toolkit.

Author: Generated for BSM Toolkit
Version: 1.0
Date: August 2025
"""

import argparse
import csv
import json
import os
import subprocess
import sys
import tempfile
from datetime import datetime, date
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Union
import re


class OptionPricingAnalyzer:
    """Main class for analyzing option pricing data from CSV files."""
    
    def __init__(self, bsm_executable: str = None):
        """Initialize the analyzer with BSM executable path."""
        self.bsm_executable = bsm_executable or self._find_bsm_executable()
        self.results = []
        
    def _find_bsm_executable(self) -> str:
        """Find the BSM executable in the project directory."""
        project_root = Path(__file__).parent.parent
        possible_paths = [
            project_root / "build" / "release" / "bin" / "bsm_enhanced.exe",
            project_root / "build" / "release" / "bin" / "bsm.exe",
            project_root / "build" / "bin" / "bsm_enhanced.exe", 
            project_root / "build" / "bin" / "bsm.exe",
        ]
        
        for path in possible_paths:
            if path.exists():
                return str(path)
        
        raise FileNotFoundError("BSM executable not found. Please build the project first.")
    
    def parse_option_chain_csv(self, filepath: str) -> List[Dict]:
        """Parse option chain CSV (NSE/BSE format) to extract option data."""
        options = []
        
        with open(filepath, 'r', encoding='utf-8') as file:
            reader = csv.reader(file)
            headers = next(reader)  # Skip header row
            
            for row in reader:
                if len(row) < 23:  # Skip incomplete rows
                    continue
                    
                try:
                    # Extract strike price (column 11, 0-indexed)
                    strike_str = row[11].replace(',', '').replace('"', '')
                    if not strike_str or strike_str == '-':
                        continue
                    strike = float(strike_str)
                    
                    # Extract call data (left side)
                    call_ltp = self._parse_price(row[5])  # Call LTP
                    call_iv = self._parse_percentage(row[4])  # Call IV
                    
                    # Extract put data (right side)  
                    put_ltp = self._parse_price(row[17])  # Put LTP
                    put_iv = self._parse_percentage(row[18])  # Put IV
                    
                    # Add call option if data exists
                    if call_ltp is not None and call_ltp > 0:
                        options.append({
                            'symbol': 'NIFTY',
                            'strike': strike,
                            'option_type': 'call',
                            'market_price': call_ltp,
                            'implied_volatility': call_iv or 0.20,  # Default 20%
                            'expiry_days': self._estimate_expiry_days(filepath),
                        })
                    
                    # Add put option if data exists
                    if put_ltp is not None and put_ltp > 0:
                        options.append({
                            'symbol': 'NIFTY', 
                            'strike': strike,
                            'option_type': 'put',
                            'market_price': put_ltp,
                            'implied_volatility': put_iv or 0.20,  # Default 20%
                            'expiry_days': self._estimate_expiry_days(filepath),
                        })
                        
                except (ValueError, IndexError) as e:
                    continue  # Skip malformed rows
                    
        return options
    
    def parse_portfolio_csv(self, filepath: str) -> List[Dict]:
        """Parse portfolio CSV format (symbol,position,spot,strike,expiry,volatility,option_type)."""
        options = []
        
        with open(filepath, 'r', encoding='utf-8') as file:
            reader = csv.DictReader(file)
            
            for row in reader:
                try:
                    option = {
                        'symbol': row['symbol'],
                        'position': float(row['position']),
                        'spot_price': float(row['spot']),
                        'strike': float(row['strike']),
                        'expiry_days': float(row['expiry']) * 365,  # Convert years to days
                        'implied_volatility': float(row['volatility']),
                        'option_type': row['option_type'].lower(),
                        'market_price': None  # Will be calculated
                    }
                    options.append(option)
                except (ValueError, KeyError) as e:
                    print(f"Warning: Skipping malformed row: {e}")
                    continue
                    
        return options
    
    def _parse_price(self, price_str: str) -> Optional[float]:
        """Parse price string, handling commas and quotes."""
        if not price_str or price_str == '-':
            return None
        try:
            clean_str = price_str.replace(',', '').replace('"', '').strip()
            return float(clean_str) if clean_str else None
        except ValueError:
            return None
    
    def _parse_percentage(self, pct_str: str) -> Optional[float]:
        """Parse percentage string to decimal."""
        if not pct_str or pct_str == '-':
            return None
        try:
            clean_str = pct_str.replace('%', '').replace(',', '').replace('"', '').strip()
            return float(clean_str) / 100.0 if clean_str else None
        except ValueError:
            return None
    
    def _estimate_expiry_days(self, filepath: str) -> int:
        """Estimate expiry days from filename or default to 30."""
        # Try to extract date from filename like "option-chain-ED-NIFTY-14-Aug-2025.csv"
        filename = Path(filepath).name
        date_pattern = r'(\d{1,2})-([A-Za-z]{3})-(\d{4})'
        match = re.search(date_pattern, filename)
        
        if match:
            day, month_str, year = match.groups()
            month_map = {
                'Jan': 1, 'Feb': 2, 'Mar': 3, 'Apr': 4, 'May': 5, 'Jun': 6,
                'Jul': 7, 'Aug': 8, 'Sep': 9, 'Oct': 10, 'Nov': 11, 'Dec': 12
            }
            try:
                month = month_map.get(month_str, 8)  # Default to August
                expiry_date = date(int(year), month, int(day))
                today = date.today()
                return max(1, (expiry_date - today).days)
            except ValueError:
                pass
                
        return 30  # Default to 30 days
    
    def estimate_spot_price(self, options: List[Dict]) -> float:
        """Estimate spot price from option chain using put-call parity."""
        if not options:
            return 23000.0  # Default NIFTY level
            
        # Find ATM options (closest to spot)
        call_strikes = [opt for opt in options if opt['option_type'] == 'call']
        put_strikes = [opt for opt in options if opt['option_type'] == 'put']
        
        if not call_strikes or not put_strikes:
            return 23000.0
            
        # Use mid-range strike as estimate
        all_strikes = [opt['strike'] for opt in options]
        return sum(all_strikes) / len(all_strikes) if all_strikes else 23000.0
    
    def calculate_option_price(self, option: Dict, spot_price: float, risk_free_rate: float = 0.05) -> Dict:
        """Calculate option price using BSM executable."""
        try:
            # Prepare parameters
            time_to_expiry = option['expiry_days'] / 365.0
            
            # Call BSM executable
            cmd = [
                self.bsm_executable,
                'price',
                '--spot', str(spot_price),
                '--strike', str(option['strike']),
                '--rate', str(risk_free_rate),
                '--time', str(time_to_expiry),
                '--vol', str(option['implied_volatility']),
                '--type', option['option_type']
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            
            if result.returncode == 0:
                # Parse output to extract price and Greeks
                output = result.stdout
                price_match = re.search(r'Price:\s*([\d.]+)', output)
                delta_match = re.search(r'Delta:\s*([-\d.]+)', output)
                gamma_match = re.search(r'Gamma:\s*([\d.]+)', output)
                vega_match = re.search(r'Vega:\s*([\d.]+)', output)
                theta_match = re.search(r'Theta:\s*([-\d.]+)', output)
                
                return {
                    'theoretical_price': float(price_match.group(1)) if price_match else 0.0,
                    'delta': float(delta_match.group(1)) if delta_match else 0.0,
                    'gamma': float(gamma_match.group(1)) if gamma_match else 0.0,
                    'vega': float(vega_match.group(1)) if vega_match else 0.0,
                    'theta': float(theta_match.group(1)) if theta_match else 0.0,
                    'calculation_success': True
                }
            else:
                print(f"BSM calculation failed: {result.stderr}")
                return self._default_option_result()
                
        except (subprocess.TimeoutExpired, subprocess.SubprocessError, ValueError) as e:
            print(f"Error calculating option price: {e}")
            return self._default_option_result()
    
    def _default_option_result(self) -> Dict:
        """Return default option calculation result."""
        return {
            'theoretical_price': 0.0,
            'delta': 0.0,
            'gamma': 0.0,
            'vega': 0.0,
            'theta': 0.0,
            'calculation_success': False
        }
    
    def analyze_csv(self, filepath: str, output_format: str = 'table', 
                   spot_price: Optional[float] = None, risk_free_rate: float = 0.05) -> Dict:
        """Analyze CSV file and return results."""
        print(f"Analyzing: {filepath}")
        
        # Determine CSV format and parse
        with open(filepath, 'r') as f:
            first_line = f.readline().lower()
            
        if 'symbol,position,spot,strike' in first_line:
            options = self.parse_portfolio_csv(filepath)
            format_type = 'portfolio'
        else:
            options = self.parse_option_chain_csv(filepath)
            format_type = 'option_chain'
            
        if not options:
            return {'error': 'No valid options found in CSV file'}
        
        # Estimate spot price if not provided
        if spot_price is None:
            if format_type == 'portfolio':
                spot_price = options[0].get('spot_price', 23000.0)
            else:
                spot_price = self.estimate_spot_price(options)
        
        print(f"Using spot price: {spot_price}")
        print(f"Found {len(options)} options to analyze")
        
        # Calculate prices and Greeks for each option
        results = []
        for i, option in enumerate(options):
            print(f"Processing option {i+1}/{len(options)}: {option['option_type']} strike {option['strike']}")
            
            calculation = self.calculate_option_price(option, spot_price, risk_free_rate)
            
            result = {
                'symbol': option.get('symbol', 'UNKNOWN'),
                'option_type': option['option_type'],
                'strike': option['strike'],
                'spot_price': spot_price,
                'expiry_days': option['expiry_days'],
                'implied_volatility': option['implied_volatility'],
                'market_price': option.get('market_price'),
                'position': option.get('position', 1),
                **calculation
            }
            
            # Calculate market vs theoretical difference
            if result['market_price'] is not None:
                result['price_difference'] = result['market_price'] - result['theoretical_price']
                result['price_difference_pct'] = (result['price_difference'] / result['theoretical_price'] * 100) if result['theoretical_price'] > 0 else 0
            
            results.append(result)
        
        # Generate summary statistics
        summary = self._generate_summary(results, spot_price)
        
        return {
            'metadata': {
                'filename': Path(filepath).name,
                'format_type': format_type,
                'total_options': len(results),
                'spot_price': spot_price,
                'risk_free_rate': risk_free_rate,
                'analysis_time': datetime.now().isoformat()
            },
            'options': results,
            'summary': summary
        }
    
    def _generate_summary(self, results: List[Dict], spot_price: float) -> Dict:
        """Generate summary statistics from results."""
        successful_calcs = [r for r in results if r['calculation_success']]
        
        if not successful_calcs:
            return {'error': 'No successful calculations'}
        
        # Portfolio level metrics
        total_delta = sum(r['delta'] * r['position'] for r in successful_calcs)
        total_gamma = sum(r['gamma'] * r['position'] for r in successful_calcs)
        total_vega = sum(r['vega'] * r['position'] for r in successful_calcs)
        total_theta = sum(r['theta'] * r['position'] for r in successful_calcs)
        
        portfolio_value = sum(r['theoretical_price'] * r['position'] for r in successful_calcs)
        
        # Market vs theoretical analysis
        mispriced_options = [r for r in successful_calcs if r.get('market_price') is not None]
        avg_price_diff = sum(abs(r['price_difference_pct']) for r in mispriced_options) / len(mispriced_options) if mispriced_options else 0
        
        return {
            'portfolio_metrics': {
                'total_value': portfolio_value,
                'net_delta': total_delta,
                'net_gamma': total_gamma,
                'net_vega': total_vega,
                'net_theta': total_theta,
                'delta_adjusted_value': portfolio_value + total_delta * spot_price * 0.01  # 1% move
            },
            'market_analysis': {
                'options_with_market_prices': len(mispriced_options),
                'avg_price_difference_pct': avg_price_diff,
                'overpriced_count': len([r for r in mispriced_options if r['price_difference'] > 0]),
                'underpriced_count': len([r for r in mispriced_options if r['price_difference'] < 0])
            },
            'risk_metrics': {
                'portfolio_delta_risk': abs(total_delta) * spot_price * 0.01,  # 1% move impact
                'portfolio_gamma_risk': abs(total_gamma) * spot_price * spot_price * 0.0001,  # Gamma risk
                'portfolio_vega_risk': abs(total_vega) * 0.01,  # 1% vol move
                'portfolio_theta_decay': total_theta  # Daily decay
            }
        }
    
    def output_results(self, results: Dict, format_type: str = 'table', output_file: Optional[str] = None):
        """Output results in specified format."""
        if format_type == 'json':
            output = json.dumps(results, indent=2)
        elif format_type == 'csv':
            output = self._format_as_csv(results)
        else:  # table
            output = self._format_as_table(results)
        
        if output_file:
            with open(output_file, 'w') as f:
                f.write(output)
            print(f"Results saved to: {output_file}")
        else:
            print(output)
    
    def _format_as_table(self, results: Dict) -> str:
        """Format results as ASCII table."""
        if 'error' in results:
            return f"Error: {results['error']}"
        
        lines = []
        lines.append("=" * 80)
        lines.append("BLACK-SCHOLES-MERTON OPTION PRICING ANALYSIS")
        lines.append("=" * 80)
        
        # Metadata
        meta = results['metadata']
        lines.append(f"File: {meta['filename']}")
        lines.append(f"Format: {meta['format_type']}")
        lines.append(f"Options Analyzed: {meta['total_options']}")
        lines.append(f"Spot Price: {meta['spot_price']:.2f}")
        lines.append(f"Risk-free Rate: {meta['risk_free_rate']:.2%}")
        lines.append("")
        
        # Options table
        lines.append("OPTION DETAILS:")
        lines.append("-" * 120)
        header = f"{'Symbol':<8} {'Type':<4} {'Strike':<8} {'Days':<4} {'IV':<6} {'Market':<8} {'Theor.':<8} {'Diff%':<6} {'Delta':<7} {'Gamma':<7} {'Vega':<6} {'Theta':<7}"
        lines.append(header)
        lines.append("-" * 120)
        
        for opt in results['options'][:20]:  # Limit to first 20
            market_price = f"{opt['market_price']:.2f}" if opt['market_price'] is not None else "N/A"
            diff_pct = f"{opt.get('price_difference_pct', 0):.1f}%" if opt.get('price_difference_pct') is not None else "N/A"
            
            line = f"{opt['symbol']:<8} {opt['option_type']:<4} {opt['strike']:<8.0f} {opt['expiry_days']:<4.0f} " + \
                   f"{opt['implied_volatility']:<6.1%} {market_price:<8} {opt['theoretical_price']:<8.2f} " + \
                   f"{diff_pct:<6} {opt['delta']:<7.3f} {opt['gamma']:<7.4f} {opt['vega']:<6.2f} {opt['theta']:<7.2f}"
            lines.append(line)
        
        if len(results['options']) > 20:
            lines.append(f"... and {len(results['options']) - 20} more options")
        
        # Summary
        if 'summary' in results and 'error' not in results['summary']:
            summary = results['summary']
            lines.append("")
            lines.append("PORTFOLIO SUMMARY:")
            lines.append("-" * 40)
            
            portfolio = summary['portfolio_metrics']
            lines.append(f"Total Portfolio Value: {portfolio['total_value']:,.2f}")
            lines.append(f"Net Delta: {portfolio['net_delta']:.2f}")
            lines.append(f"Net Gamma: {portfolio['net_gamma']:.4f}")
            lines.append(f"Net Vega: {portfolio['net_vega']:.2f}")
            lines.append(f"Net Theta: {portfolio['net_theta']:.2f}")
            
            if 'market_analysis' in summary:
                market = summary['market_analysis']
                lines.append("")
                lines.append("MARKET ANALYSIS:")
                lines.append("-" * 20)
                lines.append(f"Options with Market Prices: {market['options_with_market_prices']}")
                lines.append(f"Avg Price Difference: {market['avg_price_difference_pct']:.1f}%")
                lines.append(f"Overpriced: {market['overpriced_count']}, Underpriced: {market['underpriced_count']}")
        
        lines.append("=" * 80)
        return "\n".join(lines)
    
    def _format_as_csv(self, results: Dict) -> str:
        """Format results as CSV."""
        if 'error' in results:
            return f"Error,{results['error']}"
        
        lines = []
        # CSV header
        header = "symbol,option_type,strike,expiry_days,implied_volatility,market_price,theoretical_price,price_difference,price_difference_pct,delta,gamma,vega,theta,position"
        lines.append(header)
        
        # Data rows
        for opt in results['options']:
            row = f"{opt['symbol']},{opt['option_type']},{opt['strike']},{opt['expiry_days']},{opt['implied_volatility']}," + \
                  f"{opt.get('market_price', '')}," + \
                  f"{opt['theoretical_price']},{opt.get('price_difference', '')}," + \
                  f"{opt.get('price_difference_pct', '')},{opt['delta']},{opt['gamma']},{opt['vega']},{opt['theta']},{opt['position']}"
            lines.append(row)
        
        return "\n".join(lines)


def main():
    """Main function to run the option pricing analyzer."""
    parser = argparse.ArgumentParser(
        description="Analyze option pricing from CSV files using Black-Scholes-Merton model",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python option_pricing_analyzer.py input/option-chain-ED-NIFTY-14-Aug-2025.csv
  python option_pricing_analyzer.py examples/sample_portfolio.csv --output json --file results.json
  python option_pricing_analyzer.py input/options.csv --spot 23000 --rate 0.06 --output table
        """
    )
    
    parser.add_argument('csv_file', help='CSV file containing option data')
    parser.add_argument('--output', '-o', choices=['table', 'json', 'csv'], default='table',
                       help='Output format (default: table)')
    parser.add_argument('--file', '-f', help='Output file (default: stdout)')
    parser.add_argument('--spot', '-s', type=float, help='Spot price (auto-detected if not provided)')
    parser.add_argument('--rate', '-r', type=float, default=0.05, help='Risk-free rate (default: 0.05)')
    parser.add_argument('--bsm-executable', help='Path to BSM executable (auto-detected if not provided)')
    
    args = parser.parse_args()
    
    # Validate input file
    if not os.path.exists(args.csv_file):
        print(f"Error: File '{args.csv_file}' not found")
        sys.exit(1)
    
    try:
        # Initialize analyzer
        analyzer = OptionPricingAnalyzer(args.bsm_executable)
        
        # Analyze CSV
        results = analyzer.analyze_csv(
            args.csv_file,
            output_format=args.output,
            spot_price=args.spot,
            risk_free_rate=args.rate
        )
        
        # Output results
        analyzer.output_results(results, args.output, args.file)
        
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
