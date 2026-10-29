#include <gtest/gtest.h>
#include "presentation/frame_compiler.hpp"
#include "presentation/state.hpp"
#include "api/presentation.hpp"
#include "images/drawing_resources.hpp"
#include "yrpp/FileFormats/SHP.h"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace {
std::vector<std::uint8_t> residency_image() {
    std::vector<std::uint8_t> bytes(sizeof(SHPStruct) + sizeof(SHPFrame) + 4, 0);
    auto* shape = reinterpret_cast<SHPStruct*>(bytes.data());
    shape->Width = shape->Height = 2;
    shape->Frames = 1;
    auto& frame = *reinterpret_cast<SHPFrame*>(bytes.data() + sizeof(SHPStruct));
    frame.Width = frame.Height = 2;
    frame.Offset = sizeof(SHPStruct) + sizeof(SHPFrame);
    std::fill(bytes.begin() + frame.Offset, bytes.end(), 1);
    return bytes;
}
}

TEST(PresentationResidency, SequenceTablePressureRetainsQueuedFramesAndReclaimsIdleSequences) {
    // A single table entry forces real pressure without large synthetic assets.
    game::FrameCompiler compiler(false, 1);
    auto a = residency_image(), b = residency_image();
    auto* first = reinterpret_cast<SHPStruct*>(a.data());
    auto* second = reinterpret_cast<SHPStruct*>(b.data());
    auto queued = compiler.acquire_frame();
    queued->width = queued->height = 32;
    compiler.begin_frame(*queued);
    ASSERT_EQ(compiler.submit_shape_sequence(first, game::shape_sequence_color), game::DrawingStatus::drawn);
    ASSERT_TRUE(compiler.end_frame());
    auto next = compiler.acquire_frame();
    next->width = next->height = 32;
    compiler.begin_frame(*next);
    EXPECT_NE(compiler.submit_shape_sequence(second, game::shape_sequence_color), game::DrawingStatus::drawn);
    // Simulate the consumer completing the queued frame. Its table address is
    // unavailable until this ownership goes away.
    queued->resources.clear();
    queued->sequences.reset();
    queued->retired_sequences.clear();
    EXPECT_EQ(compiler.submit_shape_sequence(second, game::shape_sequence_color), game::DrawingStatus::drawn);
    ASSERT_TRUE(compiler.end_frame());
    compiler.clear();
    game::invalidate_shape_resource(first);
    game::invalidate_shape_resource(second);
}

TEST(PresentationResidency, PressureEvictsOnlyTheOldestIdleSequence) {
    game::FrameCompiler compiler(false, 2);
    auto a = residency_image(), b = residency_image(), c = residency_image();
    const auto load = [&](std::vector<std::uint8_t>& bytes) {
        auto frame = compiler.acquire_frame();
        frame->width = frame->height = 32;
        compiler.begin_frame(*frame);
        EXPECT_EQ(compiler.submit_shape_sequence(reinterpret_cast<SHPStruct*>(bytes.data()), game::shape_sequence_color),
                  game::DrawingStatus::drawn);
        EXPECT_TRUE(compiler.end_frame());
        std::weak_ptr<render::MapBufferData> resident;
        if (!frame->resources.empty()) resident = frame->resources.back();
        frame->resources.clear();
        frame->sequences.reset();
        frame->retired_sequences.clear();
        return resident;
    };
    const auto oldest = load(a), newer = load(b);
    EXPECT_FALSE(oldest.expired());
    EXPECT_FALSE(newer.expired());
    const auto newest = load(c);
    EXPECT_TRUE(oldest.expired());
    EXPECT_FALSE(newer.expired());
    EXPECT_FALSE(newest.expired());
    compiler.clear();
    for (auto* bytes : {&a, &b, &c})
        game::invalidate_shape_resource(reinterpret_cast<SHPStruct*>(bytes->data()));
}

TEST(PresentationResidency, WorldAndFrameCompilerStateHaveIndependentLifetimes) {
    game::PresentationState world;
    world.layer_visibility.assign(32, 1);
    game::PresentationHandle* compiler = nullptr;
    ASSERT_TRUE(game::create_presentation(compiler, false));
    game::PresentationFrameHandle* frame = nullptr;
    ASSERT_TRUE(game::begin_presentation_frame(*compiler, 1, 32, 32, frame));
    render::Frame output;
    ASSERT_TRUE(game::end_presentation_frame(*compiler, *frame, output));
    game::destroy_presentation(compiler);
    // The queued frame owns its compiler state independently of the map's
    // PresentationState, including destruction after the public handle is gone.
    game::release_presentation_frame(frame);
    EXPECT_EQ(world.layer_visibility, (std::vector<std::uint8_t>(32, 1)));
}
