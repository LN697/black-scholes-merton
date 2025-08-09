#include "ui/cli/enhanced_cli.hpp"
#include <iostream>

/**
 * @brief Enhanced main function with rich CLI interface
 * 
 * This new main function provides a professional CLI experience while
 * maintaining backward compatibility with the existing functionality.
 * 
 * Features:
 * - Rich, colored output with progress bars
 * - Interactive mode for exploratory analysis
 * - Table-formatted results
 * - Comprehensive help system
 * - Tab completion support
 * - Configuration management
 */

int main(int argc, char* argv[]) {
    try {
        bsm::ui::EnhancedCLI cli;
        
        // Configure CLI for professional appearance
        bsm::ui::CLIConfig config;
        config.output_format = bsm::ui::OutputFormat::TABLE;
        config.color_theme = bsm::ui::ColorTheme::PROFESSIONAL;
        config.show_progress = true;
        config.include_greeks = true;
        config.precision = 4;
        
        cli.set_config(config);
        
        return cli.run(argc, argv);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}

/**
 * Usage Examples:
 * 
 * Basic option pricing:
 * ./bsm price --spot 100 --strike 105 --rate 0.05 --time 0.25 --vol 0.2 --type call
 * 
 * With Greeks calculation:
 * ./bsm price -S 100 -K 105 -r 0.05 -T 0.25 -v 0.2 -t call --greeks
 * 
 * JSON output:
 * ./bsm price --spot 100 --strike 105 --rate 0.05 --time 0.25 --vol 0.2 --type call --format json
 * 
 * Interactive mode:
 * ./bsm --interactive
 * 
 * Help:
 * ./bsm help
 * ./bsm price --help
 */
