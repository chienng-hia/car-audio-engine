#include "car_audio/dsp/limiter.hpp"

#include <algorithm>
#include <cmath>

namespace car_audio::dsp {

Limiter::Limiter(float threshold, double attack_ms, double release_ms, uint32_t sample_rate)
    : threshold_(std::clamp(threshold, 0.1f, 1.0f)) {
    const double fs = static_cast<double>(sample_rate);
    attack_coeff_ = static_cast<float>(std::exp(-1.0 / std::max(1.0, (attack_ms / 1000.0) * fs)));
    release_coeff_ = static_cast<float>(std::exp(-1.0 / std::max(1.0, (release_ms / 1000.0) * fs)));
}

void Limiter::set_threshold(float threshold) {
    threshold_ = std::clamp(threshold, 0.1f, 1.0f);
}

void Limiter::reset() {
    envelope_ = 0.0f;
    last_gr_db_ = 0.0f;
}

void Limiter::process(float* interleaved_stereo, size_t frames) {
    float min_gain = 1.0f;
    for (size_t f = 0; f < frames; ++f) {
        float l = interleaved_stereo[f * 2];
        float r = interleaved_stereo[f * 2 + 1];
        float peak = std::max(std::abs(l), std::abs(r));

        if (peak > envelope_) {
            envelope_ = attack_coeff_ * envelope_ + (1.0f - attack_coeff_) * peak;
        } else {
            envelope_ = release_coeff_ * envelope_ + (1.0f - release_coeff_) * peak;
        }

        float gain = 1.0f;
        if (envelope_ > threshold_) {
            gain = threshold_ / envelope_;
        }
        min_gain = std::min(min_gain, gain);

        interleaved_stereo[f * 2] = std::clamp(l * gain, -1.0f, 1.0f);
        interleaved_stereo[f * 2 + 1] = std::clamp(r * gain, -1.0f, 1.0f);
    }
    last_gr_db_ = (min_gain < 1.0f) ? (20.0f * std::log10(min_gain)) : 0.0f;
}

}  // namespace car_audio::dsp
