#include "support/test_support.hpp"
#include "api/video_backend.hpp"
#include "yrpp/Unsorted.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <type_traits>

static_assert(std::is_same_v<decltype(&Game::PlayMovie),
    void (YRPP_FASTCALL*)(const char*, int, char, char, char, char)>);

namespace {

class Probe final : public game::VideoBackend {
public:
    const char* filename = nullptr;
    int theme = 0;
    std::array<char, 4> options{};
    int calls = 0;
    int* destructions = nullptr;
    bool returned = false;
    ~Probe() override { if (destructions) ++*destructions; }
    void play_movie(const char* name, int queue_theme, char a, char b, char c, char d) noexcept override {
        filename = name;
        theme = queue_theme;
        options = {a, b, c, d};
        ++calls;
        returned = true;
    }
};
TEST(VideoBackend, Contracts) {
    game::reset_video_backend();
    Probe first;
    game::VideoBackend* output = &first;
    EXPECT_TRUE((!game::get_video_backend(output) && output == &first)) << "missing provider preserves output";
    Game::PlayMovie("absent.bik"); // The standalone unavailable path is a recorded no-op.
    EXPECT_TRUE((first.calls == 0)) << "unregistered backend is not called";

    game::set_video_backend(first);
    struct Reset { ~Reset() { game::reset_video_backend(); } } reset;
    EXPECT_TRUE((game::get_video_backend(output) && output == &first)) << "registration borrows the object";
    const char filename[] = "movie.BIK";
    Game::PlayMovie(filename);
    EXPECT_TRUE((first.filename == filename && first.theme == -1 &&
        first.options == std::array<char, 4>{-1, -1, -1, -1})) << "original defaults and borrowed filename";
    EXPECT_TRUE((first.calls == 1 && first.returned)) << "dispatch completes before the caller continues";

    // Each byte value must survive independently, especially -1, 0, 1 and
    // non-boolean values. The filename is not prefiltered by the core wrapper.
    for (unsigned int value = 0; value < 256; ++value) {
        const std::array<char, 4> expected{static_cast<char>(value),
            static_cast<char>(value ^ 0x55), static_cast<char>(value ^ 0xAA),
            static_cast<char>(value ^ 0xFF)};
        Game::PlayMovie("movie.vqa", -2147483647 - 1,
            expected[0], expected[1], expected[2], expected[3]);
        EXPECT_TRUE((first.options == expected && first.theme == (-2147483647 - 1))) << "all four option bytes and theme bits survive";
    }

    const int previous_calls = first.calls;
    int destructions = 0;
    {
        Probe second;
        second.destructions = &destructions;
        game::set_video_backend(second);
        EXPECT_TRUE((game::get_video_backend(output) && output == &second)) << "backend replacement";
        Game::PlayMovie("missing.file", 2147483647, 0, 1, 2, -1);
        EXPECT_TRUE((second.calls == 1 && second.theme == 2147483647 &&
            first.calls == previous_calls)) << "replacement receives the next call";
        game::reset_video_backend();
        EXPECT_TRUE((destructions == 0)) << "reset does not delete the host";
        EXPECT_TRUE((!game::get_video_backend(output) && output == &second)) << "reset restores unavailable provider";
        Game::PlayMovie("missing.file");
        EXPECT_TRUE((second.calls == 1)) << "no stale dispatch after reset";
    }
    EXPECT_TRUE((destructions == 1)) << "host owns destruction";
}
}

