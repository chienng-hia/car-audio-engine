#include "car_audio/dsp/spatial_balance.hpp"

#include <algorithm>

namespace car_audio::dsp {

void SpatialBalance::set_fade(float fade) {
    fade_ = std::clamp(fade, -1.0f, 1.0f);
}

void SpatialBalance::set_balance(float balance) {
    balance_ = std::clamp(balance, -1.0f, 1.0f);
}

std::pair<float, float> SpatialBalance::zone_gains(AudioZoneId zone) const noexcept {
    // Front/Rear attenuation
    float zone_fade_gain = 1.0f;
    if (zone == AudioZoneId::Driver || zone == AudioZoneId::Passenger) {
        // Front zones attenuate when fade < 0 (towards rear)
        zone_fade_gain = (fade_ < 0.0f) ? (1.0f + fade_) : 1.0f;
    } else if (zone == AudioZoneId::RearLeft || zone == AudioZoneId::RearRight) {
        // Rear zones attenuate when fade > 0 (towards front)
        zone_fade_gain = (fade_ > 0.0f) ? (1.0f - fade_) : 1.0f;
    }

    // Left/Right attenuation
    float left_gain = (balance_ > 0.0f) ? (1.0f - balance_) : 1.0f;
    float right_gain = (balance_ < 0.0f) ? (1.0f + balance_) : 1.0f;

    // For single-seat zones, also reflect seat lateral position if desired,
    // while preserving stereo L/R balance inside the zone.
    return {zone_fade_gain * left_gain, zone_fade_gain * right_gain};
}

void SpatialBalance::process(AudioZoneId zone, float* interleaved_stereo, size_t frames) const {
    auto [lg, rg] = zone_gains(zone);
    if (lg == 1.0f && rg == 1.0f) return;
    for (size_t f = 0; f < frames; ++f) {
        interleaved_stereo[f * 2] *= lg;
        interleaved_stereo[f * 2 + 1] *= rg;
    }
}

}  // namespace car_audio::dsp
