#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include "option_types.hpp"
#include "stats.hpp"

namespace bsm::ml {

/**
 * @brief Market regime classification for volatility modeling
 */
enum class MarketRegime {
    LOW_VOLATILITY,
    HIGH_VOLATILITY, 
    TRENDING,
    MEAN_REVERTING,
    CRISIS
};

/**
 * @brief Market features for ML model input
 */
struct MarketFeatures {
    double spot_price;
    double realized_vol_1d;
    double realized_vol_5d;
    double realized_vol_21d;
    double vix_level;
    double term_structure_slope;
    double skew_level;
    double volume_ratio;
    double time_of_day;
    int day_of_week;
    MarketRegime regime;
};

/**
 * @brief Volatility surface point
 */
struct VolatilityPoint {
    double strike;
    double time_to_expiry;
    double implied_vol;
    double confidence;
};

/**
 * @brief Complete volatility surface
 */
class VolatilitySurface {
private:
    std::vector<VolatilityPoint> points_;
    std::chrono::system_clock::time_point timestamp_;
    
public:
    VolatilitySurface(const std::vector<VolatilityPoint>& points);
    
    double interpolate_volatility(double strike, double time_to_expiry) const;
    std::vector<VolatilityPoint> get_points() const { return points_; }
    
    // Surface quality metrics
    bool is_arbitrage_free() const;
    double surface_smoothness() const;
    double average_confidence() const;
};

/**
 * @brief Abstract base class for volatility predictors
 */
class VolatilityPredictor {
public:
    virtual ~VolatilityPredictor() = default;
    
    virtual double predict_volatility(double strike, double time_to_expiry,
                                    const MarketFeatures& features) = 0;
    
    virtual VolatilitySurface predict_surface(const MarketFeatures& features,
                                            const std::vector<double>& strikes,
                                            const std::vector<double>& expiries) = 0;
    
    virtual std::string model_name() const = 0;
    virtual double model_confidence() const = 0;
};

/**
 * @brief Traditional Black-Scholes constant volatility predictor (baseline)
 */
class ConstantVolatilityPredictor : public VolatilityPredictor {
private:
    double constant_vol_;
    
public:
    explicit ConstantVolatilityPredictor(double vol) : constant_vol_(vol) {}
    
    double predict_volatility(double strike, double time_to_expiry,
                            const MarketFeatures& features) override;
    
    VolatilitySurface predict_surface(const MarketFeatures& features,
                                    const std::vector<double>& strikes,
                                    const std::vector<double>& expiries) override;
    
    std::string model_name() const override { return "ConstantVolatility"; }
    double model_confidence() const override { return 0.5; }
};

/**
 * @brief SABR model-based volatility predictor
 */
class SABRVolatilityPredictor : public VolatilityPredictor {
private:
    double alpha_;    // Vol of vol
    double beta_;     // CEV exponent
    double rho_;      // Correlation
    double nu_;       // Vol of vol
    
public:
    SABRVolatilityPredictor(double alpha, double beta, double rho, double nu)
        : alpha_(alpha), beta_(beta), rho_(rho), nu_(nu) {}
    
    double predict_volatility(double strike, double time_to_expiry,
                            const MarketFeatures& features) override;
    
    VolatilitySurface predict_surface(const MarketFeatures& features,
                                    const std::vector<double>& strikes,
                                    const std::vector<double>& expiries) override;
    
    std::string model_name() const override { return "SABR"; }
    double model_confidence() const override { return 0.75; }
    
    // SABR-specific methods
    void calibrate(const std::vector<VolatilityPoint>& market_data);
    double sabr_volatility(double forward, double strike, double time_to_expiry) const;
};

/**
 * @brief ML-enhanced volatility predictor (placeholder for future ML integration)
 */
class MLVolatilityPredictor : public VolatilityPredictor {
private:
    std::string model_path_;
    bool is_loaded_;
    
public:
    explicit MLVolatilityPredictor(const std::string& model_path);
    
    double predict_volatility(double strike, double time_to_expiry,
                            const MarketFeatures& features) override;
    
    VolatilitySurface predict_surface(const MarketFeatures& features,
                                    const std::vector<double>& strikes,
                                    const std::vector<double>& expiries) override;
    
    std::string model_name() const override { return "MLEnhanced"; }
    double model_confidence() const override { return 0.9; }
    
    // ML-specific methods
    bool load_model(const std::string& model_path);
    void update_model(const std::vector<VolatilityPoint>& new_data);
    MarketRegime classify_regime(const MarketFeatures& features) const;
};

/**
 * @brief Ensemble volatility predictor combining multiple models
 */
class EnsembleVolatilityPredictor : public VolatilityPredictor {
private:
    std::vector<std::unique_ptr<VolatilityPredictor>> predictors_;
    std::vector<double> weights_;
    
public:
    EnsembleVolatilityPredictor();
    
    void add_predictor(std::unique_ptr<VolatilityPredictor> predictor, double weight);
    
    double predict_volatility(double strike, double time_to_expiry,
                            const MarketFeatures& features) override;
    
    VolatilitySurface predict_surface(const MarketFeatures& features,
                                    const std::vector<double>& strikes,
                                    const std::vector<double>& expiries) override;
    
    std::string model_name() const override { return "Ensemble"; }
    double model_confidence() const override;
    
    // Ensemble-specific methods
    void optimize_weights(const std::vector<VolatilityPoint>& validation_data);
    std::vector<double> get_predictor_weights() const { return weights_; }
};

/**
 * @brief Volatility model factory
 */
class VolatilityModelFactory {
public:
    static std::unique_ptr<VolatilityPredictor> create_constant_vol_model(double vol);
    static std::unique_ptr<VolatilityPredictor> create_sabr_model(
        double alpha, double beta, double rho, double nu);
    static std::unique_ptr<VolatilityPredictor> create_ml_model(
        const std::string& model_path);
    static std::unique_ptr<VolatilityPredictor> create_ensemble_model(
        const std::vector<std::string>& model_configs);
};

/**
 * @brief Volatility surface arbitrage checker
 */
class VolatilitySurfaceValidator {
public:
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> violations;
        double severity_score;
    };
    
    static ValidationResult validate_surface(const VolatilitySurface& surface);
    static bool check_calendar_arbitrage(const VolatilitySurface& surface);
    static bool check_butterfly_arbitrage(const VolatilitySurface& surface);
    static double calculate_smoothness_penalty(const VolatilitySurface& surface);
};

} // namespace bsm::ml
