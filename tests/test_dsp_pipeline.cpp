#include "test_harness.hpp"
#include "car_audio/common/wav_io.hpp"
#include "car_audio/dsp/biquad_eq.hpp"
#include "car_audio/dsp/gain_smoother.hpp"
#include "car_audio/dsp/limiter.hpp"
#include "car_audio/dsp/resampler.hpp"
#include "car_audio/dsp/sdvc.hpp"
#include "car_audio/dsp/spatial_balance.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace car_audio;
using namespace car_audio::dsp;

TEST(DspPipeline, ResamplerMono16kToStereo48k) {
    AudioBuffer in = synth::generate_sine(1000.0, 0.1, 0.5f, 16000, 1);
    AudioBuffer out = Resampler::convert_to_engine_format(in, 48000);
    EXPECT_EQ(out.sample_rate, 48000u);
    EXPECT_EQ(out.channels, 2);
    EXPECT_EQ(out.frame_count(), 4800u);
    // Check L == R for mono-to-stereo upmix
    EXPECT_NEAR(out.samples[100], out.samples[101], 1e-6);
}

TEST(DspPipeline, GainSmootherRampsSmoothly) {
    GainSmoother gs(1.0f, 10.0, 48000);  // 480 samples ramp
    gs.set_target(0.25f);
    std::vector<float> buf(480 * 2, 1.0f);
    gs.process(buf.data(), 480);
    EXPECT_NEAR(gs.current(), 0.25f, 1e-4);
    // First sample should still be near 1.0, last sample near 0.25
    EXPECT_GT(buf[0], 0.99f);
    EXPECT_NEAR(buf.back(), 0.25f, 1e-3);
}

TEST(DspPipeline, SpatialFadeAndBalance) {
    SpatialBalance sb;
    sb.set_fade(0.5f);     // Towards front -> rear attenuated by 0.5
    sb.set_balance(-0.5f); // Towards left -> right attenuated by 0.5

    auto [dl, dr] = sb.zone_gains(AudioZoneId::Driver);
    EXPECT_NEAR(dl, 1.0f, 1e-5);
    EXPECT_NEAR(dr, 0.5f, 1e-5);

    auto [rl, rr] = sb.zone_gains(AudioZoneId::RearLeft);
    EXPECT_NEAR(rl, 0.5f, 1e-5);
    EXPECT_NEAR(rr, 0.25f, 1e-5);
}

TEST(DspPipeline, SdvcCurve) {
    Sdvc sdvc(SdvcConfig{true, 40.0f, 120.0f, 6.0f});
    sdvc.set_speed_kmh(20.0f);
    EXPECT_NEAR(sdvc.boost_db(), 0.0f, 1e-5);

    sdvc.set_speed_kmh(80.0f);
    EXPECT_NEAR(sdvc.boost_db(), 3.0f, 1e-4);

    sdvc.set_speed_kmh(140.0f);
    EXPECT_NEAR(sdvc.boost_db(), 6.0f, 1e-5);
}

TEST(DspPipeline, FiveBandEqBassBoostIncreasesLowFreqEnergy) {
    AudioBuffer low = synth::generate_sine(60.0, 0.2, 0.25f, 48000, 2);
    auto calc_rms = [](const std::vector<float>& s) {
        double sum = 0.0;
        for (float v : s) sum += v * v;
        return std::sqrt(sum / s.size());
    };
    double before_rms = calc_rms(low.samples);

    FiveBandEq eq(48000);
    eq.apply_preset("BassBoost");
    eq.process(low.samples.data(), low.frame_count());
    double after_rms = calc_rms(low.samples);

    EXPECT_GT(after_rms, before_rms * 1.4);
}

TEST(DspPipeline, LimiterPreventsClipping) {
    Limiter lim(0.9f, 0.5, 50.0, 48000);
    std::vector<float> hot(480 * 2, 2.0f);
    lim.process(hot.data(), 480);
    for (float s : hot) {
        EXPECT_TRUE(std::abs(s) <= 1.0f);
    }
    EXPECT_LT(lim.last_gain_reduction_db(), -1.0f);
}
