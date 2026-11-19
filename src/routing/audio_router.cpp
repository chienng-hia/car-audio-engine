#include "car_audio/routing/audio_router.hpp"

#include <algorithm>

namespace car_audio {

AudioRouter::AudioRouter(const AudioConfig& config) : buses_(config.buses) {}

const BusConfig* AudioRouter::find_bus(const BusId& id) const {
    for (const auto& bus : buses_) {
        if (bus.id == id) return &bus;
    }
    return nullptr;
}

std::optional<RouteTarget> AudioRouter::resolve(AudioZoneId zone, AudioUsage usage) const {
    auto ov_it = overrides_.find({zone, usage});
    if (ov_it != overrides_.end()) {
        if (const auto* bus = find_bus(ov_it->second)) {
            return RouteTarget{bus->id, bus->zone, bus->volume_group};
        }
    }

    // 1. Exact zone match
    for (const auto& bus : buses_) {
        if (bus.zone == zone &&
            std::find(bus.usages.begin(), bus.usages.end(), usage) != bus.usages.end()) {
            return RouteTarget{bus.id, bus.zone, bus.volume_group};
        }
    }

    // 2. CabinGlobal safety/broadcast fallback
    for (const auto& bus : buses_) {
        if (bus.zone == AudioZoneId::CabinGlobal &&
            std::find(bus.usages.begin(), bus.usages.end(), usage) != bus.usages.end()) {
            return RouteTarget{bus.id, bus.zone, bus.volume_group};
        }
    }

    return std::nullopt;
}

void AudioRouter::set_route_override(AudioZoneId zone, AudioUsage usage, const BusId& bus_id) {
    if (find_bus(bus_id) != nullptr) {
        overrides_[{zone, usage}] = bus_id;
    }
}

void AudioRouter::clear_route_override(AudioZoneId zone, AudioUsage usage) {
    overrides_.erase({zone, usage});
}

}  // namespace car_audio
