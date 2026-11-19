#include "car_audio/dsp/gain_smoother.hpp"

#include <algorithm>
#include <cmath>

namespace car_audio::dsp {

GainSmoother::GainSmoother(float initial_gain, double ramp_ms, uint32_t sample_rate)
    : sample_rate_(sample_rate),
      ramp_ms_(std::max(0.0, ramp_ms)),
      current_(initial_gain),
      target_(initial_gain) {}

void GainSmoother::set_target(float target_gain) {
    target_ = std::max(0.0f, target_gain);
    if (std::abs(target_ - current_) < 1e-6f || ramp_ms_ <= 0.0) {
        current_ = target_;
        remaining_steps_ = 0;
        step_ = 0.0f;
        return;
    }
    remaining_steps_ = std::max<size_t>(
        1, static_cast<size_t>(std::llround((ramp_ms_ / 1000.0) * sample_rate_)));
    step_ = (target_ - current_) / static_cast<float>(remaining_steps_);
}

void GainSmoother::set_immediate(float gain) {
    current_ = std::max(0.0f, gain);
    target_ = current_;
    remaining_steps_ = 0;
    step_ = 0.0f;
}

void GainSmoother::set_ramp_time_ms(double ramp_ms) {
    ramp_ms_ = std::max(0.0, ramp_ms);
}

void GainSmoother::process(float* interleaved_stereo, size_t frames) {
    for (size_t f = 0; f < frames; ++f) {
        if (remaining_steps_ > 0) {
            current_ += step_;
            --remaining_steps_;
            if (remaining_steps_ == 0) {
                current_ = target_;
            }
        }
        interleaved_stereo[f * 2] *= current_;
        interleaved_stereo[f * 2 + 1] *= current_;
    }
}

}  // namespace car_audio::dsp
