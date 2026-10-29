#include <gtest/gtest.h>
#include "api/audio_backend.hpp"
#include "support/scoped_state.hpp"
#include <limits>
TEST(AudioSessionContract, UniqueOwnerAndFiniteLevels) {
    game::AudioSession* session = nullptr;
    ASSERT_TRUE(game::create_audio_session(48000, session));
    const auto cleanup = ra2::test::scope_exit([&] { game::destroy_audio_session(session); });
    game::AudioSession* other = nullptr;
    EXPECT_FALSE(game::create_audio_session(48000, other));
    EXPECT_EQ(other, nullptr);
    EXPECT_FALSE(game::set_audio_levels(*session, std::numeric_limits<float>::quiet_NaN(), 1, 1));
    ASSERT_TRUE(game::set_audio_levels(*session, 0.25f, 0.5f, 0.75f));
    float effects = 0, speech = 0, music = 0;
    ASSERT_TRUE(game::get_audio_levels(*session, effects, speech, music));
    EXPECT_FLOAT_EQ(effects, 0.25f);
    EXPECT_FLOAT_EQ(speech, 0.5f);
    EXPECT_FLOAT_EQ(music, 0.75f);
}
