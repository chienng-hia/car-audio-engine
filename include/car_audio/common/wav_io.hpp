#pragma once

#include "car_audio/common/types.hpp"

#include <string>

namespace car_audio {

/// Read a PCM16 or IEEE Float32 WAV file into an AudioBuffer.
AudioBuffer read_wav(const std::string& path);

/// Write an AudioBuffer as a 16-bit PCM WAV file (samples clamped to [-1.0, 1.0]).
void write_wav_pcm16(const std::string& path, const AudioBuffer& buffer);

/// Synthetic signal generators for offline testing and simulation.
namespace synth {

AudioBuffer generate_sine(double freq_hz, double duration_sec, float amplitude = 0.5f,
                          uint32_t sample_rate = kEngineSampleRate, uint16_t channels = 2);

AudioBuffer generate_chime(double freq1_hz, double freq2_hz, double duration_sec,
                           float amplitude = 0.6f, uint32_t sample_rate = kEngineSampleRate,
                           uint16_t channels = 2);

AudioBuffer generate_adas_beep(double freq_hz, double duration_sec, double pulse_hz = 4.0,
                               float amplitude = 0.7f, uint32_t sample_rate = kEngineSampleRate,
                               uint16_t channels = 2);

}  // namespace synth

}  // namespace car_audio
