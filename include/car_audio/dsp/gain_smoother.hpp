#pragma once

#include <cstddef>
#include <cstdint>

namespace car_audio::dsp {

/// Click-free linear/exponential gain ramper for ducking, muting, and volume steps.
class GainSmoother {
public:
    explicit GainSmoother(float initial_gain = 1.0f, double ramp_ms = 20.0,
                          uint32_t sample_rate = 48000);

    void set_target(float target_gain);
    void set_immediate(float gain);
    void set_ramp_time_ms(double ramp_ms);

    /// Apply smoothed gain in-place to interleaved stereo samples.
    void process(float* interleaved_stereo, size_t frames);

    [[nodiscard]] float current() const noexcept { return current_; }
    [[nodiscard]] float target() const noexcept { return target_; }

private:
    uint32_t sample_rate_;
    double ramp_ms_;
    float current_;
    float target_;
    float step_ = 0.0f;
    size_t remaining_steps_ = 0;
};

}  // namespace car_audio::dsp
