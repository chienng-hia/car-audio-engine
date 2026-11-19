#pragma once

#include "car_audio/common/types.hpp"

#include <cstdint>

namespace car_audio::dsp {

/// Converts an AudioBuffer from arbitrary sample rate & mono/stereo to 48kHz stereo.
class Resampler {
public:
    static AudioBuffer convert_to_engine_format(const AudioBuffer& input,
                                                uint32_t target_rate = kEngineSampleRate);
};

}  // namespace car_audio::dsp
