#pragma once

#include "car_audio/common/types.hpp"
#include "car_audio/config/audio_config.hpp"
#include "car_audio/dsp/biquad_eq.hpp"
#include "car_audio/dsp/gain_smoother.hpp"
#include "car_audio/dsp/limiter.hpp"
#include "car_audio/dsp/sdvc.hpp"
#include "car_audio/dsp/spatial_balance.hpp"
#include "car_audio/engine/audio_sink.hpp"
#include "car_audio/engine/audio_source.hpp"
#include "car_audio/focus/focus_manager.hpp"
#include "car_audio/routing/audio_router.hpp"

#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace car_audio {

struct ActiveStreamInfo {
    StreamId id = 0;
    AudioZoneId zone = AudioZoneId::Driver;
    AudioUsage usage = AudioUsage::Media;
    FocusGain focus_gain = FocusGain::Gain;
    FocusState focus_state = FocusState::Active;
    BusId bus_id;
    float current_gain = 1.0f;
};

/// Core automotive multi-zone audio engine.
/// Integrates FocusManager, AudioRouter, multi-bus mixing, and full DSP pipeline.
class AudioEngine {
public:
    explicit AudioEngine(AudioConfig config = AudioConfig::default_config());

    void attach_sink(std::shared_ptr<IAudioSink> sink);

    /// Request focus, resolve route, and start an audio stream.
    /// Returns std::nullopt if focus is rejected or no route exists.
    std::optional<StreamId> start_stream(AudioZoneId zone, AudioUsage usage, FocusGain gain,
                                         std::shared_ptr<IAudioSource> source);

    /// Stop a stream and abandon its audio focus.
    void stop_stream(StreamId id);

    /// Volume & cabin DSP controls
    void set_volume(const VolumeGroupId& group_id, int step);
    [[nodiscard]] int get_volume(const VolumeGroupId& group_id) const;

    void set_vehicle_speed(float speed_kmh);
    [[nodiscard]] float vehicle_speed() const noexcept { return sdvc_.speed_kmh(); }

    void set_fade_balance(float fade, float balance);
    void set_zone_eq_preset(AudioZoneId zone, const std::string& preset_name);

    void set_route_override(AudioZoneId zone, AudioUsage usage, const BusId& bus_id);
    void clear_route_override(AudioZoneId zone, AudioUsage usage);

    /// Process one audio block (default 480 frames = 10ms @ 48kHz).
    void process_block(size_t frames = 480);

    /// Run simulation for a fixed duration in 10ms blocks.
    void run_simulation(double duration_sec, size_t block_frames = 480);

    [[nodiscard]] std::vector<ActiveStreamInfo> active_streams() const;
    [[nodiscard]] const FocusManager& focus_manager() const noexcept { return focus_manager_; }
    [[nodiscard]] const AudioRouter& router() const noexcept { return router_; }
    [[nodiscard]] const AudioConfig& config() const noexcept { return config_; }
    [[nodiscard]] uint64_t processed_frames() const noexcept { return total_frames_; }

private:
    struct StreamContext {
        StreamId id = 0;
        AudioZoneId zone = AudioZoneId::Driver;
        AudioUsage usage = AudioUsage::Media;
        FocusGain focus_gain = FocusGain::Gain;
        RouteTarget route;
        std::shared_ptr<IAudioSource> source;
        dsp::GainSmoother smoother;
    };

    AudioConfig config_;
    FocusManager focus_manager_;
    AudioRouter router_;
    dsp::Sdvc sdvc_;
    dsp::SpatialBalance spatial_balance_;

    std::map<VolumeGroupId, int> volume_steps_;
    std::map<AudioZoneId, dsp::FiveBandEq> zone_eqs_;
    std::map<BusId, dsp::Limiter> bus_limiters_;

    std::vector<StreamContext> streams_;
    std::shared_ptr<IAudioSink> sink_;
    StreamId next_stream_id_ = 1;
    uint64_t total_frames_ = 0;

    void on_focus_changed(const FocusChangeEvent& ev);
    [[nodiscard]] float target_gain_for_state(FocusState state) const noexcept;
    [[nodiscard]] float volume_group_gain(const VolumeGroupId& group_id) const noexcept;
};

}  // namespace car_audio
