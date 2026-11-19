#pragma once

namespace car_audio::dsp {

struct SdvcConfig {
    bool enabled = true;
    float min_speed_kmh = 40.0f;
    float max_speed_kmh = 120.0f;
    float max_boost_db = 6.0f;
};

/// Speed-Dependent Volume Compensation (SDVC).
/// Boosts audio gain smoothly as vehicle speed rises to overcome road/wind noise.
class Sdvc {
public:
    explicit Sdvc(SdvcConfig cfg = {});

    void configure(const SdvcConfig& cfg);
    void set_speed_kmh(float speed_kmh);

    [[nodiscard]] float speed_kmh() const noexcept { return speed_kmh_; }
    [[nodiscard]] float boost_db() const noexcept;
    [[nodiscard]] float linear_gain() const noexcept;

private:
    SdvcConfig cfg_;
    float speed_kmh_ = 0.0f;
};

}  // namespace car_audio::dsp
