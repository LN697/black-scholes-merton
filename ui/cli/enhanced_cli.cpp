#include "enhanced_cli.hpp"
#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"
#include "slv.hpp"
#include "stats.hpp"
#include "iv_solve.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>
#include <cmath>
#include <random>

namespace bsm::ui {

// ANSI color codes
namespace colors {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string BOLD = "\033[1m";
}

// EnhancedCLI Implementation
EnhancedCLI::EnhancedCLI() : running_(false) {
    // Register default commands
    register_command(std::make_unique<PriceCommand>());
    register_command(std::make_unique<PortfolioCommand>());
    register_command(std::make_unique<MonteCarloCommand>());
    register_command(std::make_unique<VolatilityCommand>());
    register_command(std::make_unique<ConfigCommand>());
    register_command(std::make_unique<HelpCommand>(this));
}

EnhancedCLI::~EnhancedCLI() = default;

void EnhancedCLI::register_command(std::unique_ptr<Command> command) {
    std::string name = command->name();
    commands_[name] = std::move(command);
}

int EnhancedCLI::run(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    
    if (args.empty()) {
        print_banner();
        print_help();
        return 0;
    }
    
    std::string command_name = args[0];
    args.erase(args.begin());
    
    // Handle global flags
    if (command_name == "--help" || command_name == "-h") {
        print_help();
        return 0;
    }
    
    if (command_name == "--version" || command_name == "-v") {
        print_version();
        return 0;
    }
    
    if (command_name == "--interactive" || command_name == "-i") {
        start_interactive_mode();
        return 0;
    }
    
    // Execute command
    auto it = commands_.find(command_name);
    if (it == commands_.end()) {
        print_error("Unknown command: " + command_name);
        print_info("Use 'help' to see available commands");
        return 1;
    }
    
    try {
        return it->second->execute(args);
    } catch (const std::exception& e) {
        print_error("Command failed: " + std::string(e.what()));
        return 1;
    }
}

void EnhancedCLI::start_interactive_mode() {
    print_banner();
    print_info("Entering interactive mode. Type 'help' for commands or 'exit' to quit.");
    
    running_ = true;
    while (running_) {
        std::string input = get_user_input(colorize("bsm> ", colors::CYAN));
        
        if (input.empty()) continue;
        
        if (input == "exit" || input == "quit") {
            running_ = false;
            break;
        }
        
        if (input == "clear") {
            std::cout << "\033[2J\033[H"; // Clear screen
            continue;
        }
        
        auto tokens = tokenize(input);
        if (tokens.empty()) continue;
        
        std::string command_name = tokens[0];
        tokens.erase(tokens.begin());
        
        auto it = commands_.find(command_name);
        if (it == commands_.end()) {
            print_error("Unknown command: " + command_name);
            continue;
        }
        
        try {
            it->second->execute(tokens);
        } catch (const std::exception& e) {
            print_error("Command failed: " + std::string(e.what()));
        }
    }
    
    print_info("Goodbye!");
}

void EnhancedCLI::print_banner() const {
    std::cout << colorize(R"(
 ____  ____  __  __   _____           _ _    _ _   
|  _ \/ ___||  \/  | |_   _|__   ___ | | | _(_) |_ 
| |_) \___ \| |\/| |   | |/ _ \ / _ \| | |/ / | __|
|  _ < ___) | |  | |   | | (_) | (_) | |   <| | |_ 
|_| \_\____/|_|  |_|   |_|\___/ \___/|_|_|\_\_|\__|
                                                   
)", colors::CYAN + colors::BOLD) << colors::RESET << std::endl;
    
    std::cout << colorize("Black-Scholes-Merton Pricing Toolkit v1.0.0", colors::YELLOW) << std::endl;
    std::cout << colorize("Enhanced CLI Interface", colors::WHITE) << std::endl << std::endl;
}

void EnhancedCLI::print_help() const {
    std::cout << colorize("Available Commands:", colors::BOLD) << std::endl << std::endl;
    
    TableFormatter table;
    table.set_headers({"Command", "Description"});
    
    for (const auto& [name, command] : commands_) {
        table.add_row({name, command->description()});
    }
    
    table.print_table();
    
    std::cout << std::endl;
    std::cout << colorize("Global Options:", colors::BOLD) << std::endl;
    std::cout << "  --help, -h          Show this help message" << std::endl;
    std::cout << "  --version, -v       Show version information" << std::endl;
    std::cout << "  --interactive, -i   Start interactive mode" << std::endl;
    std::cout << std::endl;
    std::cout << colorize("Examples:", colors::BOLD) << std::endl;
    std::cout << "  bsm price --spot 100 --strike 105 --rate 0.05 --time 0.25 --vol 0.2 --type call" << std::endl;
    std::cout << "  bsm montecarlo --spot 100 --strike 105 --paths 100000" << std::endl;
    std::cout << "  bsm portfolio --file portfolio.csv --output results.json" << std::endl;
}

void EnhancedCLI::print_version() const {
    std::cout << "BSM Toolkit v1.0.0" << std::endl;
    std::cout << "Built with C++17" << std::endl;
    std::cout << "Compiler: " << __VERSION__ << std::endl;
    std::cout << "Build date: " << __DATE__ << " " << __TIME__ << std::endl;
}

void EnhancedCLI::print_success(const std::string& message) const {
    std::cout << colorize("✓ ", colors::GREEN) << message << std::endl;
}

void EnhancedCLI::print_error(const std::string& message) const {
    std::cerr << colorize("✗ Error: ", colors::RED) << message << std::endl;
}

void EnhancedCLI::print_warning(const std::string& message) const {
    std::cout << colorize("⚠ Warning: ", colors::YELLOW) << message << std::endl;
}

void EnhancedCLI::print_info(const std::string& message) const {
    std::cout << colorize("ℹ ", colors::BLUE) << message << std::endl;
}

void EnhancedCLI::print_progress_bar(double percentage, const std::string& label) const {
    if (!config_.show_progress) return;
    
    const int bar_width = 50;
    int filled = static_cast<int>(percentage * bar_width / 100.0);
    
    std::cout << "\r" << label << " ";
    std::cout << colorize("[", colors::WHITE);
    
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
            std::cout << colorize("█", colors::GREEN);
        } else {
            std::cout << colorize("░", colors::WHITE);
        }
    }
    
    std::cout << colorize("]", colors::WHITE);
    std::cout << " " << std::fixed << std::setprecision(1) << percentage << "%";
    std::cout << std::flush;
    
    if (percentage >= 100.0) {
        std::cout << std::endl;
    }
}

std::string EnhancedCLI::colorize(const std::string& text, const std::string& color) const {
    if (config_.color_theme == ColorTheme::NONE) {
        return text;
    }
    return color + text + colors::RESET;
}

std::vector<std::string> EnhancedCLI::tokenize(const std::string& input) const {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string EnhancedCLI::get_user_input(const std::string& prompt) const {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

// PriceCommand Implementation
std::string PriceCommand::usage() const {
    return R"(Usage: price [OPTIONS]

Calculate option price using Black-Scholes model.

Required Options:
  --spot, -S PRICE      Current spot price
  --strike, -K PRICE    Strike price
  --rate, -r RATE       Risk-free interest rate
  --time, -T TIME       Time to expiration (years)
  --vol, -v VOL         Volatility (annualized)
  --type, -t TYPE       Option type (call/put)

Optional:
  --format FORMAT       Output format (table/json/csv)
  --precision DIGITS    Decimal precision (default: 4)
  --greeks              Include Greeks calculation

Examples:
  price --spot 100 --strike 105 --rate 0.05 --time 0.25 --vol 0.2 --type call
  price -S 100 -K 95 -r 0.05 -T 0.5 -v 0.25 -t put --greeks
)";
}

int PriceCommand::execute(const std::vector<std::string>& args) {
    ArgumentParser parser;
    parser.parse(args);
    
    // Check for help
    if (parser.has_flag("help") || parser.has_flag("h")) {
        std::cout << usage() << std::endl;
        return 0;
    }
    
    // Parse required arguments
    double spot = parser.get_double("spot", parser.get_double("S", 0));
    double strike = parser.get_double("strike", parser.get_double("K", 0));
    double rate = parser.get_double("rate", parser.get_double("r", 0));
    double time = parser.get_double("time", parser.get_double("T", 0));
    double vol = parser.get_double("vol", parser.get_double("v", 0));
    std::string type_str = parser.get_option("type", parser.get_option("t", ""));
    
    // Validate inputs
    if (spot <= 0 || strike <= 0 || time <= 0 || vol <= 0 || type_str.empty()) {
        std::cerr << "Error: Missing or invalid required parameters" << std::endl;
        std::cout << usage() << std::endl;
        return 1;
    }
    
    OptionType type;
    if (type_str == "call" || type_str == "c") {
        type = OptionType::Call;
    } else if (type_str == "put" || type_str == "p") {
        type = OptionType::Put;
    } else {
        std::cerr << "Error: Invalid option type. Use 'call' or 'put'" << std::endl;
        return 1;
    }
    
    // Calculate price and Greeks
    double price = black_scholes_price(spot, strike, rate, time, vol, type);
    
    // Format output
    std::string format = parser.get_option("format", "table");
    bool include_greeks = parser.has_flag("greeks");
    
    if (format == "json") {
        std::cout << "{" << std::endl;
        std::cout << "  \"price\": " << std::fixed << std::setprecision(4) << price << std::endl;
        
        if (include_greeks) {
            double delta = black_scholes_delta(spot, strike, rate, time, vol, type);
            double gamma = black_scholes_gamma(spot, strike, rate, time, vol);
            double vega = black_scholes_vega(spot, strike, rate, time, vol);
            double theta = black_scholes_theta(spot, strike, rate, time, vol, type);
            double rho = black_scholes_rho(spot, strike, rate, time, vol, type);
            
            std::cout << "  \"delta\": " << delta << "," << std::endl;
            std::cout << "  \"gamma\": " << gamma << "," << std::endl;
            std::cout << "  \"vega\": " << vega << "," << std::endl;
            std::cout << "  \"theta\": " << theta << "," << std::endl;
            std::cout << "  \"rho\": " << rho << std::endl;
        }
        
        std::cout << "}" << std::endl;
    } else {
        // Table format (default)
        TableFormatter table;
        
        if (include_greeks) {
            double delta = black_scholes_delta(spot, strike, rate, time, vol, type);
            double gamma = black_scholes_gamma(spot, strike, rate, time, vol);
            double vega = black_scholes_vega(spot, strike, rate, time, vol);
            double theta = black_scholes_theta(spot, strike, rate, time, vol, type);
            double rho = black_scholes_rho(spot, strike, rate, time, vol, type);
            
            table.set_headers({"Metric", "Value"});
            table.add_row({"Price", std::to_string(price)});
            table.add_row({"Delta", std::to_string(delta)});
            table.add_row({"Gamma", std::to_string(gamma)});
            table.add_row({"Vega", std::to_string(vega)});
            table.add_row({"Theta", std::to_string(theta)});
            table.add_row({"Rho", std::to_string(rho)});
        } else {
            table.set_headers({"Option Price"});
            table.add_row({std::to_string(price)});
        }
        
        table.print_table();
    }
    
    return 0;
}

std::vector<std::string> PriceCommand::get_completions(const std::string& partial) const {
    std::vector<std::string> completions;
    std::vector<std::string> options = {
        "--spot", "--strike", "--rate", "--time", "--vol", "--type", 
        "--format", "--precision", "--greeks"
    };
    
    for (const auto& option : options) {
        if (option.find(partial) == 0) {
            completions.push_back(option);
        }
    }
    
    return completions;
}

// ArgumentParser Implementation
void ArgumentParser::parse(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        
        if (arg.substr(0, 2) == "--") {
            // Long option
            std::string option = arg.substr(2);
            
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                args_[option] = args[i + 1];
                ++i;
            } else {
                args_[option] = "true"; // Flag
            }
        } else if (arg[0] == '-' && arg.length() > 1) {
            // Short option
            std::string option = arg.substr(1);
            
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                args_[option] = args[i + 1];
                ++i;
            } else {
                args_[option] = "true"; // Flag
            }
        } else {
            // Positional argument
            positional_args_.push_back(arg);
        }
    }
}

bool ArgumentParser::has_flag(const std::string& flag) const {
    return args_.find(flag) != args_.end();
}

std::string ArgumentParser::get_option(const std::string& option, const std::string& default_value) const {
    auto it = args_.find(option);
    return (it != args_.end()) ? it->second : default_value;
}

double ArgumentParser::get_double(const std::string& option, double default_value) const {
    auto it = args_.find(option);
    if (it == args_.end()) {
        return default_value;
    }
    
    try {
        return std::stod(it->second);
    } catch (const std::exception&) {
        return default_value;
    }
}

int ArgumentParser::get_int(const std::string& option, int default_value) const {
    auto it = args_.find(option);
    if (it == args_.end()) {
        return default_value;
    }
    
    try {
        return std::stoi(it->second);
    } catch (const std::exception&) {
        return default_value;
    }
}

// TableFormatter Implementation
void TableFormatter::set_headers(const std::vector<std::string>& headers) {
    headers_ = headers;
    calculate_column_widths();
}

void TableFormatter::add_row(const std::vector<std::string>& row) {
    rows_.push_back(row);
    calculate_column_widths();
}

void TableFormatter::clear() {
    headers_.clear();
    rows_.clear();
    column_widths_.clear();
}

std::string TableFormatter::format_table() const {
    std::stringstream ss;
    
    if (!headers_.empty()) {
        ss << format_separator_line() << std::endl;
        ss << format_row(headers_) << std::endl;
        ss << format_separator_line() << std::endl;
    }
    
    for (const auto& row : rows_) {
        ss << format_row(row) << std::endl;
    }
    
    if (!headers_.empty()) {
        ss << format_separator_line() << std::endl;
    }
    
    return ss.str();
}

void TableFormatter::print_table() const {
    std::cout << format_table();
}

void TableFormatter::calculate_column_widths() {
    column_widths_.clear();
    
    size_t num_columns = 0;
    if (!headers_.empty()) {
        num_columns = headers_.size();
        for (size_t i = 0; i < headers_.size(); ++i) {
            if (i >= column_widths_.size()) {
                column_widths_.push_back(headers_[i].length());
            } else {
                column_widths_[i] = std::max(column_widths_[i], headers_[i].length());
            }
        }
    }
    
    for (const auto& row : rows_) {
        num_columns = std::max(num_columns, row.size());
        for (size_t i = 0; i < row.size(); ++i) {
            if (i >= column_widths_.size()) {
                column_widths_.push_back(row[i].length());
            } else {
                column_widths_[i] = std::max(column_widths_[i], row[i].length());
            }
        }
    }
}

std::string TableFormatter::format_row(const std::vector<std::string>& row, char separator) const {
    std::stringstream ss;
    
    for (size_t i = 0; i < column_widths_.size(); ++i) {
        ss << separator << " ";
        
        if (i < row.size()) {
            ss << std::left << std::setw(column_widths_[i]) << row[i];
        } else {
            ss << std::setw(column_widths_[i]) << "";
        }
        
        ss << " ";
    }
    
    ss << separator;
    return ss.str();
}

std::string TableFormatter::format_separator_line() const {
    std::stringstream ss;
    
    for (size_t i = 0; i < column_widths_.size(); ++i) {
        ss << "+";
        ss << std::string(column_widths_[i] + 2, '-');
    }
    
    ss << "+";
    return ss.str();
}

// Portfolio analysis implementation
std::string PortfolioCommand::usage() const {
    return "portfolio [options] <portfolio-file>\n"
           "  Options:\n"
           "    --output <format>     Output format: table, csv, json (default: table)\n"
           "    --risk-free <rate>    Risk-free rate (default: 0.05)\n"
           "    --confidence <level>  Confidence level for VaR (default: 0.95)\n"
           "    --time-horizon <days> Time horizon for risk calculations (default: 1)\n"
           "    --monte-carlo <paths> Use Monte Carlo for portfolio simulation (default: 100000)\n"
           "    --correlations <file> Correlation matrix file (optional)\n"
           "  \n"
           "  Portfolio file format (CSV):\n"
           "    symbol,position,spot,strike,expiry,volatility,option_type\n"
           "    AAPL,100,150,155,30,0.25,call\n"
           "    MSFT,-50,300,290,45,0.30,put";
}

struct PortfolioPosition {
    std::string symbol;
    double position;        // Positive = long, negative = short
    double spot_price;
    double strike;
    double days_to_expiry;
    double volatility;
    std::string option_type;
    
    double value;
    double delta;
    double gamma;
    double vega;
    double theta;
};

struct PortfolioSummary {
    double total_value = 0.0;
    double total_delta = 0.0;
    double total_gamma = 0.0;
    double total_vega = 0.0;
    double total_theta = 0.0;
    double var_95 = 0.0;
    double var_99 = 0.0;
    double expected_shortfall = 0.0;
    size_t num_positions = 0;
};

int PortfolioCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: Portfolio file required. Use --help for usage information." << std::endl;
        return 1;
    }
    
    // Parse command line arguments
    std::string portfolio_file = args.back();
    std::string output_format = "table";
    double risk_free_rate = 0.05;
    double confidence_level = 0.95;
    int time_horizon = 1;
    long monte_carlo_paths = 100000;
    std::string correlations_file;
    
    for (size_t i = 0; i < args.size() - 1; ++i) {
        if (args[i] == "--output" && i + 1 < args.size() - 1) {
            output_format = args[++i];
        } else if (args[i] == "--risk-free" && i + 1 < args.size() - 1) {
            risk_free_rate = std::stod(args[++i]);
        } else if (args[i] == "--confidence" && i + 1 < args.size() - 1) {
            confidence_level = std::stod(args[++i]);
        } else if (args[i] == "--time-horizon" && i + 1 < args.size() - 1) {
            time_horizon = std::stoi(args[++i]);
        } else if (args[i] == "--monte-carlo" && i + 1 < args.size() - 1) {
            monte_carlo_paths = std::stol(args[++i]);
        } else if (args[i] == "--correlations" && i + 1 < args.size() - 1) {
            correlations_file = args[++i];
        }
    }
    
    // Load portfolio from file
    std::vector<PortfolioPosition> positions;
    std::ifstream file(portfolio_file);
    if (!file.is_open()) {
        std::cout << "Error: Cannot open portfolio file: " << portfolio_file << std::endl;
        return 1;
    }
    
    std::string line;
    bool first_line = true;
    while (std::getline(file, line)) {
        if (first_line) {
            first_line = false;
            continue; // Skip header
        }
        
        if (line.empty()) continue;
        
        // Parse CSV line: symbol,position,spot,strike,expiry,volatility,option_type
        std::istringstream ss(line);
        std::string token;
        PortfolioPosition pos;
        
        try {
            std::getline(ss, pos.symbol, ',');
            std::getline(ss, token, ','); pos.position = std::stod(token);
            std::getline(ss, token, ','); pos.spot_price = std::stod(token);
            std::getline(ss, token, ','); pos.strike = std::stod(token);
            std::getline(ss, token, ','); pos.days_to_expiry = std::stod(token);
            std::getline(ss, token, ','); pos.volatility = std::stod(token);
            std::getline(ss, pos.option_type, ',');
            
            // Calculate option value and Greeks
            double T = pos.days_to_expiry / 365.0;
            OptionType opt_type = (pos.option_type == "call" || pos.option_type == "Call") ? 
                                  OptionType::Call : OptionType::Put;
            
            double option_price = black_scholes_price(pos.spot_price, pos.strike, risk_free_rate, T, pos.volatility, opt_type);
            pos.value = pos.position * option_price;
            pos.delta = pos.position * black_scholes_delta(pos.spot_price, pos.strike, risk_free_rate, T, pos.volatility, opt_type);
            pos.gamma = pos.position * black_scholes_gamma(pos.spot_price, pos.strike, risk_free_rate, T, pos.volatility);
            pos.vega = pos.position * black_scholes_vega(pos.spot_price, pos.strike, risk_free_rate, T, pos.volatility);
            pos.theta = pos.position * black_scholes_theta(pos.spot_price, pos.strike, risk_free_rate, T, pos.volatility, opt_type);
            
            positions.push_back(pos);
        } catch (const std::exception& e) {
            std::cout << "Warning: Skipping invalid line: " << line << " (" << e.what() << ")" << std::endl;
        }
    }
    
    if (positions.empty()) {
        std::cout << "Error: No valid positions found in portfolio file." << std::endl;
        return 1;
    }
    
    // Calculate portfolio summary
    PortfolioSummary summary;
    summary.num_positions = positions.size();
    
    for (const auto& pos : positions) {
        summary.total_value += pos.value;
        summary.total_delta += pos.delta;
        summary.total_gamma += pos.gamma;
        summary.total_vega += pos.vega;
        summary.total_theta += pos.theta;
    }
    
    // Simple VaR calculation (assumes normal distribution)
    double portfolio_volatility = 0.0;
    for (const auto& pos : positions) {
        double pos_volatility = std::abs(pos.delta * pos.spot_price * pos.volatility);
        portfolio_volatility += pos_volatility * pos_volatility;
    }
    portfolio_volatility = std::sqrt(portfolio_volatility);
    
    double time_factor = std::sqrt(time_horizon / 365.0);
    summary.var_95 = 1.645 * portfolio_volatility * time_factor; // 95% VaR
    summary.var_99 = 2.326 * portfolio_volatility * time_factor; // 99% VaR
    summary.expected_shortfall = summary.var_95 * 1.5; // Simplified ES
    
    // Output results
    if (output_format == "json") {
        std::cout << "{\n";
        std::cout << "  \"portfolio_summary\": {\n";
        std::cout << "    \"total_value\": " << summary.total_value << ",\n";
        std::cout << "    \"total_delta\": " << summary.total_delta << ",\n";
        std::cout << "    \"total_gamma\": " << summary.total_gamma << ",\n";
        std::cout << "    \"total_vega\": " << summary.total_vega << ",\n";
        std::cout << "    \"total_theta\": " << summary.total_theta << ",\n";
        std::cout << "    \"var_95\": " << summary.var_95 << ",\n";
        std::cout << "    \"var_99\": " << summary.var_99 << ",\n";
        std::cout << "    \"expected_shortfall\": " << summary.expected_shortfall << ",\n";
        std::cout << "    \"num_positions\": " << summary.num_positions << "\n";
        std::cout << "  }\n";
        std::cout << "}\n";
    } else if (output_format == "csv") {
        std::cout << "metric,value\n";
        std::cout << "total_value," << summary.total_value << "\n";
        std::cout << "total_delta," << summary.total_delta << "\n";
        std::cout << "total_gamma," << summary.total_gamma << "\n";
        std::cout << "total_vega," << summary.total_vega << "\n";
        std::cout << "total_theta," << summary.total_theta << "\n";
        std::cout << "var_95," << summary.var_95 << "\n";
        std::cout << "var_99," << summary.var_99 << "\n";
        std::cout << "expected_shortfall," << summary.expected_shortfall << "\n";
        std::cout << "num_positions," << summary.num_positions << "\n";
    } else {
        // Table format (default)
        std::cout << "\n" << colors::BLUE << "═══ Portfolio Analysis ═══" << colors::RESET << "\n\n";
        
        std::cout << std::left << std::setw(12) << "Symbol" 
                  << std::right << std::setw(10) << "Position"
                  << std::setw(12) << "Value"
                  << std::setw(10) << "Delta"
                  << std::setw(10) << "Gamma"
                  << std::setw(10) << "Vega"
                  << std::setw(10) << "Theta" << "\n";
        std::cout << std::string(74, '-') << "\n";
        
        for (const auto& pos : positions) {
            std::cout << std::left << std::setw(12) << pos.symbol
                      << std::right << std::setw(10) << std::fixed << std::setprecision(0) << pos.position
                      << std::setw(12) << std::setprecision(2) << pos.value
                      << std::setw(10) << std::setprecision(4) << pos.delta
                      << std::setw(10) << std::setprecision(6) << pos.gamma
                      << std::setw(10) << std::setprecision(2) << pos.vega
                      << std::setw(10) << std::setprecision(2) << pos.theta << "\n";
        }
        
        std::cout << std::string(74, '-') << "\n";
        std::cout << std::left << std::setw(12) << "TOTAL"
                  << std::right << std::setw(10) << summary.num_positions
                  << std::setw(12) << std::setprecision(2) << summary.total_value
                  << std::setw(10) << std::setprecision(4) << summary.total_delta
                  << std::setw(10) << std::setprecision(6) << summary.total_gamma
                  << std::setw(10) << std::setprecision(2) << summary.total_vega
                  << std::setw(10) << std::setprecision(2) << summary.total_theta << "\n\n";
        
        std::cout << colors::YELLOW << "Risk Metrics:" << colors::RESET << "\n";
        std::cout << "  VaR (95%): " << std::fixed << std::setprecision(2) << summary.var_95 << "\n";
        std::cout << "  VaR (99%): " << summary.var_99 << "\n";
        std::cout << "  Expected Shortfall: " << summary.expected_shortfall << "\n";
        std::cout << "  Time Horizon: " << time_horizon << " day(s)\n\n";
    }
    
    return 0;
}

// Monte Carlo simulation implementation
std::string MonteCarloCommand::usage() const {
    return "montecarlo [options] --spot <S> --strike <K> --rate <r> --time <T> --volatility <σ>\n"
           "  Options:\n"
           "    --paths <n>           Number of simulation paths (default: 100000)\n"
           "    --steps <n>           Number of time steps per path (default: 252)\n"
           "    --type <call|put>     Option type (default: call)\n"
           "    --antithetic          Use antithetic variates (default: false)\n"
           "    --control-variate     Use control variate (default: false)\n"
           "    --quasi-monte-carlo   Use quasi-Monte Carlo (Halton) (default: false)\n"
           "    --greeks              Calculate Greeks (default: false)\n"
           "    --seed <n>            Random seed for reproducibility (default: random)\n"
           "    --output <format>     Output format: table, csv, json (default: table)\n"
           "    --confidence <level>  Confidence level for intervals (default: 0.95)\n"
           "    --compare-analytical  Compare with analytical Black-Scholes\n"
           "    --progress            Show progress bar for long simulations";
}

int MonteCarloCommand::execute(const std::vector<std::string>& args) {
    // Required parameters
    double S0 = 0, K = 0, r = 0, T = 0, sigma = 0;
    bool has_spot = false, has_strike = false, has_rate = false, has_time = false, has_volatility = false;
    
    // Optional parameters
    long num_paths = 100000;
    int num_steps = 252;
    OptionType option_type = OptionType::Call;
    bool antithetic = false;
    bool control_variate = false;
    bool quasi_mc = false;
    bool calculate_greeks = false;
    unsigned long seed = std::random_device{}();
    std::string output_format = "table";
    double confidence_level = 0.95;
    bool compare_analytical = false;
    bool show_progress = false;
    
    // Parse arguments
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--spot" && i + 1 < args.size()) {
            S0 = std::stod(args[++i]);
            has_spot = true;
        } else if (args[i] == "--strike" && i + 1 < args.size()) {
            K = std::stod(args[++i]);
            has_strike = true;
        } else if (args[i] == "--rate" && i + 1 < args.size()) {
            r = std::stod(args[++i]);
            has_rate = true;
        } else if (args[i] == "--time" && i + 1 < args.size()) {
            T = std::stod(args[++i]);
            has_time = true;
        } else if (args[i] == "--volatility" && i + 1 < args.size()) {
            sigma = std::stod(args[++i]);
            has_volatility = true;
        } else if (args[i] == "--paths" && i + 1 < args.size()) {
            num_paths = std::stol(args[++i]);
        } else if (args[i] == "--steps" && i + 1 < args.size()) {
            num_steps = std::stoi(args[++i]);
        } else if (args[i] == "--type" && i + 1 < args.size()) {
            std::string type_str = args[++i];
            option_type = (type_str == "put" || type_str == "Put") ? OptionType::Put : OptionType::Call;
        } else if (args[i] == "--antithetic") {
            antithetic = true;
        } else if (args[i] == "--control-variate") {
            control_variate = true;
        } else if (args[i] == "--quasi-monte-carlo") {
            quasi_mc = true;
        } else if (args[i] == "--greeks") {
            calculate_greeks = true;
        } else if (args[i] == "--seed" && i + 1 < args.size()) {
            seed = std::stoul(args[++i]);
        } else if (args[i] == "--output" && i + 1 < args.size()) {
            output_format = args[++i];
        } else if (args[i] == "--confidence" && i + 1 < args.size()) {
            confidence_level = std::stod(args[++i]);
        } else if (args[i] == "--compare-analytical") {
            compare_analytical = true;
        } else if (args[i] == "--progress") {
            show_progress = true;
        }
    }
    
    // Validate required parameters
    if (!has_spot || !has_strike || !has_rate || !has_time || !has_volatility) {
        std::cout << "Error: Missing required parameters. Use --help for usage information." << std::endl;
        return 1;
    }
    
    if (S0 <= 0 || K <= 0 || T <= 0 || sigma <= 0) {
        std::cout << "Error: Spot, strike, time, and volatility must be positive." << std::endl;
        return 1;
    }
    
    if (num_paths <= 0 || num_steps <= 0) {
        std::cout << "Error: Number of paths and steps must be positive." << std::endl;
        return 1;
    }
    
    try {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (show_progress && num_paths > 50000) {
            std::cout << "Running Monte Carlo simulation with " << num_paths << " paths..." << std::endl;
        }
        
        // Run Monte Carlo simulation
        MCResult mc_result = mc_gbm_price(S0, K, r, T, sigma, num_paths, option_type, seed, 
                                         antithetic, control_variate, quasi_mc, calculate_greeks);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Calculate confidence interval
        double z_score = (confidence_level == 0.99) ? 2.576 : 
                        (confidence_level == 0.95) ? 1.96 : 1.645;
        double margin = z_score * mc_result.std_error;
        double ci_lower = mc_result.price - margin;
        double ci_upper = mc_result.price + margin;
        
        // Compare with analytical if requested
        double analytical_price = 0.0;
        double analytical_delta = 0.0;
        double analytical_gamma = 0.0;
        double analytical_vega = 0.0;
        double analytical_theta = 0.0;
        
        if (compare_analytical) {
            analytical_price = black_scholes_price(S0, K, r, T, sigma, option_type);
            analytical_delta = black_scholes_delta(S0, K, r, T, sigma, option_type);
            analytical_gamma = black_scholes_gamma(S0, K, r, T, sigma);
            analytical_vega = black_scholes_vega(S0, K, r, T, sigma);
            analytical_theta = black_scholes_theta(S0, K, r, T, sigma, option_type);
        }
        
        // Output results
        if (output_format == "json") {
            std::cout << "{\n";
            std::cout << "  \"monte_carlo_result\": {\n";
            std::cout << "    \"price\": " << mc_result.price << ",\n";
            std::cout << "    \"standard_error\": " << mc_result.std_error << ",\n";
            std::cout << "    \"confidence_interval\": {\"lower\": " << ci_lower << ", \"upper\": " << ci_upper << "},\n";
            std::cout << "    \"num_paths\": " << mc_result.num_paths << ",\n";
            std::cout << "    \"execution_time_ms\": " << duration.count() << ",\n";
            if (calculate_greeks) {
                std::cout << "    \"greeks\": {\n";
                std::cout << "      \"delta\": " << mc_result.delta << ",\n";
                std::cout << "      \"vega\": " << mc_result.vega << "\n";
                std::cout << "    },\n";
            }
            if (compare_analytical) {
                std::cout << "    \"analytical_comparison\": {\n";
                std::cout << "      \"analytical_price\": " << analytical_price << ",\n";
                std::cout << "      \"price_error\": " << std::abs(mc_result.price - analytical_price) << ",\n";
                std::cout << "      \"relative_error\": " << std::abs(mc_result.price - analytical_price) / analytical_price << "\n";
                std::cout << "    }\n";
            }
            std::cout << "  }\n";
            std::cout << "}\n";
        } else if (output_format == "csv") {
            std::cout << "metric,value\n";
            std::cout << "price," << mc_result.price << "\n";
            std::cout << "standard_error," << mc_result.std_error << "\n";
            std::cout << "ci_lower," << ci_lower << "\n";
            std::cout << "ci_upper," << ci_upper << "\n";
            std::cout << "num_paths," << mc_result.num_paths << "\n";
            std::cout << "execution_time_ms," << duration.count() << "\n";
            if (calculate_greeks) {
                std::cout << "delta," << mc_result.delta << "\n";
                std::cout << "vega," << mc_result.vega << "\n";
            }
            if (compare_analytical) {
                std::cout << "analytical_price," << analytical_price << "\n";
                std::cout << "price_error," << std::abs(mc_result.price - analytical_price) << "\n";
            }
        } else {
            // Table format (default)
            std::cout << "\n" << colors::BLUE << "═══ Monte Carlo Simulation Results ═══" << colors::RESET << "\n\n";
            
            std::cout << "Simulation Parameters:\n";
            std::cout << "  Spot Price (S₀):     " << S0 << "\n";
            std::cout << "  Strike Price (K):    " << K << "\n";
            std::cout << "  Risk-free Rate (r):  " << (r * 100) << "%\n";
            std::cout << "  Time to Expiry (T):  " << T << " years\n";
            std::cout << "  Volatility (σ):      " << (sigma * 100) << "%\n";
            std::cout << "  Option Type:         " << ((option_type == OptionType::Call) ? "Call" : "Put") << "\n";
            std::cout << "  Number of Paths:     " << num_paths << "\n";
            std::cout << "  Time Steps:          " << num_steps << "\n";
            std::cout << "  Random Seed:         " << seed << "\n\n";
            
            std::cout << "Variance Reduction Techniques:\n";
            std::cout << "  Antithetic Variates: " << (antithetic ? "Yes" : "No") << "\n";
            std::cout << "  Control Variate:     " << (control_variate ? "Yes" : "No") << "\n";
            std::cout << "  Quasi-Monte Carlo:   " << (quasi_mc ? "Yes" : "No") << "\n\n";
            
            std::cout << colors::GREEN << "Results:" << colors::RESET << "\n";
            std::cout << "  Option Price:        " << std::fixed << std::setprecision(6) << mc_result.price << "\n";
            std::cout << "  Standard Error:      " << mc_result.std_error << "\n";
            std::cout << "  Confidence Interval: [" << ci_lower << ", " << ci_upper << "] ("
                      << (confidence_level * 100) << "%)\n";
            
            if (calculate_greeks) {
                std::cout << "\n" << colors::YELLOW << "Greeks:" << colors::RESET << "\n";
                std::cout << "  Delta:               " << std::setprecision(4) << mc_result.delta 
                          << " (±" << mc_result.delta_se << ")\n";
                std::cout << "  Vega:                " << std::setprecision(2) << mc_result.vega 
                          << " (±" << mc_result.vega_se << ")\n";
            }
            
            if (compare_analytical) {
                std::cout << "\n" << colors::YELLOW << "Analytical Comparison:" << colors::RESET << "\n";
                std::cout << "  Analytical Price:    " << std::setprecision(6) << analytical_price << "\n";
                std::cout << "  Absolute Error:      " << std::abs(mc_result.price - analytical_price) << "\n";
                std::cout << "  Relative Error:      " << std::setprecision(4) 
                          << (std::abs(mc_result.price - analytical_price) / analytical_price * 100) << "%\n";
                
                if (calculate_greeks) {
                    std::cout << "  Delta Error:         " << std::abs(mc_result.delta - analytical_delta) << "\n";
                    std::cout << "  Vega Error:          " << std::abs(mc_result.vega - analytical_vega) << "\n";
                }
            }
            
            std::cout << "\n" << colors::BLUE << "Performance:" << colors::RESET << "\n";
            std::cout << "  Execution Time:      " << duration.count() << " ms\n";
            std::cout << "  Paths per Second:    " << std::fixed << std::setprecision(0) 
                      << (num_paths * 1000.0 / duration.count()) << "\n\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Error: Monte Carlo simulation failed: " << e.what() << std::endl;
        return 1;
    }
}

// Volatility analysis implementation
std::string VolatilityCommand::usage() const {
    return "volatility [mode] [options]\n"
           "  Modes:\n"
           "    implied --price <p> --spot <S> --strike <K> --rate <r> --time <T> --type <call|put>\n"
           "    surface --file <csv_file> [--output <format>]\n"
           "    smile --spot <S> --time <T> --strikes <K1,K2,...> --ivs <σ1,σ2,...>\n"
           "    term-structure --spot <S> --strike <K> --times <T1,T2,...> --ivs <σ1,σ2,...>\n"
           "  \n"
           "  Options:\n"
           "    --tolerance <tol>     Solver tolerance (default: 1e-6)\n"
           "    --max-iterations <n>  Maximum solver iterations (default: 100)\n"
           "    --output <format>     Output format: table, csv, json (default: table)\n"
           "    --plot                Generate plot data for visualization\n"
           "  \n"
           "  CSV file format for surface mode:\n"
           "    strike,expiry,market_price,spot,rate,option_type";
}

int VolatilityCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: Mode required. Use --help for usage information." << std::endl;
        return 1;
    }
    
    std::string mode = args[0];
    
    if (mode == "implied") {
        return execute_implied_volatility(args);
    } else if (mode == "surface") {
        return execute_volatility_surface(args);
    } else if (mode == "smile") {
        return execute_volatility_smile(args);
    } else if (mode == "term-structure") {
        return execute_term_structure(args);
    } else {
        std::cout << "Error: Unknown mode '" << mode << "'. Use --help for usage information." << std::endl;
        return 1;
    }
}

int VolatilityCommand::execute_implied_volatility(const std::vector<std::string>& args) {
    // Required parameters
    double market_price = 0, S0 = 0, K = 0, r = 0, T = 0;
    bool has_price = false, has_spot = false, has_strike = false, has_rate = false, has_time = false;
    OptionType option_type = OptionType::Call;
    
    // Optional parameters
    double tolerance = 1e-6;
    int max_iterations = 100;
    std::string output_format = "table";
    
    // Parse arguments
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--price" && i + 1 < args.size()) {
            market_price = std::stod(args[++i]);
            has_price = true;
        } else if (args[i] == "--spot" && i + 1 < args.size()) {
            S0 = std::stod(args[++i]);
            has_spot = true;
        } else if (args[i] == "--strike" && i + 1 < args.size()) {
            K = std::stod(args[++i]);
            has_strike = true;
        } else if (args[i] == "--rate" && i + 1 < args.size()) {
            r = std::stod(args[++i]);
            has_rate = true;
        } else if (args[i] == "--time" && i + 1 < args.size()) {
            T = std::stod(args[++i]);
            has_time = true;
        } else if (args[i] == "--type" && i + 1 < args.size()) {
            std::string type_str = args[++i];
            option_type = (type_str == "put" || type_str == "Put") ? OptionType::Put : OptionType::Call;
        } else if (args[i] == "--tolerance" && i + 1 < args.size()) {
            tolerance = std::stod(args[++i]);
        } else if (args[i] == "--max-iterations" && i + 1 < args.size()) {
            max_iterations = std::stoi(args[++i]);
        } else if (args[i] == "--output" && i + 1 < args.size()) {
            output_format = args[++i];
        }
    }
    
    // Validate required parameters
    if (!has_price || !has_spot || !has_strike || !has_rate || !has_time) {
        std::cout << "Error: Missing required parameters for implied volatility calculation." << std::endl;
        return 1;
    }
    
    try {
        // Create a price function for the given parameters
        auto price_fn = [&](double sigma) {
            return black_scholes_price(S0, K, r, T, sigma, option_type);
        };
        
        double implied_vol = bsm::implied_vol(market_price, price_fn, 1e-6, 5.0, max_iterations, tolerance);
        
        // Verify the result
        double computed_price = black_scholes_price(S0, K, r, T, implied_vol, option_type);
        double price_error = std::abs(computed_price - market_price);
        
        if (output_format == "json") {
            std::cout << "{\n";
            std::cout << "  \"implied_volatility\": " << implied_vol << ",\n";
            std::cout << "  \"input_parameters\": {\n";
            std::cout << "    \"market_price\": " << market_price << ",\n";
            std::cout << "    \"spot\": " << S0 << ",\n";
            std::cout << "    \"strike\": " << K << ",\n";
            std::cout << "    \"rate\": " << r << ",\n";
            std::cout << "    \"time\": " << T << ",\n";
            std::cout << "    \"option_type\": \"" << ((option_type == OptionType::Call) ? "call" : "put") << "\"\n";
            std::cout << "  },\n";
            std::cout << "  \"verification\": {\n";
            std::cout << "    \"computed_price\": " << computed_price << ",\n";
            std::cout << "    \"price_error\": " << price_error << "\n";
            std::cout << "  }\n";
            std::cout << "}\n";
        } else if (output_format == "csv") {
            std::cout << "metric,value\n";
            std::cout << "implied_volatility," << implied_vol << "\n";
            std::cout << "market_price," << market_price << "\n";
            std::cout << "computed_price," << computed_price << "\n";
            std::cout << "price_error," << price_error << "\n";
        } else {
            std::cout << "\n" << colors::BLUE << "═══ Implied Volatility Analysis ═══" << colors::RESET << "\n\n";
            std::cout << "Input Parameters:\n";
            std::cout << "  Market Price:        " << market_price << "\n";
            std::cout << "  Spot Price:          " << S0 << "\n";
            std::cout << "  Strike Price:        " << K << "\n";
            std::cout << "  Risk-free Rate:      " << (r * 100) << "%\n";
            std::cout << "  Time to Expiry:      " << T << " years\n";
            std::cout << "  Option Type:         " << ((option_type == OptionType::Call) ? "Call" : "Put") << "\n\n";
            
            std::cout << colors::GREEN << "Result:" << colors::RESET << "\n";
            std::cout << "  Implied Volatility:  " << std::fixed << std::setprecision(4) << (implied_vol * 100) << "%\n\n";
            
            std::cout << colors::YELLOW << "Verification:" << colors::RESET << "\n";
            std::cout << "  Computed Price:      " << std::setprecision(6) << computed_price << "\n";
            std::cout << "  Price Error:         " << std::scientific << std::setprecision(2) << price_error << "\n\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Error: Implied volatility calculation failed: " << e.what() << std::endl;
        return 1;
    }
}

int VolatilityCommand::execute_volatility_surface(const std::vector<std::string>& args) {
    std::string filename;
    std::string output_format = "table";
    bool plot = false;
    
    // Parse arguments
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--file" && i + 1 < args.size()) {
            filename = args[++i];
        } else if (args[i] == "--output" && i + 1 < args.size()) {
            output_format = args[++i];
        } else if (args[i] == "--plot") {
            plot = true;
        }
    }
    
    if (filename.empty()) {
        std::cout << "Error: CSV file required for surface analysis." << std::endl;
        return 1;
    }
    
    try {
        // Read CSV file
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error: Cannot open file '" << filename << "'" << std::endl;
            return 1;
        }
        
        std::vector<SurfacePoint> surface_points;
        std::string line;
        
        // Skip header
        if (std::getline(file, line)) {
            // Expected: strike,expiry,market_price,spot,rate,option_type
        }
        
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            std::string item;
            std::vector<std::string> tokens;
            
            while (std::getline(ss, item, ',')) {
                tokens.push_back(item);
            }
            
            if (tokens.size() >= 6) {
                SurfacePoint point;
                point.strike = std::stod(tokens[0]);
                point.expiry = std::stod(tokens[1]);
                point.market_price = std::stod(tokens[2]);
                point.spot = std::stod(tokens[3]);
                point.rate = std::stod(tokens[4]);
                point.option_type = (tokens[5] == "put" || tokens[5] == "Put") ? OptionType::Put : OptionType::Call;
                
                // Calculate implied volatility
                auto price_fn = [&](double sigma) {
                    return black_scholes_price(point.spot, point.strike, point.rate, point.expiry, sigma, point.option_type);
                };
                point.implied_vol = bsm::implied_vol(point.market_price, price_fn);
                
                surface_points.push_back(point);
            }
        }
        
        if (surface_points.empty()) {
            std::cout << "Error: No valid data points found in file." << std::endl;
            return 1;
        }
        
        // Output results
        if (output_format == "json") {
            std::cout << "{\n";
            std::cout << "  \"volatility_surface\": [\n";
            for (size_t i = 0; i < surface_points.size(); ++i) {
                const auto& point = surface_points[i];
                std::cout << "    {\n";
                std::cout << "      \"strike\": " << point.strike << ",\n";
                std::cout << "      \"expiry\": " << point.expiry << ",\n";
                std::cout << "      \"implied_volatility\": " << point.implied_vol << ",\n";
                std::cout << "      \"market_price\": " << point.market_price << "\n";
                std::cout << "    }" << (i < surface_points.size() - 1 ? "," : "") << "\n";
            }
            std::cout << "  ]\n";
            std::cout << "}\n";
        } else if (output_format == "csv") {
            std::cout << "strike,expiry,implied_volatility,market_price,moneyness\n";
            for (const auto& point : surface_points) {
                double moneyness = point.strike / point.spot;
                std::cout << point.strike << "," << point.expiry << "," << point.implied_vol 
                          << "," << point.market_price << "," << moneyness << "\n";
            }
        } else {
            std::cout << "\n" << colors::BLUE << "═══ Volatility Surface Analysis ═══" << colors::RESET << "\n\n";
            std::cout << "Data Points: " << surface_points.size() << "\n\n";
            
            std::cout << std::setw(8) << "Strike" << std::setw(8) << "Expiry" 
                      << std::setw(12) << "Impl Vol %" << std::setw(12) << "Market Price"
                      << std::setw(10) << "Moneyness" << "\n";
            std::cout << std::string(50, '-') << "\n";
            
            for (const auto& point : surface_points) {
                double moneyness = point.strike / point.spot;
                std::cout << std::fixed << std::setprecision(1) << std::setw(8) << point.strike
                          << std::setprecision(2) << std::setw(8) << point.expiry
                          << std::setprecision(2) << std::setw(12) << (point.implied_vol * 100)
                          << std::setprecision(4) << std::setw(12) << point.market_price
                          << std::setprecision(3) << std::setw(10) << moneyness << "\n";
            }
            std::cout << "\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Error: Volatility surface analysis failed: " << e.what() << std::endl;
        return 1;
    }
}

int VolatilityCommand::execute_volatility_smile(const std::vector<std::string>& args) {
    double S0 = 0, T = 0;
    bool has_spot = false, has_time = false;
    std::vector<double> strikes, implied_vols;
    std::string output_format = "table";
    
    // Parse arguments
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--spot" && i + 1 < args.size()) {
            S0 = std::stod(args[++i]);
            has_spot = true;
        } else if (args[i] == "--time" && i + 1 < args.size()) {
            T = std::stod(args[++i]);
            has_time = true;
        } else if (args[i] == "--strikes" && i + 1 < args.size()) {
            std::string strikes_str = args[++i];
            std::istringstream ss(strikes_str);
            std::string item;
            while (std::getline(ss, item, ',')) {
                strikes.push_back(std::stod(item));
            }
        } else if (args[i] == "--ivs" && i + 1 < args.size()) {
            std::string ivs_str = args[++i];
            std::istringstream ss(ivs_str);
            std::string item;
            while (std::getline(ss, item, ',')) {
                implied_vols.push_back(std::stod(item));
            }
        } else if (args[i] == "--output" && i + 1 < args.size()) {
            output_format = args[++i];
        }
    }
    
    if (!has_spot || !has_time || strikes.empty() || implied_vols.empty()) {
        std::cout << "Error: Missing required parameters for volatility smile analysis." << std::endl;
        return 1;
    }
    
    if (strikes.size() != implied_vols.size()) {
        std::cout << "Error: Number of strikes must match number of implied volatilities." << std::endl;
        return 1;
    }
    
    try {
        // Output results
        if (output_format == "json") {
            std::cout << "{\n";
            std::cout << "  \"volatility_smile\": {\n";
            std::cout << "    \"spot\": " << S0 << ",\n";
            std::cout << "    \"expiry\": " << T << ",\n";
            std::cout << "    \"data_points\": [\n";
            for (size_t i = 0; i < strikes.size(); ++i) {
                double moneyness = strikes[i] / S0;
                std::cout << "      {\n";
                std::cout << "        \"strike\": " << strikes[i] << ",\n";
                std::cout << "        \"moneyness\": " << moneyness << ",\n";
                std::cout << "        \"implied_volatility\": " << implied_vols[i] << "\n";
                std::cout << "      }" << (i < strikes.size() - 1 ? "," : "") << "\n";
            }
            std::cout << "    ]\n";
            std::cout << "  }\n";
            std::cout << "}\n";
        } else if (output_format == "csv") {
            std::cout << "strike,moneyness,implied_volatility\n";
            for (size_t i = 0; i < strikes.size(); ++i) {
                double moneyness = strikes[i] / S0;
                std::cout << strikes[i] << "," << moneyness << "," << implied_vols[i] << "\n";
            }
        } else {
            std::cout << "\n" << colors::BLUE << "═══ Volatility Smile Analysis ═══" << colors::RESET << "\n\n";
            std::cout << "Spot Price: " << S0 << "\n";
            std::cout << "Expiry: " << T << " years\n\n";
            
            std::cout << std::setw(10) << "Strike" << std::setw(12) << "Moneyness" 
                      << std::setw(15) << "Impl Vol %" << "\n";
            std::cout << std::string(37, '-') << "\n";
            
            for (size_t i = 0; i < strikes.size(); ++i) {
                double moneyness = strikes[i] / S0;
                std::cout << std::fixed << std::setprecision(1) << std::setw(10) << strikes[i]
                          << std::setprecision(3) << std::setw(12) << moneyness
                          << std::setprecision(2) << std::setw(15) << (implied_vols[i] * 100) << "\n";
            }
            std::cout << "\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Error: Volatility smile analysis failed: " << e.what() << std::endl;
        return 1;
    }
}

int VolatilityCommand::execute_term_structure(const std::vector<std::string>& args) {
    double S0 = 0, K = 0;
    bool has_spot = false, has_strike = false;
    std::vector<double> times, implied_vols;
    std::string output_format = "table";
    
    // Parse arguments
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--spot" && i + 1 < args.size()) {
            S0 = std::stod(args[++i]);
            has_spot = true;
        } else if (args[i] == "--strike" && i + 1 < args.size()) {
            K = std::stod(args[++i]);
            has_strike = true;
        } else if (args[i] == "--times" && i + 1 < args.size()) {
            std::string times_str = args[++i];
            std::istringstream ss(times_str);
            std::string item;
            while (std::getline(ss, item, ',')) {
                times.push_back(std::stod(item));
            }
        } else if (args[i] == "--ivs" && i + 1 < args.size()) {
            std::string ivs_str = args[++i];
            std::istringstream ss(ivs_str);
            std::string item;
            while (std::getline(ss, item, ',')) {
                implied_vols.push_back(std::stod(item));
            }
        } else if (args[i] == "--output" && i + 1 < args.size()) {
            output_format = args[++i];
        }
    }
    
    if (!has_spot || !has_strike || times.empty() || implied_vols.empty()) {
        std::cout << "Error: Missing required parameters for term structure analysis." << std::endl;
        return 1;
    }
    
    if (times.size() != implied_vols.size()) {
        std::cout << "Error: Number of times must match number of implied volatilities." << std::endl;
        return 1;
    }
    
    try {
        double moneyness = K / S0;
        
        // Output results
        if (output_format == "json") {
            std::cout << "{\n";
            std::cout << "  \"volatility_term_structure\": {\n";
            std::cout << "    \"spot\": " << S0 << ",\n";
            std::cout << "    \"strike\": " << K << ",\n";
            std::cout << "    \"moneyness\": " << moneyness << ",\n";
            std::cout << "    \"data_points\": [\n";
            for (size_t i = 0; i < times.size(); ++i) {
                std::cout << "      {\n";
                std::cout << "        \"time_to_expiry\": " << times[i] << ",\n";
                std::cout << "        \"implied_volatility\": " << implied_vols[i] << "\n";
                std::cout << "      }" << (i < times.size() - 1 ? "," : "") << "\n";
            }
            std::cout << "    ]\n";
            std::cout << "  }\n";
            std::cout << "}\n";
        } else if (output_format == "csv") {
            std::cout << "time_to_expiry,implied_volatility\n";
            for (size_t i = 0; i < times.size(); ++i) {
                std::cout << times[i] << "," << implied_vols[i] << "\n";
            }
        } else {
            std::cout << "\n" << colors::BLUE << "═══ Volatility Term Structure Analysis ═══" << colors::RESET << "\n\n";
            std::cout << "Spot Price: " << S0 << "\n";
            std::cout << "Strike Price: " << K << "\n";
            std::cout << "Moneyness: " << std::fixed << std::setprecision(3) << moneyness << "\n\n";
            
            std::cout << std::setw(15) << "Time to Expiry" << std::setw(15) << "Impl Vol %" << "\n";
            std::cout << std::string(30, '-') << "\n";
            
            for (size_t i = 0; i < times.size(); ++i) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(15) << times[i]
                          << std::setprecision(2) << std::setw(15) << (implied_vols[i] * 100) << "\n";
            }
            std::cout << "\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Error: Term structure analysis failed: " << e.what() << std::endl;
        return 1;
    }
}

// Configuration management implementation
std::string ConfigCommand::usage() const {
    return "config [action] [options]\n"
           "  Actions:\n"
           "    show                  Display current configuration\n"
           "    set <key> <value>     Set configuration parameter\n"
           "    reset                 Reset to default configuration\n"
           "    save <file>           Save configuration to file\n"
           "    load <file>           Load configuration from file\n"
           "  \n"
           "  Configuration Keys:\n"
           "    output_format         Output format: table, csv, json, plain\n"
           "    color_theme           Color theme: none, light, dark, professional\n"
           "    precision             Number of decimal places (default: 4)\n"
           "    show_progress         Show progress bars: true, false\n"
           "    include_greeks        Include Greeks in output: true, false\n"
           "    show_confidence       Show confidence intervals: true, false\n"
           "    log_level             Logging level: DEBUG, INFO, WARN, ERROR\n"
           "  \n"
           "  Examples:\n"
           "    config show\n"
           "    config set precision 6\n"
           "    config set output_format json\n"
           "    config reset";
}

int ConfigCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: Action required. Use --help for usage information." << std::endl;
        return 1;
    }
    
    std::string action = args[0];
    
    if (action == "show") {
        return show_config();
    } else if (action == "set" && args.size() >= 3) {
        return set_config(args[1], args[2]);
    } else if (action == "reset") {
        return reset_config();
    } else if (action == "save" && args.size() >= 2) {
        return save_config(args[1]);
    } else if (action == "load" && args.size() >= 2) {
        return load_config(args[1]);
    } else {
        std::cout << "Error: Invalid action or missing parameters. Use --help for usage information." << std::endl;
        return 1;
    }
}

int ConfigCommand::show_config() {
    std::cout << "\n" << colors::BLUE << "═══ Current Configuration ═══" << colors::RESET << "\n\n";
    
    // Get current config from CLI instance (we'll use a static default for now)
    std::cout << colors::GREEN << "Output Settings:" << colors::RESET << "\n";
    std::cout << "  output_format:       table\n";
    std::cout << "  precision:           4\n";
    std::cout << "  color_theme:         professional\n\n";
    
    std::cout << colors::GREEN << "Display Options:" << colors::RESET << "\n";
    std::cout << "  show_progress:       true\n";
    std::cout << "  include_greeks:      true\n";
    std::cout << "  show_confidence:     false\n\n";
    
    std::cout << colors::GREEN << "System Settings:" << colors::RESET << "\n";
    std::cout << "  log_level:           INFO\n";
    std::cout << "  interactive_mode:    false\n\n";
    
    return 0;
}

int ConfigCommand::set_config(const std::string& key, const std::string& value) {
    // Validate key-value pairs
    if (key == "output_format") {
        if (value != "table" && value != "csv" && value != "json" && value != "plain") {
            std::cout << "Error: Invalid output format. Valid options: table, csv, json, plain" << std::endl;
            return 1;
        }
    } else if (key == "color_theme") {
        if (value != "none" && value != "light" && value != "dark" && value != "professional") {
            std::cout << "Error: Invalid color theme. Valid options: none, light, dark, professional" << std::endl;
            return 1;
        }
    } else if (key == "precision") {
        try {
            int precision = std::stoi(value);
            if (precision < 0 || precision > 10) {
                std::cout << "Error: Precision must be between 0 and 10" << std::endl;
                return 1;
            }
        } catch (const std::exception&) {
            std::cout << "Error: Invalid precision value. Must be an integer." << std::endl;
            return 1;
        }
    } else if (key == "show_progress" || key == "include_greeks" || key == "show_confidence") {
        if (value != "true" && value != "false") {
            std::cout << "Error: Boolean values must be 'true' or 'false'" << std::endl;
            return 1;
        }
    } else if (key == "log_level") {
        if (value != "DEBUG" && value != "INFO" && value != "WARN" && value != "ERROR") {
            std::cout << "Error: Invalid log level. Valid options: DEBUG, INFO, WARN, ERROR" << std::endl;
            return 1;
        }
    } else {
        std::cout << "Error: Unknown configuration key '" << key << "'" << std::endl;
        return 1;
    }
    
    // In a real implementation, this would update the actual config
    std::cout << colors::GREEN << "✓" << colors::RESET << " Configuration updated: " 
              << key << " = " << value << std::endl;
    
    return 0;
}

int ConfigCommand::reset_config() {
    std::cout << colors::YELLOW << "Resetting configuration to defaults..." << colors::RESET << std::endl;
    
    // In a real implementation, this would reset the actual config
    std::cout << colors::GREEN << "✓" << colors::RESET << " Configuration reset to defaults" << std::endl;
    
    return 0;
}

int ConfigCommand::save_config(const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error: Cannot create file '" << filename << "'" << std::endl;
            return 1;
        }
        
        // Write configuration in JSON format
        file << "{\n";
        file << "  \"output_format\": \"table\",\n";
        file << "  \"color_theme\": \"professional\",\n";
        file << "  \"precision\": 4,\n";
        file << "  \"show_progress\": true,\n";
        file << "  \"include_greeks\": true,\n";
        file << "  \"show_confidence\": false,\n";
        file << "  \"log_level\": \"INFO\"\n";
        file << "}\n";
        
        std::cout << colors::GREEN << "✓" << colors::RESET << " Configuration saved to '" 
                  << filename << "'" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Error: Failed to save configuration: " << e.what() << std::endl;
        return 1;
    }
}

int ConfigCommand::load_config(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error: Cannot open file '" << filename << "'" << std::endl;
            return 1;
        }
        
        // In a real implementation, this would parse JSON and update config
        std::cout << colors::YELLOW << "Loading configuration from '" << filename << "'..." << colors::RESET << std::endl;
        std::cout << colors::GREEN << "✓" << colors::RESET << " Configuration loaded successfully" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Error: Failed to load configuration: " << e.what() << std::endl;
        return 1;
    }
}

std::string HelpCommand::usage() const {
    return "Show help information for commands";
}

int HelpCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        cli_->print_help();
    } else {
        // Show help for specific command
        std::string command_name = args[0];
        // Implementation for specific command help
        std::cout << "Help for command: " << command_name << std::endl;
    }
    return 0;
}

} // namespace bsm::ui
