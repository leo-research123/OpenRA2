#include <gtest/gtest.h>
#include "frame_diagnostics.hpp"
#include "type_drawing_packets.hpp"
#include "yrpp/FileFormats/SHP.h"
#include <cstring>

TEST(FrameDiagnostics, SeparatesCoreFailureFromFinalizationAndKeepsFirstCause) {
    using game::DrawingStatus;
    for (const bool core_ok : {false, true})
        for (const bool end_ok : {false, true}) {
            game::begin_drawing_diagnostics();
            const auto status = core_ok ? DrawingStatus::drawn : DrawingStatus::invalid_argument;
            if (!core_ok)
                game::record_drawing_diagnostic(status, "indexed", "invalid_depth",
                                                "absolute_depth=-1 expected=[0,65535]", __FILE__, __LINE__);
            game::record_core_frame_result(status);
            if (!end_ok)
                game::record_drawing_diagnostic(DrawingStatus::backend_failure, "bridge_end_frame",
                                                "injected_exception", "end failure", __FILE__, __LINE__);
            game::RenderFailureContext context;
            game::DrawingFailureInfo failure;
            EXPECT_EQ(game::finish_frame_diagnostics(status, end_ok, end_ok ? "" : "end failure", context, failure),
                      core_ok && end_ok);
            if (core_ok && end_ok) {
                EXPECT_FALSE(game::get_drawing_failure(failure));
                continue;
            }
            EXPECT_EQ(context.core, core_ok ? 1u : 2u);
            EXPECT_EQ(context.end_frame, end_ok ? 1u : 2u);
            EXPECT_EQ(context.enqueue, 0u);
            EXPECT_EQ(context.gpu_submit, 0u);
            EXPECT_EQ(context.gpu_complete, 0u);
            EXPECT_STREQ(failure.stage, core_ok ? "bridge_end_frame" : "indexed");
            EXPECT_STREQ(context.bridge_error, end_ok ? "" : "end failure");
        }
    game::begin_drawing_diagnostics();
}

TEST(FrameDiagnostics, ShapeFrameRejectionRecordsActualFrameAndResourceLimit) {
    game::begin_drawing_diagnostics();
    SHPStruct image;
    image.Frames = 2;
    game::ShapeDrawingRequest request;
    request.image = &image;
    request.frame = 7;
    game::TypeShapeMeta output;
    EXPECT_EQ(game::describe_type_shape(request, output), game::DrawingStatus::invalid_argument);
    game::DrawingFailureInfo failure;
    ASSERT_TRUE(game::get_drawing_failure(failure));
    EXPECT_STREQ(failure.condition, "shape_frame_out_of_range");
    EXPECT_NE(std::strstr(failure.detail, "frame=7 expected_frame=[0,2)"), nullptr);
    game::begin_drawing_diagnostics();
}
