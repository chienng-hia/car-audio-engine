#include "car_audio/engine/audio_sink.hpp"
#include "car_audio/common/wav_io.hpp"

#include <filesystem>

namespace car_audio {

WavFileSink::WavFileSink(std::string output_dir) : output_dir_(std::move(output_dir)) {}

void WavFileSink::write_bus_frames(const BusId& bus_id, const float* interleaved_stereo,
                                   size_t frames) {
    auto& buf = bus_buffers_[bus_id];
    buf.sample_rate = kEngineSampleRate;
    buf.channels = kEngineChannels;
    buf.samples.insert(buf.samples.end(), interleaved_stereo,
                       interleaved_stereo + frames * kEngineChannels);
}

void WavFileSink::flush() {
    if (output_dir_.empty()) return;
    std::filesystem::create_directories(output_dir_);
    for (const auto& [bus_id, buf] : bus_buffers_) {
        std::filesystem::path p = std::filesystem::path(output_dir_) / (bus_id + ".wav");
        write_wav_pcm16(p.string(), buf);
    }
}

}  // namespace car_audio
