#include <gtest/gtest.h>
#include "draw_resource_pool.hpp"
#include <array>
#include <thread>

TEST(DrawResourcePool, QueuedVersionKeepsItsAddressAndBudget) {
    render::DrawResourcePool pool(16, 4);
    auto old = pool.allocate(8);
    const auto version = old->version;
    auto queued = old;
    old.reset();
    auto next = pool.allocate(8);
    EXPECT_NE(queued->first, next->first);
    EXPECT_THROW(pool.allocate(1), std::length_error);
    const auto first = queued->first;
    const auto slot = queued->slot;
    queued.reset();
    auto replacement = pool.allocate(8);
    EXPECT_EQ(replacement->first, first);
    EXPECT_EQ(replacement->slot, slot);
    EXPECT_GT(replacement->version, version);
}
TEST(DrawResourcePool, CoalescingReclaimsBothNeighbours) {
    render::DrawResourcePool pool(24, 3);
    auto a = pool.allocate(8), b = pool.allocate(8), c = pool.allocate(8);
    a.reset();
    c.reset();
    b.reset();
    auto full = pool.allocate(24);
    EXPECT_EQ(full->first, 0u);
    EXPECT_THROW(pool.allocate(1), std::length_error);
}
TEST(DrawResourcePool, DescriptorLimitAndRendererThreadRelease) {
    render::DrawResourcePool pool(1024, 2);
    auto a = pool.allocate(1), b = pool.allocate(1);
    EXPECT_THROW(pool.allocate(1), std::length_error);
    std::thread renderer([owned = std::move(a)]() mutable { owned.reset(); });
    renderer.join();
    auto c = pool.allocate(1022);
    EXPECT_EQ(c->count, 1022u);
}

TEST(DrawResourcePool, StatisticsDistinguishFragmentationAndPinnedRetirement) {
    render::DrawResourcePool pool(24, 3);
    auto a = pool.allocate(8), b = pool.allocate(8), c = pool.allocate(8);
    auto queued = a;
    a.reset();
    EXPECT_EQ(pool.statistics().releases, 0u);
    queued.reset();
    c.reset();
    const auto fragmented = pool.statistics();
    EXPECT_EQ(fragmented.free_words, 16u);
    EXPECT_EQ(fragmented.largest_hole, 8u);
    EXPECT_EQ(fragmented.free_slots, 2u);
    EXPECT_EQ(fragmented.releases, 2u);
    EXPECT_EQ(fragmented.released_words, 16u);
    EXPECT_THROW(pool.allocate(16), std::length_error);
    b.reset();
    EXPECT_EQ(pool.statistics().largest_hole, 24u);
    EXPECT_EQ(pool.allocate(24)->count, 24u);
}
