#include "car_audio/dsp/sdvc.hpp"

#include <algorithm>
#include <cmath>

namespace car_audio::dsp {

Sdvc::Sdvc(SdvcConfig cfg) : cfg_(cfg) {}

void Sdvc::configure(const SdvcConfig& cfg) {
    cfg_ = cfg;
}

void Sdvc::set_speed_kmh(float speed_kmh) {
    speed_kmh_ = std::max(0.0f, speed_kmh);
}

float Sdvc::boost_db() const noexcept {
    if (!cfg_.enabled || speed_kmh_ <= cfg_.min_speed_kmh) {
        return 0.0f;
    }
    if (cfg_.max_speed_kmh <= cfg_.min_speed_kmh) {
        return cfg_.max_boost_db;
    }
    float t = std::clamp(
        (speed_kmh_ - cfg_.min_speed_kmh) / (cfg_.max_speed_kmh - cfg_.min_speed_kmh),
        0.0f, 1.0f);
    // Smooth S-curve (Hermite) for natural-sounding volume compensation
    float smooth_t = t * t * (3.0f - 2.0f * t);
    return smooth_t * cfg_.max_boost_db;
}

float Sdvc::linear_gain() const noexcept {
    return std::pow(10.0f, boost_db() / 20.0f);
}

}  // namespace car_audio::dsp
