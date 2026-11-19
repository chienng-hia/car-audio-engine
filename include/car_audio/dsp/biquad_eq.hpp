#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace car_audio::dsp {

enum class FilterType {
    LowShelf,
    Peaking,
    HighShelf
};

struct EqBandConfig {
    FilterType type = FilterType::Peaking;
    double freq_hz = 1000.0;
    double q = 1.0;
    double gain_db = 0.0;
};

/// Stereo Biquad IIR filter (Direct Form I).
class BiquadFilter {
public:
    void configure(const EqBandConfig& cfg, uint32_t sample_rate);
    void reset();
    void process(float* interleaved_stereo, size_t frames);

private:
    double b0_ = 1.0, b1_ = 0.0, b2_ = 0.0;
    double a1_ = 0.0, a2_ = 0.0;

    struct ChannelState {
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
    };
    std::array<ChannelState, 2> state_{};
};

/// 5-Band Parametric Equalizer for automotive cabin tuning.
class FiveBandEq {
public:
    static constexpr size_t kNumBands = 5;

    explicit FiveBandEq(uint32_t sample_rate = 48000);

    void set_band(size_t band_idx, const EqBandConfig& cfg);
    void apply_preset(const std::string& preset_name);
    void reset();
    void process(float* interleaved_stereo, size_t frames);

    [[nodiscard]] const std::array<EqBandConfig, kNumBands>& bands() const { return configs_; }

private:
    uint32_t sample_rate_;
    std::array<EqBandConfig, kNumBands> configs_{};
    std::array<BiquadFilter, kNumBands> filters_{};
};

}  // namespace car_audio::dsp
