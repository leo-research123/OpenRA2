#include <gtest/gtest.h>
#include "point_grid.hpp"
#include <algorithm>
#include <cstdint>
#include <climits>
#include <random>
#include <vector>

TEST(PointGrid, RandomMovementDeletionAndReusedIdsMatchBruteForce) {
    game::PointGrid grid(5);
    std::mt19937 random(0xABC);
    struct Point {
        int x = 0, y = 0;
        bool live = false;
    };
    std::vector<Point> reference(10000);
    std::vector<std::uint32_t> actual, expected;
    for (unsigned step = 0; step < 30000; ++step) {
        const auto id = random() % reference.size();
        auto& p = reference[id];
        if (random() % 5 == 0) {
            p.live = false;
            grid.erase(id);
        } else {
            p = {int(random() % 20000) - 10000, int(random() % 20000) - 10000, true};
            grid.set(id, p.x, p.y);
        }
        if (step % 17) continue;
        const int x = int(random() % 20000) - 10000, y = int(random() % 20000) - 10000, w = random() % 2000,
                  h = random() % 2000;
        expected.clear();
        for (unsigned n = 0; n < reference.size(); ++n) {
            const auto& v = reference[n];
            if (v.live && v.x >= x && v.x < x + w && v.y >= y && v.y < y + h) expected.push_back(n);
        }
        grid.query(x, y, x + w, y + h, actual);
        std::sort(actual.begin(), actual.end());
        EXPECT_EQ(actual, expected) << step;
    }
    grid.set(0, INT32_MIN, INT32_MAX);
    grid.set(1, -33, -32);
    grid.set(2, -32, -33);
    grid.set(3, 32, 31);
    grid.query(INT32_MIN, INT32_MAX, std::int64_t(INT32_MIN) + 1, std::int64_t(INT32_MAX) + 1, actual);
    EXPECT_EQ(actual, (std::vector<std::uint32_t>{0}));
}
