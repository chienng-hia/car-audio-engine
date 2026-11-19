#pragma once

#include "car_audio/common/types.hpp"

#include <cstddef>
#include <map>
#include <string>

namespace car_audio {

/// Abstract audio output backend (WAV exporter, or future ALSA/PortAudio realtime sink).
class IAudioSink {
public:
    virtual ~IAudioSink() = default;
    virtual void write_bus_frames(const BusId& bus_id, const float* interleaved_stereo,
                                size_t frames) = 0;
    virtual void flush() = 0;
};

/// Records each bus output in memory and writes <output_dir>/<bus_id>.wav on flush().
class WavFileSink : public IAudioSink {
public:
    explicit WavFileSink(std::string output_dir);

    void write_bus_frames(const BusId& bus_id, const float* interleaved_stereo,
                        size_t frames) override;
    void flush() override;

    [[nodiscard]] const std::map<BusId, AudioBuffer>& recorded_buses() const noexcept {
        return bus_buffers_;
    }

private:
    std::string output_dir_;
    std::map<BusId, AudioBuffer> bus_buffers_;
};

}  // namespace car_audio
