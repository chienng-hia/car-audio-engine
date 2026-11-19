#pragma once

#include <cstddef>
#include <cstdint>

namespace car_audio::dsp {

/// Stereo-linked soft-knee peak limiter to prevent digital clipping on bus mixdown.
class Limiter {
public:
    explicit Limiter(float threshold = 0.95f, double attack_ms = 1.0, double release_ms = 50.0,
                     uint32_t sample_rate = 48000);

    void set_threshold(float threshold);
    void reset();
    void process(float* interleaved_stereo, size_t frames);

    [[nodiscard]] float last_gain_reduction_db() const noexcept { return last_gr_db_; }

private:
    float threshold_;
    float attack_coeff_;
    float release_coeff_;
    float envelope_ = 0.0f;
    float last_gr_db_ = 0.0f;
};

}  // namespace car_audio::dsp
