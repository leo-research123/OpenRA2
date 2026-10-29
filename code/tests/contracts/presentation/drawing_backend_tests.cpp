#include <gtest/gtest.h>
#include "type_drawing_context.hpp"
#include "api/type_drawing.hpp"
#include "api/drawing_diagnostics.hpp"
#include "yrpp/FileFormats/SHP.h"
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <vector>
namespace {
struct Recorder {
    game::ShapeDrawingRequest shape{};
    game::TileDrawingRequest tile{};
    int shapes = 0, tiles = 0;
};
game::DrawingStatus shape(void* user, const game::ShapeDrawingRequest& request) {
    auto& state = *static_cast<Recorder*>(user);
    state.shape = request;
    ++state.shapes;
    return game::DrawingStatus::drawn;
}
TEST(TypeDrawing, LightingResourceAndRasterFallback) {
    using namespace game;
    std::vector<byte> storage(sizeof(SHPFile) + 4);
    auto* image = new (storage.data()) SHPFile;
    image->Frames = 1;
    image->FirstFrame = {};
    image->FirstFrame.Left = -2;
    image->FirstFrame.Top = 3;
    image->FirstFrame.Width = 2;
    image->FirstFrame.Height = 2;
    image->FirstFrame.Offset = sizeof(SHPFile);
    const byte samples[]{0, 127, 254, 255};
    std::memcpy(storage.data() + sizeof(SHPFile), samples, 4);
    TypeDrawingContext context;
    context.target = reinterpret_cast<DrawingTargetHandle*>(&context);
    LightingShapeDrawingRequest request{context.target, image, 0, {5, 7}, {0, 0, 20, 20}, RasterBlendMode::alpha_shape};
    context.backend.lighting_shape = [](void*, const LightingShapeDrawingRequest& r) {
        EXPECT_EQ(r.position.X, 5);
        EXPECT_EQ(r.position.Y, 7);
        EXPECT_EQ(r.frame, 0);
        EXPECT_EQ(r.operation, RasterBlendMode::alpha_shape);
        return DrawingStatus::drawn;
    };
    EXPECT_EQ(submit_type_lighting_shape(context, request), DrawingStatus::drawn);
    context.backend.lighting_shape = [](void*, const LightingShapeDrawingRequest&) -> DrawingStatus { throw 1; };
    EXPECT_EQ(submit_type_lighting_shape(context, request), DrawingStatus::backend_failure);
    context.backend.lighting_shape = nullptr;
    context.backend.raster = [](void*, const RasterDrawingRequest& r) {
        EXPECT_EQ(r.position.X, 3);
        EXPECT_EQ(r.position.Y, 10);
        EXPECT_EQ(r.pitch, 2);
        EXPECT_EQ(r.pixel_count, 4u);
        EXPECT_EQ(r.pixels[0], 0);
        EXPECT_EQ(r.pixels[1], 127);
        EXPECT_EQ(r.pixels[2], 254);
        EXPECT_EQ(r.pixels[3], 255);
        EXPECT_EQ(r.blend_mode, RasterBlendMode::alpha_shape);
        return DrawingStatus::drawn;
    };
    EXPECT_EQ(submit_type_lighting_shape(context, request), DrawingStatus::drawn);
    request.frame = 1;
    EXPECT_EQ(submit_type_lighting_shape(context, request), DrawingStatus::invalid_argument);
    request.frame = 0;
    request.operation = RasterBlendMode::copy;
    EXPECT_EQ(submit_type_lighting_shape(context, request), DrawingStatus::invalid_argument);
}

TEST(TypeDrawing, LightTestRejectsOldBackendAndNonPixelRequests) {
    using namespace game;
    TypeDrawingContext c;
    int identity = 0;
    c.target = reinterpret_cast<DrawingTargetHandle*>(&identity);
    c.backend.raster = [](void*, const RasterDrawingRequest&) { return DrawingStatus::drawn; };
    RasterDrawingRequest r;
    r.target = c.target;
    r.clip = {0, 0, 20, 20};
    r.width = r.height = 1;
    for (auto mode : {RasterBlendMode::light_nonzero, RasterBlendMode::light_zero}) {
        r.blend_mode = mode;
        EXPECT_EQ(submit_type_raster(c, r), DrawingStatus::drawn);
        c.backend.version = 16;
        EXPECT_EQ(submit_type_raster(c, r), DrawingStatus::unsupported);
        c.backend.version = TypeDrawingBackend{}.version;
        r.width = 2;
        EXPECT_EQ(submit_type_raster(c, r), DrawingStatus::invalid_argument);
        r.width = 1;
        r.pixels = &r.color;
        EXPECT_EQ(submit_type_raster(c, r), DrawingStatus::invalid_argument);
        r.pixels = nullptr;
    }
}

TEST(TypeDrawing, UiBatchFallbackAndFailureContracts) {
    using namespace game;
    Recorder recorder;
    TypeDrawingContext c;
    c.backend_context = &recorder;
    c.backend.shape = shape;
    c.target = reinterpret_cast<DrawingTargetHandle*>(&recorder);
    c.palette = reinterpret_cast<DrawingPaletteHandle*>(&recorder);
    SHPStruct image;
    ShapeStripDrawingRequest strip;
    strip.shape.target = c.target;
    strip.shape.palette = c.palette;
    strip.shape.image = &image;
    strip.shape.clip = {0, 0, 100, 100};
    strip.shape.position = {4, 5};
    strip.count = 3;
    strip.step = {0, 3};
    EXPECT_EQ(submit_type_shape_strip(c, strip), DrawingStatus::drawn);
    EXPECT_EQ(recorder.shapes, 3);
    EXPECT_EQ(recorder.shape.position.Y, 11);
    c.backend.shape_strip = [](void*, const ShapeStripDrawingRequest&) { return DrawingStatus::unsupported; };
    EXPECT_EQ(submit_type_shape_strip(c, strip), DrawingStatus::drawn);
    EXPECT_EQ(recorder.shapes, 6);
    c.backend.shape_strip = [](void*, const ShapeStripDrawingRequest&) -> DrawingStatus { throw std::bad_alloc(); };
    EXPECT_EQ(submit_type_shape_strip(c, strip), DrawingStatus::backend_failure);
    EXPECT_EQ(recorder.shapes, 6);
    strip.shape.flags = 0x800;
    EXPECT_EQ(submit_type_shape_strip(c, strip), DrawingStatus::unsupported);
    strip.shape.flags = 0;
    strip.shape.position.Y = INT32_MAX;
    EXPECT_EQ(submit_type_shape_strip(c, strip), DrawingStatus::invalid_argument);
    const std::uint8_t bits[] = {0x80};
    MaskDrawingRequest mask;
    mask.target = c.target;
    mask.bits = bits;
    mask.byte_count = 1;
    mask.width = mask.height = mask.pitch = 1;
    mask.clip = {0, 0, 10, 10};
    c.backend.mask = [](void*, const MaskDrawingRequest&) -> DrawingStatus { throw std::bad_alloc(); };
    EXPECT_EQ(submit_type_mask(c, mask), DrawingStatus::backend_failure);
    mask.byte_count = 0;
    EXPECT_EQ(submit_type_mask(c, mask), DrawingStatus::invalid_argument);
    c.backend.version = 25;
    EXPECT_EQ(submit_type_mask(c, mask), DrawingStatus::unsupported);
}

TEST(TypeDrawingDiagnostics, CopiesRejectedParametersAndKeepsFirstFailureUntilExplicitBegin) {
    static_assert(std::is_standard_layout_v<game::DrawingFailureInfo>);
    static_assert(std::is_trivially_copyable_v<game::DrawingFailureInfo>);
    game::begin_drawing_diagnostics();
    game::TypeDrawingContext context;
    context.backend.raster = [](void*, const game::RasterDrawingRequest&) { return game::DrawingStatus::drawn; };
    game::RasterDrawingRequest request;
    request.width = -17;
    request.height = 12;
    request.clip = {0, 0, 100, 100};
    EXPECT_EQ(game::submit_type_raster(context, request), game::DrawingStatus::invalid_argument);
    game::DrawingFailureInfo first;
    ASSERT_TRUE(game::get_drawing_failure(first));
    EXPECT_NE(first.id, 0u);
    EXPECT_NE(first.request, 0u);
    EXPECT_STREQ(first.stage, "submit_type_raster");
    EXPECT_NE(std::strstr(first.condition, "request.width < 0"), nullptr);
    EXPECT_NE(std::strstr(first.detail, "width=-17"), nullptr);
    EXPECT_NE(std::strstr(first.detail, "target=0x0"), nullptr);
    EXPECT_GT(first.line, 0u);
    request.width = 8193;
    game::submit_type_raster(context, request);
    game::record_drawing_diagnostic(game::DrawingStatus::backend_failure, "cleanup", "secondary", "later", __FILE__,
                                    __LINE__);
    game::DrawingFailureInfo after;
    ASSERT_TRUE(game::get_drawing_failure(after));
    EXPECT_EQ(after.id, first.id);
    EXPECT_STREQ(after.detail, first.detail);
    EXPECT_STREQ(after.condition, first.condition);
    game::begin_drawing_diagnostics();
    EXPECT_FALSE(game::get_drawing_failure(after));
    EXPECT_EQ(after.id, first.id);
}

TEST(TypeDrawingDiagnostics, BackendFailureAndFallbackNegotiationHaveDifferentContracts) {
    game::begin_drawing_diagnostics();
    game::TypeDrawingContext context;
    context.backend.mask = [](void*, const game::MaskDrawingRequest&) { return game::DrawingStatus::unsupported; };
    context.backend.raster = [](void*, const game::RasterDrawingRequest&) { return game::DrawingStatus::drawn; };
    std::uint8_t bits = 0x80;
    int target;
    game::MaskDrawingRequest request;
    request.target = reinterpret_cast<game::DrawingTargetHandle*>(&target);
    request.width = request.height = request.pitch = request.byte_count = 1;
    request.bits = &bits;
    request.clip = {0, 0, 10, 10};
    EXPECT_EQ(game::submit_type_mask(context, request), game::DrawingStatus::drawn);
    game::DrawingFailureInfo failure;
    EXPECT_FALSE(game::get_drawing_failure(failure));
    context.backend.raster = [](void*, const game::RasterDrawingRequest&) -> game::DrawingStatus {
        throw std::runtime_error("injected backend failure");
    };
    EXPECT_EQ(game::submit_type_mask(context, request), game::DrawingStatus::backend_failure);
    ASSERT_TRUE(game::get_drawing_failure(failure));
    EXPECT_STREQ(failure.stage, "submit_type_raster");
    EXPECT_NE(std::strstr(failure.condition, "exception"), nullptr);
    game::begin_drawing_diagnostics();
}
}
