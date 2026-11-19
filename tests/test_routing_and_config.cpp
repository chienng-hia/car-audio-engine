#include "test_harness.hpp"
#include "car_audio/config/audio_config.hpp"
#include "car_audio/routing/audio_router.hpp"

using namespace car_audio;

TEST(RoutingAndConfig, DefaultConfigRoutesDriverAndHeadrest) {
    AudioConfig cfg = AudioConfig::default_config();
    AudioRouter router(cfg);

    auto media_route = router.resolve(AudioZoneId::Driver, AudioUsage::Media);
    EXPECT_TRUE(media_route.has_value());
    EXPECT_EQ(media_route->bus_id, "bus_driver_media");

    auto nav_route = router.resolve(AudioZoneId::Driver, AudioUsage::Navigation);
    EXPECT_TRUE(nav_route.has_value());
    EXPECT_EQ(nav_route->bus_id, "bus_driver_headrest");

    auto adas_route = router.resolve(AudioZoneId::Driver, AudioUsage::AdasWarning);
    EXPECT_TRUE(adas_route.has_value());
    EXPECT_EQ(adas_route->bus_id, "bus_cabin_safety");
}

TEST(RoutingAndConfig, DynamicRouteOverrideWorks) {
    AudioConfig cfg = AudioConfig::default_config();
    AudioRouter router(cfg);

    router.set_route_override(AudioZoneId::Driver, AudioUsage::Navigation, "bus_driver_media");
    auto nav_route = router.resolve(AudioZoneId::Driver, AudioUsage::Navigation);
    EXPECT_TRUE(nav_route.has_value());
    EXPECT_EQ(nav_route->bus_id, "bus_driver_media");

    router.clear_route_override(AudioZoneId::Driver, AudioUsage::Navigation);
    nav_route = router.resolve(AudioZoneId::Driver, AudioUsage::Navigation);
    EXPECT_EQ(nav_route->bus_id, "bus_driver_headrest");
}

TEST(RoutingAndConfig, LoadFromProjectJsonFile) {
    AudioConfig cfg = AudioConfig::load_from_json("config/car_audio_config.json");
    EXPECT_EQ(cfg.zones.size(), 4u);
    EXPECT_EQ(cfg.buses.size(), 6u);
    EXPECT_NEAR(cfg.duck_gain_db, -12.0f, 1e-5);
}
