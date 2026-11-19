#include "test_harness.hpp"
#include "car_audio/focus/focus_manager.hpp"

using namespace car_audio;

TEST(FocusManager, NavigationDucksMediaAndRestores) {
    FocusManager fm;
    EXPECT_EQ(fm.request_focus(1, AudioZoneId::Driver, AudioUsage::Media, FocusGain::Gain),
              FocusDecision::Granted);
    EXPECT_EQ(*fm.get_state(1), FocusState::Active);

    EXPECT_EQ(fm.request_focus(2, AudioZoneId::Driver, AudioUsage::Navigation,
                               FocusGain::GainTransientMayDuck),
              FocusDecision::Granted);
    EXPECT_EQ(*fm.get_state(1), FocusState::Ducked);
    EXPECT_EQ(*fm.get_state(2), FocusState::Active);

    fm.abandon_focus(2);
    EXPECT_EQ(*fm.get_state(1), FocusState::Active);
}

TEST(FocusManager, CallPausesMediaAndRejectsNewMedia) {
    FocusManager fm;
    EXPECT_EQ(fm.request_focus(1, AudioZoneId::Driver, AudioUsage::Media, FocusGain::Gain),
              FocusDecision::Granted);
    EXPECT_EQ(fm.request_focus(2, AudioZoneId::Driver, AudioUsage::Call, FocusGain::GainTransient),
              FocusDecision::Granted);

    EXPECT_EQ(*fm.get_state(1), FocusState::Paused);
    EXPECT_EQ(*fm.get_state(2), FocusState::Active);

    // New media during active call should be rejected
    EXPECT_EQ(fm.request_focus(3, AudioZoneId::Driver, AudioUsage::Media, FocusGain::Gain),
              FocusDecision::Failed);

    // When call ends, media restores to Active
    fm.abandon_focus(2);
    EXPECT_EQ(*fm.get_state(1), FocusState::Active);
}

TEST(FocusManager, ZoneIsolationBetweenDriverAndRear) {
    FocusManager fm;
    EXPECT_EQ(fm.request_focus(1, AudioZoneId::Driver, AudioUsage::Media, FocusGain::Gain),
              FocusDecision::Granted);
    EXPECT_EQ(fm.request_focus(2, AudioZoneId::RearLeft, AudioUsage::Media, FocusGain::Gain),
              FocusDecision::Granted);

    // Driver navigation ducks Driver media, NOT RearLeft media
    EXPECT_EQ(fm.request_focus(3, AudioZoneId::Driver, AudioUsage::Navigation,
                               FocusGain::GainTransientMayDuck),
              FocusDecision::Granted);
    EXPECT_EQ(*fm.get_state(1), FocusState::Ducked);
    EXPECT_EQ(*fm.get_state(2), FocusState::Active);
}

TEST(FocusManager, GlobalEcallPreemptsAllZonesAndRejectsIncoming) {
    FocusManager fm;
    EXPECT_EQ(fm.request_focus(1, AudioZoneId::Driver, AudioUsage::Media, FocusGain::Gain),
              FocusDecision::Granted);
    EXPECT_EQ(fm.request_focus(2, AudioZoneId::RearLeft, AudioUsage::Media, FocusGain::Gain),
              FocusDecision::Granted);

    EXPECT_EQ(fm.request_focus(10, AudioZoneId::CabinGlobal, AudioUsage::EmergencyEcall,
                               FocusGain::GainTransientExclusive),
              FocusDecision::Granted);
    EXPECT_EQ(*fm.get_state(1), FocusState::Paused);
    EXPECT_EQ(*fm.get_state(2), FocusState::Paused);
    EXPECT_EQ(*fm.get_state(10), FocusState::Active);

    EXPECT_EQ(fm.request_focus(11, AudioZoneId::Passenger, AudioUsage::Media, FocusGain::Gain),
              FocusDecision::Failed);
}
