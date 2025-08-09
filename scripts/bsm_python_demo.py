#!/usr/bin/env python3
"""
BSM Python Integration Example
Demonstrates how to integrate C++ BSM toolkit with Python data science workflows
"""

import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from typing import Dict, List, Tuple, Optional
import subprocess
import json
import os

class BSMPythonInterface:
    """
    Python wrapper for BSM C++ toolkit
    Provides seamless integration with pandas and scientific Python ecosystem
    """
    
    def __init__(self, bsm_executable_path: str = "./build/bin/bsm"):
        self.bsm_path = bsm_executable_path
        self._verify_bsm_available()
    
    def _verify_bsm_available(self):
        """Verify BSM executable is available"""
        try:
            result = subprocess.run([self.bsm_path, "--version"], 
                                  capture_output=True, text=True, check=True)
            print(f"✓ BSM Toolkit available: {result.stdout.strip()}")
        except (subprocess.CalledProcessError, FileNotFoundError):
            raise RuntimeError(f"BSM executable not found at {self.bsm_path}")
    
    def price_option(self, spot: float, strike: float, rate: float, 
                    time: float, volatility: float, option_type: str,
                    include_greeks: bool = True) -> Dict:
        """
        Price single option using BSM C++ engine
        
        Returns:
            Dictionary with price and optionally Greeks
        """
        cmd = [
            self.bsm_path, "price",
            "--spot", str(spot),
            "--strike", str(strike), 
            "--rate", str(rate),
            "--time", str(time),
            "--vol", str(volatility),
            "--type", option_type,
            "--format", "json"
        ]
        
        if include_greeks:
            cmd.append("--greeks")
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            return json.loads(result.stdout)
        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"BSM pricing failed: {e.stderr}")
    
    def price_option_chain(self, df: pd.DataFrame) -> pd.DataFrame:
        """
        Price multiple options from pandas DataFrame
        
        Expected columns: spot, strike, rate, time, volatility, type
        """
        results = []
        
        for _, row in df.iterrows():
            try:
                result = self.price_option(
                    spot=row['spot'],
                    strike=row['strike'],
                    rate=row['rate'],
                    time=row['time'],
                    volatility=row['volatility'],
                    option_type=row['type']
                )
                
                # Combine input parameters with results
                output_row = row.to_dict()
                output_row.update(result)
                results.append(output_row)
                
            except Exception as e:
                print(f"Warning: Failed to price option {row.to_dict()}: {e}")
                continue
        
        return pd.DataFrame(results)
    
    def monte_carlo_simulation(self, spot: float, strike: float, rate: float,
                             time: float, volatility: float, option_type: str,
                             num_paths: int = 100000) -> Dict:
        """Run Monte Carlo simulation"""
        cmd = [
            self.bsm_path, "montecarlo",
            "--spot", str(spot),
            "--strike", str(strike),
            "--rate", str(rate), 
            "--time", str(time),
            "--vol", str(volatility),
            "--type", option_type,
            "--paths", str(num_paths),
            "--format", "json"
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            return json.loads(result.stdout)
        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Monte Carlo simulation failed: {e.stderr}")

class VolatilitySurfaceAnalyzer:
    """
    Analyze and visualize volatility surfaces
    """
    
    def __init__(self, bsm_interface: BSMPythonInterface):
        self.bsm = bsm_interface
    
    def create_volatility_surface(self, spot: float, rate: float, time: float,
                                strikes: List[float], volatilities: List[float]) -> pd.DataFrame:
        """
        Create volatility surface data
        """
        surface_data = []
        
        for strike in strikes:
            for vol in volatilities:
                # Price call and put for each strike/vol combination
                call_result = self.bsm.price_option(spot, strike, rate, time, vol, "call")
                put_result = self.bsm.price_option(spot, strike, rate, time, vol, "put")
                
                surface_data.append({
                    'strike': strike,
                    'volatility': vol,
                    'moneyness': strike / spot,
                    'call_price': call_result['price'],
                    'put_price': put_result['price'],
                    'call_delta': call_result.get('delta', 0),
                    'put_delta': put_result.get('delta', 0)
                })
        
        return pd.DataFrame(surface_data)
    
    def plot_volatility_smile(self, surface_df: pd.DataFrame, vol_level: float = 0.2):
        """
        Plot volatility smile for specific volatility level
        """
        smile_data = surface_df[surface_df['volatility'] == vol_level]
        
        plt.figure(figsize=(12, 8))
        
        # Call prices
        plt.subplot(2, 2, 1)
        plt.plot(smile_data['moneyness'], smile_data['call_price'], 'b-o', label='Call Price')
        plt.xlabel('Moneyness (K/S)')
        plt.ylabel('Option Price')
        plt.title('Call Option Prices vs Moneyness')
        plt.grid(True, alpha=0.3)
        plt.legend()
        
        # Put prices
        plt.subplot(2, 2, 2)
        plt.plot(smile_data['moneyness'], smile_data['put_price'], 'r-o', label='Put Price')
        plt.xlabel('Moneyness (K/S)')
        plt.ylabel('Option Price')
        plt.title('Put Option Prices vs Moneyness')
        plt.grid(True, alpha=0.3)
        plt.legend()
        
        # Delta comparison
        plt.subplot(2, 2, 3)
        plt.plot(smile_data['moneyness'], smile_data['call_delta'], 'b-o', label='Call Delta')
        plt.plot(smile_data['moneyness'], smile_data['put_delta'], 'r-o', label='Put Delta')
        plt.xlabel('Moneyness (K/S)')
        plt.ylabel('Delta')
        plt.title('Delta vs Moneyness')
        plt.grid(True, alpha=0.3)
        plt.legend()
        
        # Price difference (Call-Put parity check)
        plt.subplot(2, 2, 4)
        price_diff = smile_data['call_price'] - smile_data['put_price']
        theoretical_diff = smile_data['strike'].values[0] * np.exp(-rate * time) - spot
        plt.plot(smile_data['moneyness'], price_diff, 'g-o', label='Call - Put')
        plt.axhline(y=theoretical_diff, color='orange', linestyle='--', 
                   label=f'Theoretical: {theoretical_diff:.4f}')
        plt.xlabel('Moneyness (K/S)')
        plt.ylabel('Price Difference')
        plt.title('Call-Put Parity Check')
        plt.grid(True, alpha=0.3)
        plt.legend()
        
        plt.tight_layout()
        plt.show()

def demo_basic_pricing():
    """Demonstrate basic option pricing"""
    print("=== BSM Python Integration Demo ===\n")
    
    # Initialize BSM interface
    bsm = BSMPythonInterface()
    
    # Single option pricing
    print("1. Single Option Pricing:")
    result = bsm.price_option(
        spot=100, strike=105, rate=0.05, time=0.25, 
        volatility=0.2, option_type="call"
    )
    
    print(f"Call Option Price: ${result['price']:.4f}")
    print(f"Delta: {result['delta']:.4f}")
    print(f"Gamma: {result['gamma']:.4f}")
    print(f"Vega: {result['vega']:.4f}")
    print(f"Theta: {result['theta']:.4f}")
    print()

def demo_option_chain_analysis():
    """Demonstrate option chain analysis"""
    print("2. Option Chain Analysis:")
    
    bsm = BSMPythonInterface()
    
    # Create sample option chain
    strikes = np.arange(90, 111, 5)
    option_chain = pd.DataFrame({
        'spot': 100,
        'strike': np.tile(strikes, 2),
        'rate': 0.05,
        'time': 0.25,
        'volatility': 0.2,
        'type': ['call'] * len(strikes) + ['put'] * len(strikes)
    })
    
    # Price entire chain
    results = bsm.price_option_chain(option_chain)
    
    # Display results
    print("Option Chain Results:")
    print(results[['type', 'strike', 'price', 'delta', 'gamma']].round(4))
    print()

def demo_volatility_surface():
    """Demonstrate volatility surface analysis"""
    print("3. Volatility Surface Analysis:")
    
    bsm = BSMPythonInterface()
    analyzer = VolatilitySurfaceAnalyzer(bsm)
    
    # Create volatility surface
    strikes = [90, 95, 100, 105, 110]
    volatilities = [0.15, 0.20, 0.25, 0.30]
    
    surface = analyzer.create_volatility_surface(
        spot=100, rate=0.05, time=0.25,
        strikes=strikes, volatilities=volatilities
    )
    
    print("Volatility Surface Sample:")
    print(surface.head(10).round(4))
    
    # Plot volatility smile
    if len(sys.argv) > 1 and sys.argv[1] == "--plot":
        analyzer.plot_volatility_smile(surface, vol_level=0.2)

def demo_performance_comparison():
    """Compare analytical vs Monte Carlo pricing"""
    print("4. Performance Comparison (Analytical vs Monte Carlo):")
    
    bsm = BSMPythonInterface()
    
    # Analytical pricing
    analytical_result = bsm.price_option(
        spot=100, strike=105, rate=0.05, time=0.25,
        volatility=0.2, option_type="call"
    )
    
    print(f"Analytical Price: ${analytical_result['price']:.4f}")
    
    # Monte Carlo simulation (if implemented)
    try:
        mc_result = bsm.monte_carlo_simulation(
            spot=100, strike=105, rate=0.05, time=0.25,
            volatility=0.2, option_type="call", num_paths=100000
        )
        print(f"Monte Carlo Price: ${mc_result['price']:.4f}")
        print(f"Standard Error: ${mc_result.get('std_error', 0):.4f}")
        print(f"Difference: ${abs(analytical_result['price'] - mc_result['price']):.4f}")
    except RuntimeError as e:
        print(f"Monte Carlo not yet implemented: {e}")

if __name__ == "__main__":
    try:
        demo_basic_pricing()
        demo_option_chain_analysis()
        demo_volatility_surface()
        demo_performance_comparison()
        
        print("✓ All demos completed successfully!")
        print("\nTo enable plotting, run: python bsm_python_demo.py --plot")
        
    except Exception as e:
        print(f"✗ Demo failed: {e}")
        sys.exit(1)
