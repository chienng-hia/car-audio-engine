#include "car_audio/engine/audio_engine.hpp"

#include <algorithm>

namespace car_audio {

AudioEngine::AudioEngine(AudioConfig config)
    : config_(std::move(config)),
      router_(config_),
      sdvc_(config_.sdvc) {
    focus_manager_.set_callback([this](const FocusChangeEvent& ev) { on_focus_changed(ev); });

    for (const auto& [id, vg] : config_.volume_groups) {
        volume_steps_[id] = vg.default_step;
    }

    for (const auto& z : config_.zones) {
        dsp::FiveBandEq eq(kEngineSampleRate);
        eq.apply_preset(z.eq_preset);
        zone_eqs_.emplace(z.id, std::move(eq));
    }
    // Ensure CabinGlobal has an EQ instance (Flat)
    if (zone_eqs_.find(AudioZoneId::CabinGlobal) == zone_eqs_.end()) {
        zone_eqs_.emplace(AudioZoneId::CabinGlobal, dsp::FiveBandEq(kEngineSampleRate));
    }

    for (const auto& bus : config_.buses) {
        bus_limiters_.emplace(bus.id, dsp::Limiter(0.95f, 1.0, 50.0, kEngineSampleRate));
    }
}

void AudioEngine::attach_sink(std::shared_ptr<IAudioSink> sink) {
    sink_ = std::move(sink);
}

std::optional<StreamId> AudioEngine::start_stream(AudioZoneId zone, AudioUsage usage,
                                                  FocusGain gain,
                                                  std::shared_ptr<IAudioSource> source) {
    if (!source) return std::nullopt;

    auto route = router_.resolve(zone, usage);
    if (!route.has_value()) {
        return std::nullopt;
    }

    StreamId id = next_stream_id_++;
    FocusDecision decision = focus_manager_.request_focus(id, zone, usage, gain);
    if (decision != FocusDecision::Granted) {
        return std::nullopt;
    }

    FocusState state = focus_manager_.get_state(id).value_or(FocusState::Active);
    float init_gain = target_gain_for_state(state);

    StreamContext ctx{
        id,
        zone,
        usage,
        gain,
        *route,
        std::move(source),
        dsp::GainSmoother(init_gain, config_.duck_ramp_ms, kEngineSampleRate),
    };
    streams_.push_back(std::move(ctx));
    return id;
}

void AudioEngine::stop_stream(StreamId id) {
    auto it = std::find_if(streams_.begin(), streams_.end(),
                           [id](const StreamContext& s) { return s.id == id; });
    if (it != streams_.end()) {
        streams_.erase(it);
    }
    focus_manager_.abandon_focus(id);
}

void AudioEngine::set_volume(const VolumeGroupId& group_id, int step) {
    auto it = config_.volume_groups.find(group_id);
    if (it == config_.volume_groups.end()) return;
    volume_steps_[group_id] = std::clamp(step, it->second.min_step, it->second.max_step);
}

int AudioEngine::get_volume(const VolumeGroupId& group_id) const {
    auto it = volume_steps_.find(group_id);
    return it != volume_steps_.end() ? it->second : 0;
}

void AudioEngine::set_vehicle_speed(float speed_kmh) {
    sdvc_.set_speed_kmh(speed_kmh);
}

void AudioEngine::set_fade_balance(float fade, float balance) {
    spatial_balance_.set_fade(fade);
    spatial_balance_.set_balance(balance);
}

void AudioEngine::set_zone_eq_preset(AudioZoneId zone, const std::string& preset_name) {
    auto it = zone_eqs_.find(zone);
    if (it != zone_eqs_.end()) {
        it->second.apply_preset(preset_name);
    }
}

void AudioEngine::set_route_override(AudioZoneId zone, AudioUsage usage, const BusId& bus_id) {
    router_.set_route_override(zone, usage, bus_id);
}

void AudioEngine::clear_route_override(AudioZoneId zone, AudioUsage usage) {
    router_.clear_route_override(zone, usage);
}

void AudioEngine::on_focus_changed(const FocusChangeEvent& ev) {
    for (auto& s : streams_) {
        if (s.id == ev.stream_id) {
            s.smoother.set_target(target_gain_for_state(ev.new_state));
            break;
        }
    }
}

float AudioEngine::target_gain_for_state(FocusState state) const noexcept {
    switch (state) {
        case FocusState::Active: return 1.0f;
        case FocusState::Ducked: return config_.duck_linear_gain();
        case FocusState::Paused:
        case FocusState::Stopped: return 0.0f;
    }
    return 0.0f;
}

float AudioEngine::volume_group_gain(const VolumeGroupId& group_id) const noexcept {
    auto cfg_it = config_.volume_groups.find(group_id);
    if (cfg_it == config_.volume_groups.end()) return 1.0f;
    int step = get_volume(group_id);
    return cfg_it->second.step_to_linear_gain(step);
}

void AudioEngine::process_block(size_t frames) {
    // 1. Prepare zeroed mix buffers for every configured bus
    std::map<BusId, std::vector<float>> bus_mix;
    for (const auto& bus : config_.buses) {
        bus_mix[bus.id].assign(frames * kEngineChannels, 0.0f);
    }

    std::vector<float> scratch(frames * kEngineChannels, 0.0f);
    std::vector<StreamId> finished_ids;

    // 2. Read & mix each stream into its target bus
    for (auto& s : streams_) {
        FocusState st = focus_manager_.get_state(s.id).value_or(FocusState::Stopped);

        // If paused and ramp-down completed, pause reading from source
        if (st == FocusState::Paused && s.smoother.current() <= 1e-5f) {
            continue;
        }

        std::fill(scratch.begin(), scratch.end(), 0.0f);
        size_t read = s.source->read_frames(scratch.data(), frames);
        if (read == 0 && s.source->is_finished()) {
            finished_ids.push_back(s.id);
            continue;
        }

        s.smoother.process(scratch.data(), frames);
        const float vg_gain = volume_group_gain(s.route.volume_group);
        // Apply SDVC to entertainment/nav/voice, keep safety bus deterministic
        const float sdvc_gain = (s.route.volume_group == "safety") ? 1.0f : sdvc_.linear_gain();
        const float total_gain = vg_gain * sdvc_gain;

        auto& dest = bus_mix[s.route.bus_id];
        for (size_t i = 0; i < frames * kEngineChannels; ++i) {
            dest[i] += scratch[i] * total_gain;
        }

        if (s.source->is_finished()) {
            finished_ids.push_back(s.id);
        }
    }

    // 3. Apply per-bus Zone EQ, Spatial Fade/Balance, Limiter, and write to sink
    for (const auto& bus : config_.buses) {
        auto& buf = bus_mix[bus.id];
        auto eq_it = zone_eqs_.find(bus.zone);
        if (eq_it != zone_eqs_.end()) {
            eq_it->second.process(buf.data(), frames);
        }

        if (bus.zone != AudioZoneId::CabinGlobal) {
            spatial_balance_.process(bus.zone, buf.data(), frames);
        }

        auto lim_it = bus_limiters_.find(bus.id);
        if (lim_it != bus_limiters_.end()) {
            lim_it->second.process(buf.data(), frames);
        }

        if (sink_) {
            sink_->write_bus_frames(bus.id, buf.data(), frames);
        }
    }

    // 4. Clean up finished streams and abandon their focus
    for (StreamId fid : finished_ids) {
        stop_stream(fid);
    }

    total_frames_ += frames;
}

void AudioEngine::run_simulation(double duration_sec, size_t block_frames) {
    const size_t total_target = static_cast<size_t>(duration_sec * kEngineSampleRate);
    size_t done = 0;
    while (done < total_target) {
        size_t chunk = std::min(block_frames, total_target - done);
        process_block(chunk);
        done += chunk;
    }
}

std::vector<ActiveStreamInfo> AudioEngine::active_streams() const {
    std::vector<ActiveStreamInfo> res;
    res.reserve(streams_.size());
    for (const auto& s : streams_) {
        res.push_back(ActiveStreamInfo{
            s.id,
            s.zone,
            s.usage,
            s.focus_gain,
            focus_manager_.get_state(s.id).value_or(FocusState::Stopped),
            s.route.bus_id,
            s.smoother.current(),
        });
    }
    return res;
}

}  // namespace car_audio
