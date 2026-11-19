#include "car_audio/dsp/resampler.hpp"

#include <algorithm>
#include <cmath>

namespace car_audio::dsp {

AudioBuffer Resampler::convert_to_engine_format(const AudioBuffer& input, uint32_t target_rate) {
    const size_t in_frames = input.frame_count();
    if (in_frames == 0 || input.channels == 0 || input.sample_rate == 0) {
        return AudioBuffer{target_rate, kEngineChannels, {}};
    }

    const double ratio = static_cast<double>(target_rate) / static_cast<double>(input.sample_rate);
    const size_t out_frames = static_cast<size_t>(std::llround(in_frames * ratio));

    AudioBuffer out;
    out.sample_rate = target_rate;
    out.channels = kEngineChannels;
    out.resize_frames(out_frames);

    for (size_t of = 0; of < out_frames; ++of) {
        double src_pos = static_cast<double>(of) / ratio;
        size_t idx0 = static_cast<size_t>(std::floor(src_pos));
        size_t idx1 = std::min(idx0 + 1, in_frames - 1);
        double frac = src_pos - static_cast<double>(idx0);

        float l0 = input.samples[idx0 * input.channels];
        float l1 = input.samples[idx1 * input.channels];
        float left = static_cast<float>(l0 + frac * (l1 - l0));

        float right = left;
        if (input.channels >= 2) {
            float r0 = input.samples[idx0 * input.channels + 1];
            float r1 = input.samples[idx1 * input.channels + 1];
            right = static_cast<float>(r0 + frac * (r1 - r0));
        }

        out.samples[of * 2] = left;
        out.samples[of * 2 + 1] = right;
    }

    return out;
}

}  // namespace car_audio::dsp
