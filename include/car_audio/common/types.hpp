#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace car_audio {

/// Automotive audio usage categories modeled after AAOS AudioAttributes.
enum class AudioUsage {
    Media = 0,
    Navigation,
    VoiceCommand,
    Call,
    CallRing,
    Alarm,
    Notification,
    AdasWarning,
    EmergencyEcall
};

/// Logical cabin audio zones.
enum class AudioZoneId {
    Driver = 0,
    Passenger = 1,
    RearLeft = 2,
    RearRight = 3,
    CabinGlobal = 255  // Cross-zone broadcast (e.g. ADAS / eCall)
};

/// Audio focus gain types requested by a stream.
enum class FocusGain {
    Gain = 0,                    // Permanent focus (e.g., Media)
    GainTransient,               // Temporary exclusive (e.g., Call, VoiceCommand)
    GainTransientMayDuck,        // Temporary, allows lower-priority to duck (e.g., Navigation)
    GainTransientExclusive       // Strict exclusive, no ducking allowed (e.g., eCall / ADAS)
};

/// Runtime playback/focus state of an active stream.
enum class FocusState {
    Active = 0,
    Ducked,
    Paused,
    Stopped
};

/// Result of a focus acquisition request.
enum class FocusDecision {
    Granted = 0,
    Delayed,
    Failed
};

/// Interaction policy between an existing holder and an incoming requester.
enum class FocusInteraction {
    Concurrent = 0,  // Both play (may duck if requester is GainTransientMayDuck)
    Exclusive,       // Incoming pauses or stops existing holder
    Reject           // Incoming request is rejected
};

using StreamId = uint64_t;
using BusId = std::string;
using VolumeGroupId = std::string;

inline constexpr uint32_t kEngineSampleRate = 48000;
inline constexpr uint16_t kEngineChannels = 2;

/// Interleaved float32 audio buffer (normalized [-1.0, 1.0]).
struct AudioBuffer {
    uint32_t sample_rate = kEngineSampleRate;
    uint16_t channels = kEngineChannels;
    std::vector<float> samples;  // size = frames * channels

    [[nodiscard]] size_t frame_count() const noexcept {
        return channels == 0 ? 0 : samples.size() / channels;
    }

    void resize_frames(size_t frames) {
        samples.assign(frames * channels, 0.0f);
    }

    void clear_samples() {
        std::fill(samples.begin(), samples.end(), 0.0f);
    }
};

std::string to_string(AudioUsage usage);
AudioUsage parse_audio_usage(const std::string& str);

std::string to_string(AudioZoneId zone);
AudioZoneId parse_audio_zone(const std::string& str);

std::string to_string(FocusGain gain);
FocusGain parse_focus_gain(const std::string& str);

std::string to_string(FocusState state);

}  // namespace car_audio
