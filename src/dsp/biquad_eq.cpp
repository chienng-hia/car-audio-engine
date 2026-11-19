#include "car_audio/dsp/biquad_eq.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace car_audio::dsp {
namespace {
constexpr double kPi = 3.14159265358979323846;
}

void BiquadFilter::configure(const EqBandConfig& cfg, uint32_t sample_rate) {
    const double fs = static_cast<double>(sample_rate);
    const double f0 = std::clamp(cfg.freq_hz, 10.0, fs * 0.45);
    const double q = std::max(cfg.q, 0.1);
    const double A = std::pow(10.0, cfg.gain_db / 40.0);
    const double w0 = 2.0 * kPi * f0 / fs;
    const double cos_w0 = std::cos(w0);
    const double sin_w0 = std::sin(w0);
    const double alpha = sin_w0 / (2.0 * q);

    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a0 = 1.0, a1 = 0.0, a2 = 0.0;

    switch (cfg.type) {
        case FilterType::Peaking: {
            b0 = 1.0 + alpha * A;
            b1 = -2.0 * cos_w0;
            b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1 = -2.0 * cos_w0;
            a2 = 1.0 - alpha / A;
            break;
        }
        case FilterType::LowShelf: {
            const double two_sqrt_a_alpha = 2.0 * std::sqrt(A) * alpha;
            b0 = A * ((A + 1.0) - (A - 1.0) * cos_w0 + two_sqrt_a_alpha);
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0);
            b2 = A * ((A + 1.0) - (A - 1.0) * cos_w0 - two_sqrt_a_alpha);
            a0 = (A + 1.0) + (A - 1.0) * cos_w0 + two_sqrt_a_alpha;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_w0);
            a2 = (A + 1.0) + (A - 1.0) * cos_w0 - two_sqrt_a_alpha;
            break;
        }
        case FilterType::HighShelf: {
            const double two_sqrt_a_alpha = 2.0 * std::sqrt(A) * alpha;
            b0 = A * ((A + 1.0) + (A - 1.0) * cos_w0 + two_sqrt_a_alpha);
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w0);
            b2 = A * ((A + 1.0) + (A - 1.0) * cos_w0 - two_sqrt_a_alpha);
            a0 = (A + 1.0) - (A - 1.0) * cos_w0 + two_sqrt_a_alpha;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_w0);
            a2 = (A + 1.0) - (A - 1.0) * cos_w0 - two_sqrt_a_alpha;
            break;
        }
    }

    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;
    a1_ = a1 / a0;
    a2_ = a2 / a0;
}

void BiquadFilter::reset() {
    state_[0] = ChannelState{};
    state_[1] = ChannelState{};
}

void BiquadFilter::process(float* interleaved_stereo, size_t frames) {
    for (size_t f = 0; f < frames; ++f) {
        for (size_t ch = 0; ch < 2; ++ch) {
            double x = static_cast<double>(interleaved_stereo[f * 2 + ch]);
            auto& s = state_[ch];
            double y = b0_ * x + b1_ * s.x1 + b2_ * s.x2 - a1_ * s.y1 - a2_ * s.y2;
            s.x2 = s.x1;
            s.x1 = x;
            s.y2 = s.y1;
            s.y1 = y;
            interleaved_stereo[f * 2 + ch] = static_cast<float>(y);
        }
    }
}

FiveBandEq::FiveBandEq(uint32_t sample_rate) : sample_rate_(sample_rate) {
    apply_preset("Flat");
}

void FiveBandEq::set_band(size_t band_idx, const EqBandConfig& cfg) {
    if (band_idx >= kNumBands) {
        throw std::out_of_range("EQ band index out of range");
    }
    configs_[band_idx] = cfg;
    filters_[band_idx].configure(cfg, sample_rate_);
}

void FiveBandEq::apply_preset(const std::string& preset_name) {
    std::array<EqBandConfig, kNumBands> preset = {{
        {FilterType::LowShelf, 60.0, 0.707, 0.0},
        {FilterType::Peaking, 250.0, 1.0, 0.0},
        {FilterType::Peaking, 1000.0, 1.0, 0.0},
        {FilterType::Peaking, 4000.0, 1.0, 0.0},
        {FilterType::HighShelf, 12000.0, 0.707, 0.0},
    }};

    if (preset_name == "BassBoost") {
        preset[0].gain_db = 6.0;
        preset[1].gain_db = 3.0;
    } else if (preset_name == "Vocal") {
        preset[0].gain_db = -2.0;
        preset[2].gain_db = 3.5;
        preset[3].gain_db = 4.0;
    } else if (preset_name == "Rock") {
        preset[0].gain_db = 4.5;
        preset[1].gain_db = 1.5;
        preset[2].gain_db = -1.5;
        preset[3].gain_db = 3.0;
        preset[4].gain_db = 4.0;
    }

    for (size_t i = 0; i < kNumBands; ++i) {
        set_band(i, preset[i]);
    }
}

void FiveBandEq::reset() {
    for (auto& f : filters_) {
        f.reset();
    }
}

void FiveBandEq::process(float* interleaved_stereo, size_t frames) {
    for (auto& f : filters_) {
        f.process(interleaved_stereo, frames);
    }
}

}  // namespace car_audio::dsp
