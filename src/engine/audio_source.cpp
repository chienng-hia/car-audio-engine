#include "car_audio/engine/audio_source.hpp"
#include "car_audio/dsp/resampler.hpp"

#include <algorithm>
#include <cstring>

namespace car_audio {

BufferAudioSource::BufferAudioSource(const AudioBuffer& raw_buffer, bool loop)
    : buffer_(dsp::Resampler::convert_to_engine_format(raw_buffer, kEngineSampleRate)),
      loop_(loop) {
    if (buffer_.frame_count() == 0) {
        finished_ = true;
    }
}

size_t BufferAudioSource::read_frames(float* out_interleaved_stereo, size_t frames) {
    if (finished_ || buffer_.frame_count() == 0) {
        std::fill_n(out_interleaved_stereo, frames * kEngineChannels, 0.0f);
        return 0;
    }

    const size_t total = buffer_.frame_count();
    size_t written = 0;

    while (written < frames) {
        size_t avail = total - cursor_frame_;
        if (avail == 0) {
            if (loop_) {
                cursor_frame_ = 0;
                avail = total;
            } else {
                finished_ = true;
                break;
            }
        }
        size_t to_copy = std::min(frames - written, avail);
        std::memcpy(out_interleaved_stereo + written * kEngineChannels,
                    buffer_.samples.data() + cursor_frame_ * kEngineChannels,
                    to_copy * kEngineChannels * sizeof(float));
        cursor_frame_ += to_copy;
        written += to_copy;
    }

    if (written < frames) {
        std::fill_n(out_interleaved_stereo + written * kEngineChannels,
                    (frames - written) * kEngineChannels, 0.0f);
    }

    return written;
}

}  // namespace car_audio
