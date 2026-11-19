#include "car_audio/common/types.hpp"

#include <stdexcept>

namespace car_audio {

std::string to_string(AudioUsage usage) {
    switch (usage) {
        case AudioUsage::Media: return "MEDIA";
        case AudioUsage::Navigation: return "NAVIGATION";
        case AudioUsage::VoiceCommand: return "VOICE_COMMAND";
        case AudioUsage::Call: return "CALL";
        case AudioUsage::CallRing: return "CALL_RING";
        case AudioUsage::Alarm: return "ALARM";
        case AudioUsage::Notification: return "NOTIFICATION";
        case AudioUsage::AdasWarning: return "ADAS_WARNING";
        case AudioUsage::EmergencyEcall: return "EMERGENCY_ECALL";
    }
    return "UNKNOWN";
}

AudioUsage parse_audio_usage(const std::string& str) {
    if (str == "MEDIA") return AudioUsage::Media;
    if (str == "NAVIGATION") return AudioUsage::Navigation;
    if (str == "VOICE_COMMAND") return AudioUsage::VoiceCommand;
    if (str == "CALL") return AudioUsage::Call;
    if (str == "CALL_RING") return AudioUsage::CallRing;
    if (str == "ALARM") return AudioUsage::Alarm;
    if (str == "NOTIFICATION") return AudioUsage::Notification;
    if (str == "ADAS_WARNING") return AudioUsage::AdasWarning;
    if (str == "EMERGENCY_ECALL") return AudioUsage::EmergencyEcall;
    throw std::invalid_argument("Unknown AudioUsage: " + str);
}

std::string to_string(AudioZoneId zone) {
    switch (zone) {
        case AudioZoneId::Driver: return "DRIVER";
        case AudioZoneId::Passenger: return "PASSENGER";
        case AudioZoneId::RearLeft: return "REAR_LEFT";
        case AudioZoneId::RearRight: return "REAR_RIGHT";
        case AudioZoneId::CabinGlobal: return "CABIN_GLOBAL";
    }
    return "UNKNOWN";
}

AudioZoneId parse_audio_zone(const std::string& str) {
    if (str == "DRIVER") return AudioZoneId::Driver;
    if (str == "PASSENGER") return AudioZoneId::Passenger;
    if (str == "REAR_LEFT") return AudioZoneId::RearLeft;
    if (str == "REAR_RIGHT") return AudioZoneId::RearRight;
    if (str == "CABIN_GLOBAL") return AudioZoneId::CabinGlobal;
    throw std::invalid_argument("Unknown AudioZoneId: " + str);
}

std::string to_string(FocusGain gain) {
    switch (gain) {
        case FocusGain::Gain: return "GAIN";
        case FocusGain::GainTransient: return "GAIN_TRANSIENT";
        case FocusGain::GainTransientMayDuck: return "GAIN_TRANSIENT_MAY_DUCK";
        case FocusGain::GainTransientExclusive: return "GAIN_TRANSIENT_EXCLUSIVE";
    }
    return "UNKNOWN";
}

FocusGain parse_focus_gain(const std::string& str) {
    if (str == "GAIN") return FocusGain::Gain;
    if (str == "GAIN_TRANSIENT") return FocusGain::GainTransient;
    if (str == "GAIN_TRANSIENT_MAY_DUCK") return FocusGain::GainTransientMayDuck;
    if (str == "GAIN_TRANSIENT_EXCLUSIVE") return FocusGain::GainTransientExclusive;
    throw std::invalid_argument("Unknown FocusGain: " + str);
}

std::string to_string(FocusState state) {
    switch (state) {
        case FocusState::Active: return "ACTIVE";
        case FocusState::Ducked: return "DUCKED";
        case FocusState::Paused: return "PAUSED";
        case FocusState::Stopped: return "STOPPED";
    }
    return "UNKNOWN";
}

}  // namespace car_audio
