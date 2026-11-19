#pragma once

#include "car_audio/common/types.hpp"
#include "car_audio/config/audio_config.hpp"

#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace car_audio {

struct RouteTarget {
    BusId bus_id;
    AudioZoneId bus_zone = AudioZoneId::Driver;
    VolumeGroupId volume_group;
};

/// Resolves (AudioZoneId, AudioUsage) to a physical output BusId and VolumeGroup.
class AudioRouter {
public:
    explicit AudioRouter(const AudioConfig& config);

    /// Resolve target bus for a given zone & usage.
    [[nodiscard]] std::optional<RouteTarget> resolve(AudioZoneId zone, AudioUsage usage) const;

    /// Dynamically override routing for a specific (zone, usage) pair (e.g., headrest privacy).
    void set_route_override(AudioZoneId zone, AudioUsage usage, const BusId& bus_id);
    void clear_route_override(AudioZoneId zone, AudioUsage usage);

    [[nodiscard]] const std::vector<BusConfig>& buses() const noexcept { return buses_; }

private:
    std::vector<BusConfig> buses_;
    std::map<std::pair<AudioZoneId, AudioUsage>, BusId> overrides_;

    [[nodiscard]] const BusConfig* find_bus(const BusId& id) const;
};

}  // namespace car_audio
