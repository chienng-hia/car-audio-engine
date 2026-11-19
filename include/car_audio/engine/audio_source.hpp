#pragma once

#include "car_audio/common/types.hpp"

#include <cstddef>
#include <memory>

namespace car_audio {

/// Abstract audio input source producing 48kHz stereo float32 frames.
class IAudioSource {
public:
    virtual ~IAudioSource() = default;
    virtual size_t read_frames(float* out_interleaved_stereo, size_t frames) = 0;
    [[nodiscard]] virtual bool is_finished() const = 0;
};

/// In-memory AudioBuffer source (auto-resampled to engine 48kHz stereo).
class BufferAudioSource : public IAudioSource {
public:
    explicit BufferAudioSource(const AudioBuffer& raw_buffer, bool loop = false);

    size_t read_frames(float* out_interleaved_stereo, size_t frames) override;
    [[nodiscard]] bool is_finished() const override { return finished_; }

private:
    AudioBuffer buffer_;
    bool loop_;
    size_t cursor_frame_ = 0;
    bool finished_ = false;
};

}  // namespace car_audio
