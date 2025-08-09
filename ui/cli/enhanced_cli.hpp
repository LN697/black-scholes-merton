#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

namespace bsm::ui {

/**
 * @brief Enhanced command-line interface for BSM toolkit
 * Provides rich, interactive CLI experience while maintaining backward compatibility
 */

enum class OutputFormat {
    PLAIN,
    JSON,
    CSV,
    TABLE
};

enum class ColorTheme {
    NONE,
    LIGHT,
    DARK,
    PROFESSIONAL
};

struct CLIConfig {
    OutputFormat output_format = OutputFormat::TABLE;
    ColorTheme color_theme = ColorTheme::PROFESSIONAL;
    bool show_progress = true;
    bool interactive_mode = false;
    int precision = 4;
    bool include_greeks = true;
    bool show_confidence = false;
    std::string log_level = "INFO";
};

/**
 * @brief Command interface for CLI operations
 */
class Command {
public:
    virtual ~Command() = default;
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual std::string usage() const = 0;
    virtual int execute(const std::vector<std::string>& args) = 0;
    virtual std::vector<std::string> get_completions(const std::string& partial) const { return {}; }
};

/**
 * @brief Enhanced CLI manager with rich features
 */
class EnhancedCLI {
private:
    std::map<std::string, std::unique_ptr<Command>> commands_;
    CLIConfig config_;
    bool running_;
    
public:
    EnhancedCLI();
    ~EnhancedCLI();
    
    // Core functionality
    void register_command(std::unique_ptr<Command> command);
    int run(int argc, char* argv[]);
    void start_interactive_mode();
    
    // Configuration
    void set_config(const CLIConfig& config) { config_ = config; }
    CLIConfig get_config() const { return config_; }
    
    // Utility functions
    void print_help() const;
    void print_version() const;
    void print_banner() const;
    
    // Rich output functions
    void print_table(const std::vector<std::vector<std::string>>& data,
                    const std::vector<std::string>& headers) const;
    void print_progress_bar(double percentage, const std::string& label = "") const;
    void print_success(const std::string& message) const;
    void print_error(const std::string& message) const;
    void print_warning(const std::string& message) const;
    void print_info(const std::string& message) const;
    
private:
    std::string colorize(const std::string& text, const std::string& color) const;
    std::vector<std::string> tokenize(const std::string& input) const;
    std::string get_user_input(const std::string& prompt) const;
    std::vector<std::string> get_command_completions(const std::string& partial) const;
};

/**
 * @brief Option pricing command
 */
class PriceCommand : public Command {
public:
    std::string name() const override { return "price"; }
    std::string description() const override { return "Calculate option price and Greeks"; }
    std::string usage() const override;
    int execute(const std::vector<std::string>& args) override;
    std::vector<std::string> get_completions(const std::string& partial) const override;
};

/**
 * @brief Portfolio analysis command
 */
class PortfolioCommand : public Command {
public:
    std::string name() const override { return "portfolio"; }
    std::string description() const override { return "Analyze portfolio risk and performance"; }
    std::string usage() const override;
    int execute(const std::vector<std::string>& args) override;
};

/**
 * @brief Monte Carlo simulation command
 */
class MonteCarloCommand : public Command {
public:
    std::string name() const override { return "montecarlo"; }
    std::string description() const override { return "Run Monte Carlo option pricing simulation"; }
    std::string usage() const override;
    int execute(const std::vector<std::string>& args) override;
};

/**
 * @brief Volatility analysis command
 */
class VolatilityCommand : public Command {
public:
    std::string name() const override { return "volatility"; }
    std::string description() const override { return "Analyze volatility surfaces and implied volatility"; }
    std::string usage() const override;
    int execute(const std::vector<std::string>& args) override;
};

/**
 * @brief Configuration management command
 */
class ConfigCommand : public Command {
public:
    std::string name() const override { return "config"; }
    std::string description() const override { return "Manage CLI configuration settings"; }
    std::string usage() const override;
    int execute(const std::vector<std::string>& args) override;
};

/**
 * @brief Help command
 */
class HelpCommand : public Command {
private:
    const EnhancedCLI* cli_;
    
public:
    explicit HelpCommand(const EnhancedCLI* cli) : cli_(cli) {}
    std::string name() const override { return "help"; }
    std::string description() const override { return "Show help information"; }
    std::string usage() const override;
    int execute(const std::vector<std::string>& args) override;
};

/**
 * @brief Argument parser utility
 */
class ArgumentParser {
private:
    std::map<std::string, std::string> args_;
    std::vector<std::string> positional_args_;
    
public:
    void parse(const std::vector<std::string>& args);
    
    bool has_flag(const std::string& flag) const;
    std::string get_option(const std::string& option, const std::string& default_value = "") const;
    double get_double(const std::string& option, double default_value = 0.0) const;
    int get_int(const std::string& option, int default_value = 0) const;
    
    const std::vector<std::string>& get_positional() const { return positional_args_; }
    
    void print_parsed_args() const;
};

/**
 * @brief Progress tracking utility
 */
class ProgressTracker {
private:
    size_t total_;
    size_t current_;
    std::string label_;
    std::chrono::steady_clock::time_point start_time_;
    
public:
    ProgressTracker(size_t total, const std::string& label = "");
    
    void update(size_t current);
    void increment();
    void finish();
    
    double get_percentage() const;
    std::chrono::duration<double> get_elapsed_time() const;
    std::chrono::duration<double> get_estimated_remaining_time() const;
};

/**
 * @brief Table formatter for rich output
 */
class TableFormatter {
private:
    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> rows_;
    std::vector<size_t> column_widths_;
    
public:
    void set_headers(const std::vector<std::string>& headers);
    void add_row(const std::vector<std::string>& row);
    void clear();
    
    std::string format_table() const;
    void print_table() const;
    
private:
    void calculate_column_widths();
    std::string format_row(const std::vector<std::string>& row, char separator = '|') const;
    std::string format_separator_line() const;
};

} // namespace bsm::ui
