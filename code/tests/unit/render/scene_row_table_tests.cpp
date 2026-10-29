#include <gtest/gtest.h>
#include "scene_row_table.hpp"
#include "scene_page_cache.hpp"
#include <map>
#include <vector>

TEST(SceneRowTable, QueuedSnapshotsAndCoalescedPublications) {
    using Table = render::SceneRowTable<int>;
    Table current;
    std::map<Table::Key, Table::Value> expected;
    std::uint32_t random = 0x12345678;
    for (int transaction = 0; transaction < 100; ++transaction) {
        const auto queued = current;
        const auto old = expected;
        for (int change = 0; change < 20; ++change) {
            random = random * 1664525u + 1013904223u;
            const Table::Key key{int(random % 4), int((random >> 8) % 512) - 256};
            if (random & 4) {
                auto value = std::make_shared<int>(transaction * 20 + change);
                current.set(key, value);
                expected[key] = value;
            } else {
                current.erase(key);
                expected.erase(key);
            }
        }
        std::map<Table::Key, Table::Value> captured, previous;
        current.visit([&](auto key, const auto& value) { captured.emplace(key, value); });
        queued.visit([&](auto key, const auto& value) { previous.emplace(key, value); });
        EXPECT_EQ(captured, expected);
        EXPECT_EQ(previous, old);
        std::map<Table::Key, Table::Value> applied = old;
        current.changes(&queued, [&](auto key, const auto& before, const auto& after) {
            const auto found = old.find(key);
            EXPECT_EQ(before, found == old.end() ? Table::Value{} : found->second);
            if (after)
                applied[key] = after;
            else
                applied.erase(key);
        });
        EXPECT_EQ(applied, expected);
        int unchanged = 0;
        current.changes(&current, [&](auto&&...) { ++unchanged; });
        EXPECT_EQ(unchanged, 0);
    }
}
TEST(SceneRowTable, ChangeVisitsOnlyReplacedRows) {
    render::SceneRowTable<int> before;
    for (int i = 0; i < 2000; ++i)
        before.set({1, i}, std::make_shared<int>(i));
    auto after = before;
    auto value = std::make_shared<int>(1234);
    after.set({1, 500}, value);
    int count = 0;
    after.changes(&before, [&](auto key, const auto& old, const auto& next) {
        ++count;
        EXPECT_EQ(key, std::make_pair(1, 500));
        EXPECT_EQ(*old, 500);
        EXPECT_EQ(next, value);
    });
    EXPECT_EQ(count, 1);
    EXPECT_EQ(**before.get({1, 500}), 500);
}
TEST(ScenePageCache, RejectsViewDependentDepthAndDestinationBlends) {
    std::array<std::int32_t, 12> w{};
    w[2] = 60 | (29 << 16);
    w[5] = 0x8000;
    w[6] = 1;
    EXPECT_TRUE(render::scene_page_cacheable(w, 1080));
    for (int gradient = 0; gradient < 3; ++gradient) {
        w[6] = 0x10001;
        w[7] = (gradient + 1) << 11;
        EXPECT_FALSE(render::scene_page_cacheable(w, 1080));
    }
    for (int blend = 1; blend < 5; ++blend) {
        w[6] = 1 | (blend << 8);
        EXPECT_FALSE(render::scene_page_cacheable(w, 1080));
    }
}
TEST(ScenePageCache, WrappedDepthProofIncludesViewportAndSourceEdges) {
    for (int height : {1, 29, 127, 1024})
        for (int viewport : {1, 1080, 8192}) {
            std::array<std::int32_t, 12> w{};
            w[2] = 60 | (height << 16);
            w[4] = -700;
            w[6] = 1;
            w[9] = 1;
            for (int base : {viewport - 2, viewport - 1, 0x8000, 0xFFFF - height - 254, 0xFFFF - height - 253}) {
                w[5] = base - w[4];
                const bool allowed = render::scene_page_cacheable(w, viewport);
                bool equivalent = true;
                for (int r : {0, height - 1})
                    for (int t : {0, viewport - 1})
                        for (int z : {0, 255}) {
                            const int camera = w[4] + r - t;
                            const int original = int(std::uint16_t(w[5] + camera + z)) - camera;
                            equivalent &= original == w[5] + z;
                        }
                EXPECT_EQ(allowed, equivalent);
            }
        }
}
