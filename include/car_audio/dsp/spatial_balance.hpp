#pragma once

#include "car_audio/common/types.hpp"

#include <utility>

namespace car_audio::dsp {

/// Computes per-zone and left/right channel gains from cabin Fade and Balance.
/// fade:    -1.0 (Full Rear) to +1.0 (Full Front), 0.0 = center
/// balance: -1.0 (Full Left) to +1.0 (Full Right), 0.0 = center
class SpatialBalance {
public:
    void set_fade(float fade);
    void set_balance(float balance);

    [[nodiscard]] float fade() const noexcept { return fade_; }
    [[nodiscard]] float balance() const noexcept { return balance_; }

    /// Returns {left_gain, right_gain} for a given zone.
    [[nodiscard]] std::pair<float, float> zone_gains(AudioZoneId zone) const noexcept;

    /// Apply spatial balance in-place to interleaved stereo buffer for a zone.
    void process(AudioZoneId zone, float* interleaved_stereo, size_t frames) const;

private:
    float fade_ = 0.0f;
    float balance_ = 0.0f;
};

}  // namespace car_audio::dsp
