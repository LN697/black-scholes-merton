#include "enhanced_cli.hpp"
#include "analytic_bs.hpp"
#include "monte_carlo_gbm.hpp"
#include "iv_solve.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>

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

// Placeholder implementations for other commands
std::string PortfolioCommand::usage() const {
    return "Portfolio analysis command - implementation pending";
}

int PortfolioCommand::execute(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    std::cout << "Portfolio analysis feature coming soon!" << std::endl;
    return 0;
}

std::string MonteCarloCommand::usage() const {
    return "Monte Carlo simulation command - implementation pending";
}

int MonteCarloCommand::execute(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    std::cout << "Monte Carlo simulation feature coming soon!" << std::endl;
    return 0;
}

std::string VolatilityCommand::usage() const {
    return "Volatility analysis command - implementation pending";
}

int VolatilityCommand::execute(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    std::cout << "Volatility analysis feature coming soon!" << std::endl;
    return 0;
}

std::string ConfigCommand::usage() const {
    return "Configuration management command - implementation pending";
}

int ConfigCommand::execute(const std::vector<std::string>& args) {
    (void)args; // Suppress unused parameter warning
    std::cout << "Configuration management feature coming soon!" << std::endl;
    return 0;
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
