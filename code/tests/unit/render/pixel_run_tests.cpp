#include <gtest/gtest.h>
#include "pixel_batch.hpp"

TEST(PixelRun, PreservesOrderedClippedBinMembershipAndResourcePrefix) {
    const std::vector<render::PixelParameters> source{{96, 64, 40, 40, -9, -5, 0, 0, 96, 64},
                                                    {96, 64, 50, 30, 25, 17, 31, 20, 33, 40},
                                                    {96, 64, 10, 10, 200, 200, 0, 0, 96, 64},
                                                    {96, 64, 1, 1, 95, 63, 0, 0, 96, 64}};
    auto words = source;
    std::vector<std::int32_t> schedule{11, 12, 21, 22, 31, 32, 41, 42, 0, 0};
    render::PixelRunScratch scratch;
    const auto result = render::append_pixel_run(words, 0, words.size(), schedule, scratch);
    EXPECT_EQ(result.header, 4);
    EXPECT_EQ(schedule[0], 11);
    EXPECT_EQ(schedule[7], 42);
    const auto& h = words.back();
    EXPECT_EQ(h[19], 13);
    for (int y = 0; y < h[3]; ++y)
        for (int x = 0; x < h[2]; ++x) {
            std::vector<int> expected, actual;
            const int left = (h[5] + x) * 32, top = (h[6] + y) * 32;
            for (int i = 0; i < int(source.size()); ++i) {
                const auto& p = source[i];
                bool touches = false;
                for (int py = top; py < top + 32; ++py)
                    for (int px = left; px < left + 32; ++px)
                        touches |= px >= 0 && py >= 0 && px < p[0] && py < p[1] && px >= p[4] && py >= p[5] &&
                                   px < p[4] + p[2] && py < p[5] + p[3] && px >= p[6] && py >= p[7] &&
                                   px < p[6] + p[8] && py < p[7] + p[9];
                if (touches) expected.push_back(i);
            }
            const int entry = h[4] + (y * h[2] + x) * 2;
            for (int n = 0; n < schedule[entry + 1]; ++n)
                actual.push_back(schedule[schedule[entry] + n]);
            EXPECT_EQ(actual, expected);
        }
}
TEST(PixelRun, EmptyRunDoesNotDispatchOrOverwriteResourceOffsets) {
    std::vector<render::PixelParameters> words(2, render::PixelParameters{32, 32, 3, 3, -10, -10, 0, 0, 32, 32});
    std::vector<std::int32_t> schedule{7, 8, 9, 10, 0, 0};
    render::PixelRunScratch scratch;
    const auto run = render::append_pixel_run(words, 0, 2, schedule, scratch);
    EXPECT_EQ(run.columns, 0);
    EXPECT_EQ(run.rows, 0);
    EXPECT_EQ(schedule.size(), 6);
    EXPECT_EQ(schedule[3], 10);
}
