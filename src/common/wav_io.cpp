#include "car_audio/common/wav_io.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace car_audio {
namespace {

constexpr double kPi = 3.14159265358979323846;

template <typename T>
T read_le(std::istream& is) {
    T val{};
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
    return val;
}

template <typename T>
void write_le(std::ostream& os, T val) {
    os.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

}  // namespace

AudioBuffer read_wav(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open WAV file: " + path);
    }

    char riff[4]{};
    ifs.read(riff, 4);
    if (std::memcmp(riff, "RIFF", 4) != 0) {
        throw std::runtime_error("Invalid WAV header (missing RIFF): " + path);
    }
    read_le<uint32_t>(ifs);  // file size
    char wave[4]{};
    ifs.read(wave, 4);
    if (std::memcmp(wave, "WAVE", 4) != 0) {
        throw std::runtime_error("Invalid WAV header (missing WAVE): " + path);
    }

    uint16_t audio_format = 0;
    uint16_t channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
    bool found_fmt = false;
    bool found_data = false;
    std::vector<uint8_t> raw_data;

    while (ifs && !(found_fmt && found_data)) {
        char chunk_id[4]{};
        ifs.read(chunk_id, 4);
        if (ifs.gcount() < 4) break;
        uint32_t chunk_size = read_le<uint32_t>(ifs);

        if (std::memcmp(chunk_id, "fmt ", 4) == 0) {
            audio_format = read_le<uint16_t>(ifs);
            channels = read_le<uint16_t>(ifs);
            sample_rate = read_le<uint32_t>(ifs);
            read_le<uint32_t>(ifs);  // byte rate
            read_le<uint16_t>(ifs);  // block align
            bits_per_sample = read_le<uint16_t>(ifs);
            if (chunk_size > 16) {
                ifs.seekg(chunk_size - 16, std::ios::cur);
            }
            found_fmt = true;
        } else if (std::memcmp(chunk_id, "data", 4) == 0) {
            raw_data.resize(chunk_size);
            ifs.read(reinterpret_cast<char*>(raw_data.data()), chunk_size);
            found_data = true;
        } else {
            ifs.seekg(chunk_size + (chunk_size % 2), std::ios::cur);
        }
    }

    if (!found_fmt || !found_data) {
        throw std::runtime_error("Incomplete WAV file (missing fmt or data): " + path);
    }

    AudioBuffer buf;
    buf.sample_rate = sample_rate;
    buf.channels = channels;

    if (audio_format == 1 && bits_per_sample == 16) {
        size_t total_samples = raw_data.size() / 2;
        buf.samples.resize(total_samples);
        const int16_t* ptr = reinterpret_cast<const int16_t*>(raw_data.data());
        for (size_t i = 0; i < total_samples; ++i) {
            buf.samples[i] = static_cast<float>(ptr[i]) / 32768.0f;
        }
    } else if (audio_format == 3 && bits_per_sample == 32) {
        size_t total_samples = raw_data.size() / 4;
        buf.samples.resize(total_samples);
        std::memcpy(buf.samples.data(), raw_data.data(), total_samples * sizeof(float));
    } else {
        throw std::runtime_error("Unsupported WAV format (only PCM16 and Float32 supported)");
    }

    return buf;
}

void write_wav_pcm16(const std::string& path, const AudioBuffer& buffer) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Failed to open WAV file for writing: " + path);
    }

    const uint32_t data_bytes = static_cast<uint32_t>(buffer.samples.size() * sizeof(int16_t));
    const uint32_t riff_size = 36 + data_bytes;
    const uint16_t bits_per_sample = 16;
    const uint16_t block_align = buffer.channels * (bits_per_sample / 8);
    const uint32_t byte_rate = buffer.sample_rate * block_align;

    ofs.write("RIFF", 4);
    write_le<uint32_t>(ofs, riff_size);
    ofs.write("WAVE", 4);

    ofs.write("fmt ", 4);
    write_le<uint32_t>(ofs, 16);
    write_le<uint16_t>(ofs, 1);  // PCM
    write_le<uint16_t>(ofs, buffer.channels);
    write_le<uint32_t>(ofs, buffer.sample_rate);
    write_le<uint32_t>(ofs, byte_rate);
    write_le<uint16_t>(ofs, block_align);
    write_le<uint16_t>(ofs, bits_per_sample);

    ofs.write("data", 4);
    write_le<uint32_t>(ofs, data_bytes);

    std::vector<int16_t> pcm(buffer.samples.size());
    for (size_t i = 0; i < buffer.samples.size(); ++i) {
        float clamped = std::clamp(buffer.samples[i], -1.0f, 1.0f);
        pcm[i] = static_cast<int16_t>(std::lround(clamped * 32767.0f));
    }
    ofs.write(reinterpret_cast<const char*>(pcm.data()), data_bytes);
}

namespace synth {

AudioBuffer generate_sine(double freq_hz, double duration_sec, float amplitude,
                          uint32_t sample_rate, uint16_t channels) {
    size_t frames = static_cast<size_t>(std::llround(duration_sec * sample_rate));
    AudioBuffer buf;
    buf.sample_rate = sample_rate;
    buf.channels = channels;
    buf.resize_frames(frames);

    for (size_t f = 0; f < frames; ++f) {
        double t = static_cast<double>(f) / sample_rate;
        float val = amplitude * static_cast<float>(std::sin(2.0 * kPi * freq_hz * t));
        for (uint16_t c = 0; c < channels; ++c) {
            buf.samples[f * channels + c] = val;
        }
    }
    return buf;
}

AudioBuffer generate_chime(double freq1_hz, double freq2_hz, double duration_sec,
                           float amplitude, uint32_t sample_rate, uint16_t channels) {
    size_t frames = static_cast<size_t>(std::llround(duration_sec * sample_rate));
    AudioBuffer buf;
    buf.sample_rate = sample_rate;
    buf.channels = channels;
    buf.resize_frames(frames);

    for (size_t f = 0; f < frames; ++f) {
        double t = static_cast<double>(f) / sample_rate;
        double env = std::exp(-3.5 * t / std::max(duration_sec, 0.01));
        double s = 0.5 * (std::sin(2.0 * kPi * freq1_hz * t) + std::sin(2.0 * kPi * freq2_hz * t));
        float val = static_cast<float>(amplitude * env * s);
        for (uint16_t c = 0; c < channels; ++c) {
            buf.samples[f * channels + c] = val;
        }
    }
    return buf;
}

AudioBuffer generate_adas_beep(double freq_hz, double duration_sec, double pulse_hz,
                               float amplitude, uint32_t sample_rate, uint16_t channels) {
    size_t frames = static_cast<size_t>(std::llround(duration_sec * sample_rate));
    AudioBuffer buf;
    buf.sample_rate = sample_rate;
    buf.channels = channels;
    buf.resize_frames(frames);

    for (size_t f = 0; f < frames; ++f) {
        double t = static_cast<double>(f) / sample_rate;
        double gate = std::sin(2.0 * kPi * pulse_hz * t) >= 0.0 ? 1.0 : 0.0;
        float val = static_cast<float>(amplitude * gate * std::sin(2.0 * kPi * freq_hz * t));
        for (uint16_t c = 0; c < channels; ++c) {
            buf.samples[f * channels + c] = val;
        }
    }
    return buf;
}

}  // namespace synth

}  // namespace car_audio
