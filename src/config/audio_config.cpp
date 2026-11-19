#include "car_audio/config/audio_config.hpp"
#include "car_audio/common/json.hpp"

#include <algorithm>
#include <cmath>

namespace car_audio {

float VolumeGroupConfig::step_to_linear_gain(int step) const noexcept {
    if (step <= min_step) return 0.0f;
    if (max_step <= min_step) return 1.0f;
    float norm = std::clamp(
        static_cast<float>(step - min_step) / static_cast<float>(max_step - min_step),
        0.0f, 1.0f);
    float db = min_db + norm * (max_db - min_db);
    return std::pow(10.0f, db / 20.0f);
}

float AudioConfig::duck_linear_gain() const noexcept {
    return std::pow(10.0f, duck_gain_db / 20.0f);
}

AudioConfig AudioConfig::default_config() {
    AudioConfig cfg;
    cfg.duck_gain_db = -12.0f;
    cfg.duck_ramp_ms = 25.0;
    cfg.sdvc = dsp::SdvcConfig{true, 40.0f, 120.0f, 6.0f};

    cfg.zones = {
        {AudioZoneId::Driver, "Driver", "Flat"},
        {AudioZoneId::Passenger, "Passenger", "Flat"},
        {AudioZoneId::RearLeft, "RearLeft", "Flat"},
        {AudioZoneId::RearRight, "RearRight", "Flat"},
    };

    cfg.volume_groups = {
        {"media", {"media", 0, 40, 24, -60.0f, 0.0f}},
        {"navigation", {"navigation", 0, 40, 28, -48.0f, 0.0f}},
        {"voice", {"voice", 0, 40, 28, -48.0f, 0.0f}},
        {"call", {"call", 0, 40, 30, -48.0f, 0.0f}},
        {"safety", {"safety", 10, 40, 34, -30.0f, 0.0f}},
    };

    cfg.buses = {
        {"bus_driver_media", AudioZoneId::Driver, "media",
         {AudioUsage::Media, AudioUsage::Alarm, AudioUsage::Notification}},
        {"bus_driver_headrest", AudioZoneId::Driver, "navigation",
         {AudioUsage::Navigation, AudioUsage::VoiceCommand, AudioUsage::Call, AudioUsage::CallRing}},
        {"bus_passenger_media", AudioZoneId::Passenger, "media",
         {AudioUsage::Media, AudioUsage::Notification}},
        {"bus_rear_left_media", AudioZoneId::RearLeft, "media",
         {AudioUsage::Media, AudioUsage::Notification}},
        {"bus_rear_right_media", AudioZoneId::RearRight, "media",
         {AudioUsage::Media, AudioUsage::Notification}},
        {"bus_cabin_safety", AudioZoneId::CabinGlobal, "safety",
         {AudioUsage::AdasWarning, AudioUsage::EmergencyEcall}},
    };

    return cfg;
}

AudioConfig AudioConfig::load_from_json(const std::string& path) {
    JsonValue root = JsonValue::parse_file(path);
    AudioConfig cfg = default_config();

    if (root.contains("duck_gain_db")) {
        cfg.duck_gain_db = static_cast<float>(root.at("duck_gain_db").as_double(-12.0));
    }
    if (root.contains("duck_ramp_ms")) {
        cfg.duck_ramp_ms = root.at("duck_ramp_ms").as_double(25.0);
    }
    if (root.contains("sdvc")) {
        const auto& s = root.at("sdvc");
        cfg.sdvc.enabled = s.get_or("enabled", true).as_bool(true);
        cfg.sdvc.min_speed_kmh = static_cast<float>(s.get_or("min_speed_kmh", 40.0).as_double(40.0));
        cfg.sdvc.max_speed_kmh = static_cast<float>(s.get_or("max_speed_kmh", 120.0).as_double(120.0));
        cfg.sdvc.max_boost_db = static_cast<float>(s.get_or("max_boost_db", 6.0).as_double(6.0));
    }

    if (root.contains("zones")) {
        cfg.zones.clear();
        for (const auto& z : root.at("zones").as_array()) {
            ZoneConfig zc;
            zc.id = parse_audio_zone(z.at("id").as_string());
            zc.name = z.get_or("name", to_string(zc.id)).as_string();
            zc.eq_preset = z.get_or("eq_preset", "Flat").as_string();
            cfg.zones.push_back(std::move(zc));
        }
    }

    if (root.contains("volume_groups")) {
        cfg.volume_groups.clear();
        for (const auto& vg : root.at("volume_groups").as_array()) {
            VolumeGroupConfig vc;
            vc.id = vg.at("id").as_string();
            vc.min_step = static_cast<int>(vg.get_or("min_step", 0).as_int(0));
            vc.max_step = static_cast<int>(vg.get_or("max_step", 40).as_int(40));
            vc.default_step = static_cast<int>(vg.get_or("default_step", 20).as_int(20));
            vc.min_db = static_cast<float>(vg.get_or("min_db", -60.0).as_double(-60.0));
            vc.max_db = static_cast<float>(vg.get_or("max_db", 0.0).as_double(0.0));
            cfg.volume_groups[vc.id] = vc;
        }
    }

    if (root.contains("buses")) {
        cfg.buses.clear();
        for (const auto& b : root.at("buses").as_array()) {
            BusConfig bc;
            bc.id = b.at("id").as_string();
            bc.zone = parse_audio_zone(b.at("zone").as_string());
            bc.volume_group = b.at("volume_group").as_string();
            for (const auto& u : b.at("usages").as_array()) {
                bc.usages.push_back(parse_audio_usage(u.as_string()));
            }
            cfg.buses.push_back(std::move(bc));
        }
    }

    return cfg;
}

}  // namespace car_audio
