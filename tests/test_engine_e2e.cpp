#include "test_harness.hpp"
#include "car_audio/common/wav_io.hpp"
#include "car_audio/engine/audio_engine.hpp"
#include "car_audio/engine/audio_sink.hpp"
#include "car_audio/engine/audio_source.hpp"

#include <cmath>
#include <memory>

using namespace car_audio;

namespace {
double rms_range(const AudioBuffer& buf, size_t start_frame, size_t end_frame) {
    double sum = 0.0;
    size_t count = 0;
    for (size_t f = start_frame; f < end_frame && f < buf.frame_count(); ++f) {
        float l = buf.samples[f * buf.channels];
        sum += l * l;
        ++count;
    }
    return count == 0 ? 0.0 : std::sqrt(sum / count);
}
}  // namespace

TEST(EngineE2E, MultiZoneDuckingAndRestoration) {
    AudioEngine engine(AudioConfig::default_config());
    engine.set_volume("media", 40);       // 0 dB
    engine.set_volume("navigation", 40);  // 0 dB
    engine.set_vehicle_speed(0.0f);       // 0 dB SDVC

    auto sink = std::make_shared<WavFileSink>("");  // In-memory only
    engine.attach_sink(sink);

    auto music = std::make_shared<BufferAudioSource>(
        synth::generate_sine(440.0, 1.0, 0.5f), true);
    auto m_id = engine.start_stream(AudioZoneId::Driver, AudioUsage::Media, FocusGain::Gain, music);
    EXPECT_TRUE(m_id.has_value());

    // 0.0s -> 0.5s: Media active at full level
    engine.run_simulation(0.5);

    // 0.5s -> 1.0s: Navigation TTS plays on headrest, ducking Driver Media by -12 dB
    auto nav = std::make_shared<BufferAudioSource>(
        synth::generate_chime(660.0, 880.0, 0.5, 0.6f), false);
    auto n_id = engine.start_stream(AudioZoneId::Driver, AudioUsage::Navigation,
                                    FocusGain::GainTransientMayDuck, nav);
    EXPECT_TRUE(n_id.has_value());
    engine.run_simulation(0.5);

    // 1.0s -> 1.5s: Navigation finished, Media restored to full level
    engine.run_simulation(0.5);

    const auto& buses = sink->recorded_buses();
    const auto& media_bus = buses.at("bus_driver_media");
    const auto& headrest_bus = buses.at("bus_driver_headrest");

    double rms_active = rms_range(media_bus, 4800, 20000);   // 0.1s..0.41s
    double rms_ducked = rms_range(media_bus, 28800, 44000);  // 0.6s..0.91s (after 25ms ramp)
    double rms_restored = rms_range(media_bus, 52800, 68000);

    // -12 dB ~ 0.251x amplitude
    EXPECT_NEAR(rms_ducked / rms_active, 0.251, 0.03);
    EXPECT_NEAR(rms_restored / rms_active, 1.0, 0.03);

    // Headrest bus should have signal during 0.5s..1.0s
    double nav_rms = rms_range(headrest_bus, 25000, 35000);
    EXPECT_GT(nav_rms, 0.05);
}
