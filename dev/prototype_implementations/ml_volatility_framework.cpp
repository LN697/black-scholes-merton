#include "ml_volatility_framework.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace bsm::ml {

// VolatilitySurface Implementation
VolatilitySurface::VolatilitySurface(const std::vector<VolatilityPoint>& points)
    : points_(points), timestamp_(std::chrono::system_clock::now()) {
}

double VolatilitySurface::interpolate_volatility(double strike, double time_to_expiry) const {
    // Simple bilinear interpolation - production would use more sophisticated methods
    if (points_.empty()) return 0.2; // Default fallback
    
    // Find closest points for interpolation
    double min_distance = std::numeric_limits<double>::max();
    double result_vol = 0.2;
    
    for (const auto& point : points_) {
        double distance = std::sqrt(
            std::pow(point.strike - strike, 2) + 
            std::pow(point.time_to_expiry - time_to_expiry, 2));
        
        if (distance < min_distance) {
            min_distance = distance;
            result_vol = point.implied_vol;
        }
    }
    
    return result_vol;
}

bool VolatilitySurface::is_arbitrage_free() const {
    // Simplified arbitrage check - production needs comprehensive validation
    return true; // Placeholder
}

double VolatilitySurface::surface_smoothness() const {
    if (points_.size() < 2) return 1.0;
    
    double total_variation = 0.0;
    for (size_t i = 1; i < points_.size(); ++i) {
        total_variation += std::abs(points_[i].implied_vol - points_[i-1].implied_vol);
    }
    
    return 1.0 / (1.0 + total_variation);
}

double VolatilitySurface::average_confidence() const {
    if (points_.empty()) return 0.0;
    
    double sum_confidence = 0.0;
    for (const auto& point : points_) {
        sum_confidence += point.confidence;
    }
    
    return sum_confidence / points_.size();
}

// ConstantVolatilityPredictor Implementation
double ConstantVolatilityPredictor::predict_volatility(double strike, double time_to_expiry,
                                                     const MarketFeatures& features) {
    // Constant volatility regardless of inputs
    return constant_vol_;
}

VolatilitySurface ConstantVolatilityPredictor::predict_surface(const MarketFeatures& features,
                                                             const std::vector<double>& strikes,
                                                             const std::vector<double>& expiries) {
    std::vector<VolatilityPoint> points;
    
    for (double strike : strikes) {
        for (double expiry : expiries) {
            VolatilityPoint point;
            point.strike = strike;
            point.time_to_expiry = expiry;
            point.implied_vol = constant_vol_;
            point.confidence = 1.0; // High confidence for constant model
            points.push_back(point);
        }
    }
    
    return VolatilitySurface(points);
}

// SABRVolatilityPredictor Implementation
double SABRVolatilityPredictor::predict_volatility(double strike, double time_to_expiry,
                                                  const MarketFeatures& features) {
    // Use current spot as forward for simplicity
    double forward = features.spot_price;
    return sabr_volatility(forward, strike, time_to_expiry);
}

VolatilitySurface SABRVolatilityPredictor::predict_surface(const MarketFeatures& features,
                                                         const std::vector<double>& strikes,
                                                         const std::vector<double>& expiries) {
    std::vector<VolatilityPoint> points;
    double forward = features.spot_price;
    
    for (double strike : strikes) {
        for (double expiry : expiries) {
            VolatilityPoint point;
            point.strike = strike;
            point.time_to_expiry = expiry;
            point.implied_vol = sabr_volatility(forward, strike, expiry);
            point.confidence = 0.8; // Good confidence for calibrated SABR
            points.push_back(point);
        }
    }
    
    return VolatilitySurface(points);
}

double SABRVolatilityPredictor::sabr_volatility(double forward, double strike, double time_to_expiry) const {
    // Simplified SABR formula implementation
    if (strike <= 0 || forward <= 0 || time_to_expiry <= 0) {
        return alpha_; // Fallback to base volatility
    }
    
    double log_moneyness = std::log(forward / strike);
    double fk_mid = std::pow(forward * strike, (1 - beta_) / 2);
    
    // SABR volatility approximation
    double z = (nu_ / alpha_) * fk_mid * log_moneyness;
    double x = std::log((std::sqrt(1 - 2*rho_*z + z*z) + z - rho_) / (1 - rho_));
    
    double vol = alpha_ / fk_mid * (z / x) * 
                (1 + ((1-beta_)*(1-beta_)/24 * log_moneyness*log_moneyness +
                      rho_*beta_*nu_*alpha_/4/fk_mid +
                      (2-3*rho_*rho_)*nu_*nu_/24) * time_to_expiry);
    
    return std::max(vol, 0.01); // Minimum volatility floor
}

void SABRVolatilityPredictor::calibrate(const std::vector<VolatilityPoint>& market_data) {
    // Simplified calibration - production would use optimization
    if (market_data.empty()) return;
    
    // Simple average as starting point
    double avg_vol = 0.0;
    for (const auto& point : market_data) {
        avg_vol += point.implied_vol;
    }
    alpha_ = avg_vol / market_data.size();
    
    // Set reasonable defaults for other parameters
    beta_ = 0.5;
    rho_ = -0.3;
    nu_ = 0.3;
}

// MLVolatilityPredictor Implementation (Placeholder)
MLVolatilityPredictor::MLVolatilityPredictor(const std::string& model_path)
    : model_path_(model_path), is_loaded_(false) {
    is_loaded_ = load_model(model_path);
}

double MLVolatilityPredictor::predict_volatility(double strike, double time_to_expiry,
                                               const MarketFeatures& features) {
    if (!is_loaded_) {
        // Fallback to heuristic
        return 0.2 + 0.1 * std::abs(std::log(strike / features.spot_price));
    }
    
    // Placeholder for ML prediction
    // In production, this would call the loaded ML model
    double base_vol = 0.15;
    double moneyness_effect = 0.05 * std::abs(std::log(strike / features.spot_price));
    double time_effect = 0.02 * std::sqrt(time_to_expiry);
    double regime_effect = (features.regime == MarketRegime::HIGH_VOLATILITY) ? 0.1 : 0.0;
    
    return base_vol + moneyness_effect + time_effect + regime_effect;
}

VolatilitySurface MLVolatilityPredictor::predict_surface(const MarketFeatures& features,
                                                       const std::vector<double>& strikes,
                                                       const std::vector<double>& expiries) {
    std::vector<VolatilityPoint> points;
    
    for (double strike : strikes) {
        for (double expiry : expiries) {
            VolatilityPoint point;
            point.strike = strike;
            point.time_to_expiry = expiry;
            point.implied_vol = predict_volatility(strike, expiry, features);
            point.confidence = is_loaded_ ? 0.9 : 0.5;
            points.push_back(point);
        }
    }
    
    return VolatilitySurface(points);
}

bool MLVolatilityPredictor::load_model(const std::string& model_path) {
    // Placeholder for model loading
    // In production, this would load the actual ML model
    model_path_ = model_path;
    return true; // Assume success for now
}

void MLVolatilityPredictor::update_model(const std::vector<VolatilityPoint>& new_data) {
    // Placeholder for online learning
    // In production, this would update the ML model with new data
}

MarketRegime MLVolatilityPredictor::classify_regime(const MarketFeatures& features) const {
    // Simple heuristic-based regime classification
    if (features.vix_level > 30) return MarketRegime::HIGH_VOLATILITY;
    if (features.vix_level < 15) return MarketRegime::LOW_VOLATILITY;
    if (std::abs(features.skew_level) > 0.1) return MarketRegime::CRISIS;
    return MarketRegime::MEAN_REVERTING;
}

// EnsembleVolatilityPredictor Implementation
EnsembleVolatilityPredictor::EnsembleVolatilityPredictor() = default;

void EnsembleVolatilityPredictor::add_predictor(std::unique_ptr<VolatilityPredictor> predictor, 
                                              double weight) {
    predictors_.push_back(std::move(predictor));
    weights_.push_back(weight);
}

double EnsembleVolatilityPredictor::predict_volatility(double strike, double time_to_expiry,
                                                     const MarketFeatures& features) {
    if (predictors_.empty()) return 0.2; // Default fallback
    
    double weighted_sum = 0.0;
    double total_weight = 0.0;
    
    for (size_t i = 0; i < predictors_.size(); ++i) {
        double prediction = predictors_[i]->predict_volatility(strike, time_to_expiry, features);
        double weight = (i < weights_.size()) ? weights_[i] : 1.0;
        
        weighted_sum += prediction * weight;
        total_weight += weight;
    }
    
    return (total_weight > 0) ? weighted_sum / total_weight : 0.2;
}

VolatilitySurface EnsembleVolatilityPredictor::predict_surface(const MarketFeatures& features,
                                                             const std::vector<double>& strikes,
                                                             const std::vector<double>& expiries) {
    std::vector<VolatilityPoint> points;
    
    for (double strike : strikes) {
        for (double expiry : expiries) {
            VolatilityPoint point;
            point.strike = strike;
            point.time_to_expiry = expiry;
            point.implied_vol = predict_volatility(strike, expiry, features);
            point.confidence = model_confidence();
            points.push_back(point);
        }
    }
    
    return VolatilitySurface(points);
}

double EnsembleVolatilityPredictor::model_confidence() const {
    if (predictors_.empty()) return 0.5;
    
    double weighted_confidence = 0.0;
    double total_weight = 0.0;
    
    for (size_t i = 0; i < predictors_.size(); ++i) {
        double confidence = predictors_[i]->model_confidence();
        double weight = (i < weights_.size()) ? weights_[i] : 1.0;
        
        weighted_confidence += confidence * weight;
        total_weight += weight;
    }
    
    return (total_weight > 0) ? weighted_confidence / total_weight : 0.5;
}

void EnsembleVolatilityPredictor::optimize_weights(const std::vector<VolatilityPoint>& validation_data) {
    // Simplified weight optimization - production would use sophisticated optimization
    if (predictors_.empty() || validation_data.empty()) return;
    
    // Equal weights as starting point
    weights_.assign(predictors_.size(), 1.0 / predictors_.size());
    
    // TODO: Implement proper weight optimization based on validation performance
}

// VolatilityModelFactory Implementation
std::unique_ptr<VolatilityPredictor> VolatilityModelFactory::create_constant_vol_model(double vol) {
    return std::make_unique<ConstantVolatilityPredictor>(vol);
}

std::unique_ptr<VolatilityPredictor> VolatilityModelFactory::create_sabr_model(
    double alpha, double beta, double rho, double nu) {
    return std::make_unique<SABRVolatilityPredictor>(alpha, beta, rho, nu);
}

std::unique_ptr<VolatilityPredictor> VolatilityModelFactory::create_ml_model(
    const std::string& model_path) {
    return std::make_unique<MLVolatilityPredictor>(model_path);
}

std::unique_ptr<VolatilityPredictor> VolatilityModelFactory::create_ensemble_model(
    const std::vector<std::string>& model_configs) {
    auto ensemble = std::make_unique<EnsembleVolatilityPredictor>();
    
    // Simple ensemble with default models
    ensemble->add_predictor(create_constant_vol_model(0.2), 0.2);
    ensemble->add_predictor(create_sabr_model(0.2, 0.5, -0.3, 0.3), 0.3);
    ensemble->add_predictor(create_ml_model("default_model"), 0.5);
    
    return ensemble;
}

// VolatilitySurfaceValidator Implementation
VolatilitySurfaceValidator::ValidationResult 
VolatilitySurfaceValidator::validate_surface(const VolatilitySurface& surface) {
    ValidationResult result;
    result.is_valid = true;
    result.severity_score = 0.0;
    
    // Basic validation checks
    if (!check_calendar_arbitrage(surface)) {
        result.is_valid = false;
        result.violations.push_back("Calendar arbitrage detected");
        result.severity_score += 0.5;
    }
    
    if (!check_butterfly_arbitrage(surface)) {
        result.is_valid = false;
        result.violations.push_back("Butterfly arbitrage detected");
        result.severity_score += 0.3;
    }
    
    double smoothness = calculate_smoothness_penalty(surface);
    if (smoothness > 0.1) {
        result.violations.push_back("Surface not smooth enough");
        result.severity_score += smoothness;
    }
    
    return result;
}

bool VolatilitySurfaceValidator::check_calendar_arbitrage(const VolatilitySurface& surface) {
    // Simplified calendar arbitrage check
    // Production implementation would be more sophisticated
    return true; // Placeholder
}

bool VolatilitySurfaceValidator::check_butterfly_arbitrage(const VolatilitySurface& surface) {
    // Simplified butterfly arbitrage check
    // Production implementation would be more sophisticated
    return true; // Placeholder
}

double VolatilitySurfaceValidator::calculate_smoothness_penalty(const VolatilitySurface& surface) {
    // Simple smoothness metric based on surface variation
    return 1.0 - surface.surface_smoothness();
}

} // namespace bsm::ml
