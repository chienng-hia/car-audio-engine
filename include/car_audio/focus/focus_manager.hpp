#pragma once

#include "car_audio/common/types.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace car_audio {

struct FocusEntry {
    StreamId stream_id = 0;
    AudioZoneId zone = AudioZoneId::Driver;
    AudioUsage usage = AudioUsage::Media;
    FocusGain gain = FocusGain::Gain;
    FocusState state = FocusState::Active;
};

struct FocusChangeEvent {
    StreamId stream_id = 0;
    AudioZoneId zone = AudioZoneId::Driver;
    AudioUsage usage = AudioUsage::Media;
    FocusState old_state = FocusState::Active;
    FocusState new_state = FocusState::Active;
};

/// AAOS-inspired multi-zone automotive audio focus arbiter.
/// Supports per-zone isolation, cross-zone global preemption (eCall / ADAS),
/// ducking, exclusive pausing, and automatic restoration on focus abandon.
class FocusManager {
public:
    using FocusCallback = std::function<void(const FocusChangeEvent&)>;

    void set_callback(FocusCallback cb);

    /// Request audio focus for a stream.
    FocusDecision request_focus(StreamId stream_id, AudioZoneId zone, AudioUsage usage,
                                FocusGain gain);

    /// Abandon audio focus when a stream finishes or stops.
    void abandon_focus(StreamId stream_id);

    /// Query current state of a stream.
    [[nodiscard]] std::optional<FocusState> get_state(StreamId stream_id) const;

    /// Get all currently held focus entries.
    [[nodiscard]] const std::vector<FocusEntry>& entries() const noexcept { return entries_; }

    /// Evaluate interaction between an existing holder and an incoming requester.
    [[nodiscard]] static FocusInteraction evaluate_interaction(AudioUsage existing,
                                                               AudioUsage incoming);

    /// Priority rank (higher value = higher priority).
    [[nodiscard]] static int usage_priority(AudioUsage usage);

private:
    std::vector<FocusEntry> entries_;
    FocusCallback callback_;

    static bool zones_conflict(AudioZoneId a, AudioZoneId b);
    void recompute_states();
};

}  // namespace car_audio
