#include "car_audio/focus/focus_manager.hpp"

#include <algorithm>

namespace car_audio {

void FocusManager::set_callback(FocusCallback cb) {
    callback_ = std::move(cb);
}

int FocusManager::usage_priority(AudioUsage usage) {
    switch (usage) {
        case AudioUsage::Media: return 10;
        case AudioUsage::Notification: return 20;
        case AudioUsage::Alarm: return 30;
        case AudioUsage::Navigation: return 40;
        case AudioUsage::VoiceCommand: return 50;
        case AudioUsage::CallRing: return 60;
        case AudioUsage::Call: return 70;
        case AudioUsage::AdasWarning: return 90;
        case AudioUsage::EmergencyEcall: return 100;
    }
    return 0;
}

bool FocusManager::zones_conflict(AudioZoneId a, AudioZoneId b) {
    return a == b || a == AudioZoneId::CabinGlobal || b == AudioZoneId::CabinGlobal;
}

FocusInteraction FocusManager::evaluate_interaction(AudioUsage existing, AudioUsage incoming) {
    // eCall is highest priority: rejects everything below it, preempted by nothing.
    if (existing == AudioUsage::EmergencyEcall) {
        return incoming == AudioUsage::EmergencyEcall ? FocusInteraction::Exclusive
                                                      : FocusInteraction::Reject;
    }
    if (incoming == AudioUsage::EmergencyEcall) {
        return FocusInteraction::Exclusive;
    }

    // ADAS warning preempts media/nav/voice/notifications, runs concurrently with Call?
    // Safety-first: ADAS warning ducks Call/CallRing (or concurrent) and pauses Media/Nav/Voice.
    if (existing == AudioUsage::AdasWarning) {
        if (incoming == AudioUsage::AdasWarning) return FocusInteraction::Exclusive;
        if (incoming == AudioUsage::Call || incoming == AudioUsage::CallRing) {
            return FocusInteraction::Concurrent;
        }
        return FocusInteraction::Reject;
    }
    if (incoming == AudioUsage::AdasWarning) {
        if (existing == AudioUsage::Call || existing == AudioUsage::CallRing) {
            return FocusInteraction::Concurrent;
        }
        return FocusInteraction::Exclusive;
    }

    // Active Call or CallRing
    if (existing == AudioUsage::Call || existing == AudioUsage::CallRing) {
        if (incoming == AudioUsage::Navigation || incoming == AudioUsage::Notification) {
            return FocusInteraction::Concurrent;
        }
        if (incoming == AudioUsage::Call || incoming == AudioUsage::CallRing) {
            return FocusInteraction::Exclusive;
        }
        return FocusInteraction::Reject;
    }
    if (incoming == AudioUsage::Call || incoming == AudioUsage::CallRing) {
        return FocusInteraction::Exclusive;
    }

    // Voice Command (Assistant)
    if (existing == AudioUsage::VoiceCommand) {
        if (incoming == AudioUsage::VoiceCommand) return FocusInteraction::Exclusive;
        if (incoming == AudioUsage::Navigation) return FocusInteraction::Concurrent;
        return FocusInteraction::Reject;
    }
    if (incoming == AudioUsage::VoiceCommand) {
        return FocusInteraction::Exclusive;
    }

    // Navigation / Alarm / Notification / Media
    if (incoming == AudioUsage::Media && existing == AudioUsage::Media) {
        return FocusInteraction::Exclusive;  // New media replaces old media in same zone
    }

    // Navigation / Notification / Alarm vs Media -> Concurrent (allows ducking)
    return FocusInteraction::Concurrent;
}

FocusDecision FocusManager::request_focus(StreamId stream_id, AudioZoneId zone, AudioUsage usage,
                                          FocusGain gain) {
    // Check if any conflicting active/paused holder rejects this request
    for (const auto& entry : entries_) {
        if (!zones_conflict(entry.zone, zone)) continue;
        if (entry.state == FocusState::Stopped) continue;
        if (evaluate_interaction(entry.usage, usage) == FocusInteraction::Reject) {
            return FocusDecision::Failed;
        }
    }

    // If same stream_id already exists, update it; otherwise append
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [stream_id](const FocusEntry& e) { return e.stream_id == stream_id; });
    if (it != entries_.end()) {
        it->zone = zone;
        it->usage = usage;
        it->gain = gain;
        it->state = FocusState::Active;
    } else {
        entries_.push_back(FocusEntry{stream_id, zone, usage, gain, FocusState::Active});
    }

    recompute_states();
    return FocusDecision::Granted;
}

void FocusManager::abandon_focus(StreamId stream_id) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [stream_id](const FocusEntry& e) { return e.stream_id == stream_id; });
    if (it == entries_.end()) return;

    FocusEntry removed = *it;
    entries_.erase(it);

    if (callback_ && removed.state != FocusState::Stopped) {
        callback_(FocusChangeEvent{removed.stream_id, removed.zone, removed.usage, removed.state,
                                   FocusState::Stopped});
    }

    recompute_states();
}

std::optional<FocusState> FocusManager::get_state(StreamId stream_id) const {
    for (const auto& e : entries_) {
        if (e.stream_id == stream_id) return e.state;
    }
    return std::nullopt;
}

void FocusManager::recompute_states() {
    // Determine target state for each entry based on all newer/higher-priority conflicting entries
    std::vector<FocusState> new_states(entries_.size(), FocusState::Active);

    for (size_t i = 0; i < entries_.size(); ++i) {
        const auto& holder = entries_[i];
        for (size_t j = 0; j < entries_.size(); ++j) {
            if (i == j) continue;
            const auto& other = entries_[j];
            if (!zones_conflict(holder.zone, other.zone)) continue;

            // Only a newer entry or a strictly higher-priority entry affects 'holder'
            bool other_has_precedence =
                (usage_priority(other.usage) > usage_priority(holder.usage)) ||
                (usage_priority(other.usage) == usage_priority(holder.usage) && j > i);
            if (!other_has_precedence) continue;

            FocusInteraction inter = evaluate_interaction(holder.usage, other.usage);
            if (inter == FocusInteraction::Exclusive) {
                if (holder.gain == FocusGain::Gain && other.gain != FocusGain::Gain) {
                    new_states[i] = FocusState::Paused;
                } else {
                    new_states[i] = FocusState::Paused;
                }
            } else if (inter == FocusInteraction::Concurrent) {
                if (other.gain == FocusGain::GainTransientMayDuck &&
                    usage_priority(holder.usage) < usage_priority(other.usage)) {
                    if (new_states[i] == FocusState::Active) {
                        new_states[i] = FocusState::Ducked;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].state != new_states[i]) {
            FocusState old_st = entries_[i].state;
            entries_[i].state = new_states[i];
            if (callback_) {
                callback_(FocusChangeEvent{entries_[i].stream_id, entries_[i].zone,
                                           entries_[i].usage, old_st, new_states[i]});
            }
        }
    }
}

}  // namespace car_audio
