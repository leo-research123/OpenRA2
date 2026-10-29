#include <gtest/gtest.h>
#include "voxel_drawing.hpp"

TEST(VoxelResource, CompositeHitsUseImmutableChildrenAndAllPlacementFields) {
    game::VoxelCompositeCache cache;
    auto child = std::make_shared<game::VoxelImageHandle>();
    child->width = child->height = 2;
    child->reference_pixels = {0x1000001, 0, 0x1000002, 0x1000003};
    game::VoxelImageHandle builder;
    builder.kind = game::VoxelImageKind::composite;
    builder.width = builder.height = 256;
    builder.children.push_back(child);
    builder.layers.push_back({child.get(), {1, 1}, {0, 0, 256, 256}});
    auto original = cache.get(builder, {0, 0, 4, 4});
    EXPECT_EQ(original->reference_pixels[5], 0x1000001u);
    EXPECT_EQ(original->reference_pixels[9], 0x1000002u);
    EXPECT_EQ(cache.get(builder, {0, 0, 4, 4}), original);
    game::VoxelImageHandle second;
    second.children = builder.children;
    second.layers = builder.layers;
    EXPECT_EQ(cache.get(second, {0, 0, 4, 4}), original);
    second.layers[0].position.X = 2;
    EXPECT_NE(cache.get(second, {0, 0, 4, 4}), original);
    second.layers = builder.layers;
    second.layers[0].clip = {0, 0, 1, 1};
    EXPECT_NE(cache.get(second, {0, 0, 4, 4}), original);
    EXPECT_NE(cache.get(builder, {1, 0, 4, 4}), original);
    auto replacement = std::make_shared<game::VoxelImageHandle>();
    replacement->width = replacement->height = 2;
    builder.children = {replacement};
    builder.layers[0].image = replacement.get();
    EXPECT_NE(cache.get(builder, {0, 0, 4, 4}), original);
    EXPECT_EQ(original->children.front(), child);
    EXPECT_EQ(original->reference_pixels[10], 0x1000003u);
}
TEST(VoxelResource, CompositeOrderAndCroppedIndexesArePreserved) {
    game::VoxelCompositeCache cache;
    game::VoxelImageHandle builder;
    auto a = std::make_shared<game::VoxelImageHandle>(), b = std::make_shared<game::VoxelImageHandle>();
    a->width = a->height = b->width = b->height = 1;
    a->reference_pixels = {0x1000001};
    b->reference_pixels = {0x1000002};
    builder.children = {a, b};
    builder.layers = {{a.get(), {4, 7}, {0, 0, 256, 256}}, {b.get(), {4, 7}, {0, 0, 256, 256}}};
    auto first = cache.get(builder, {4, 7, 1, 1});
    EXPECT_EQ(first->reference_pixels[0], 0x1000002u);
    std::swap(builder.layers[0], builder.layers[1]);
    auto reversed = cache.get(builder, {4, 7, 1, 1});
    EXPECT_NE(first, reversed);
    EXPECT_EQ(reversed->reference_pixels[0], 0x1000001u);
    cache.clear();
    EXPECT_EQ(first->reference_pixels[0], 0x1000002u);
}
