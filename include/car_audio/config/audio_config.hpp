#pragma once

#include "car_audio/common/types.hpp"
#include "car_audio/dsp/sdvc.hpp"

#include <map>
#include <string>
#include <vector>

namespace car_audio {

struct VolumeGroupConfig {
    VolumeGroupId id;
    int min_step = 0;
    int max_step = 40;
    int default_step = 20;
    float min_db = -60.0f;
    float max_db = 0.0f;

    /// Convert discrete UI volume step to linear gain using logarithmic curve.
    [[nodiscard]] float step_to_linear_gain(int step) const noexcept;
};

struct BusConfig {
    BusId id;
    AudioZoneId zone = AudioZoneId::Driver;
    VolumeGroupId volume_group;
    std::vector<AudioUsage> usages;
};

struct ZoneConfig {
    AudioZoneId id = AudioZoneId::Driver;
    std::string name;
    std::string eq_preset = "Flat";
};

struct AudioConfig {
    std::vector<ZoneConfig> zones;
    std::vector<BusConfig> buses;
    std::map<VolumeGroupId, VolumeGroupConfig> volume_groups;

    float duck_gain_db = -12.0f;
    double duck_ramp_ms = 25.0;
    dsp::SdvcConfig sdvc;

    [[nodiscard]] float duck_linear_gain() const noexcept;

    static AudioConfig default_config();
    static AudioConfig load_from_json(const std::string& path);
};

}  // namespace car_audio
