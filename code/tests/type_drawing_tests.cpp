#include "support/test_support.hpp"
#include "api/type_drawing.hpp"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

struct Recorder { game::ShapeDrawingRequest shape{}; game::TileDrawingRequest tile{}; int shapes = 0, tiles = 0; };
struct World { const game::DrawingPaletteHandle* palette; int intensity = -123; int calls = 0; };
game::DrawingStatus shape(void* user, const game::ShapeDrawingRequest& request) {
    auto& state = *static_cast<Recorder*>(user); state.shape = request; ++state.shapes;
    return game::DrawingStatus::drawn;
}
game::DrawingStatus tile(void* user, const game::TileDrawingRequest& request) {
    auto& state = *static_cast<Recorder*>(user); state.tile = request; ++state.tiles;
    return game::DrawingStatus::drawn;
}
game::DrawingStatus cell(void* user, const CellStruct& coordinates,
        const game::DrawingPaletteHandle*& palette, int& intensity) {
    auto& world = *static_cast<World*>(user); ++world.calls;
    EXPECT_TRUE((coordinates == CellStruct{4, 7})) << "real world coordinate forwarding";
    palette = world.palette; intensity = world.intensity; return game::DrawingStatus::drawn;
}
TEST(TypeDrawing, Contracts) {
    using game::DrawingStatus;
    int target_identity = 0, palette_identity = 0, cell_palette_identity = 0;
    Recorder recorder;
    game::TypeDrawingContext context;
    context.backend.shape = shape;
    context.backend.tile = tile;
    context.backend_context = &recorder;
    context.target = reinterpret_cast<game::DrawingTargetHandle*>(&target_identity);
    context.palette = reinterpret_cast<const game::DrawingPaletteHandle*>(&palette_identity);
    context.overlay_offset = {7, -3};
    World world{reinterpret_cast<const game::DrawingPaletteHandle*>(&cell_palette_identity)};
    context.world_context = &world; context.smudge_cell = cell;
    SHPStruct image; image.Width = 60; image.Height = 30; image.Frames = 2;
    OverlayTypeClass overlay("draw-overlay"); overlay.Image = &image;
    RectangleStruct clip{1, 2, 200, 100}; Point2D point{10, 20};
    EXPECT_TRUE((overlay.Draw(context, point, clip, 1) == DrawingStatus::drawn)) << "overlay request";
    EXPECT_TRUE((recorder.shape.position == Point2D{47, 32} && recorder.shape.flags == 0x600 &&
        recorder.shape.frame == 1 && recorder.shape.intensity == 1000 && recorder.shape.palette == context.palette)) << "overlay source parameters";
    EXPECT_TRUE((overlay.Draw(context, {std::numeric_limits<int>::max(), 0}, clip, 0) == DrawingStatus::invalid_argument)) << "overlay coordinate overflow rejected";
    overlay.Image = nullptr; overlay.ImageLoaded = true;
    EXPECT_TRUE((overlay.Draw(context, point, clip, 0) == DrawingStatus::unavailable)) << "unmigrated demand loader is explicit";
    overlay.ImageLoaded = false;
    EXPECT_TRUE((overlay.Draw(context, point, clip, 0) == DrawingStatus::skipped)) << "absent shape is a legal no-op";
    overlay.Image = &image;
    SmudgeTypeClass smudge("draw-smudge"); smudge.Width = 3;
    // This class owns Image in its real destructor, independently of the flag.
    auto* storage = YRMemory::Allocate(sizeof(SHPStruct)); EXPECT_TRUE((storage != nullptr)) << "shape allocation";
    smudge.Image = new (storage) SHPStruct; smudge.Image->Frames = 1;
    EXPECT_TRUE((smudge.DrawIt(context, point, clip, 5, 0, {4, 7}) == DrawingStatus::drawn)) << "smudge request";
    EXPECT_TRUE((recorder.shape.position == Point2D{-20, -25} && recorder.shape.flags == 0xE00 &&
        recorder.shape.depth_adjustment == -1 && recorder.shape.palette == world.palette &&
        recorder.shape.intensity == -123)) << "smudge coordinates/light/palette/depth";
    EXPECT_TRUE((smudge.DrawIt(context, point, clip, 0, 1, {4, 7}) == DrawingStatus::unavailable)) << "unknown nonzero-height projection is not guessed";
    context.height_to_pixels = [](void*, int height, int& pixels) {
        pixels = height * 2; return DrawingStatus::drawn; // Test input, not an asserted original constant.
    };
    EXPECT_TRUE((smudge.DrawIt(context, point, clip, 0, 4, {4, 7}) == DrawingStatus::drawn &&
        recorder.shape.depth_adjustment == -9)) << "supplied host projection";
    std::vector<byte> bytes(sizeof(TMPStruct) + sizeof(int) + sizeof(TMPImage) + 900);
    const int columns = 1, rows = 1, width = 60, height = 30;
    std::memcpy(bytes.data(), &columns, 4); std::memcpy(bytes.data()+4, &rows, 4);
    std::memcpy(bytes.data()+8, &width, 4); std::memcpy(bytes.data()+12, &height, 4);
    const int offset = 20; std::memcpy(bytes.data()+16, &offset, 4);
    IsometricTileTypeClass iso(0, 0, 0, "tile", 0);
    iso.Image = reinterpret_cast<SHPStruct*>(bytes.data());
    EXPECT_TRUE((iso.DrawTMP(context, 3, 5, 6, clip, 2, 777, true, 0, false, true, false, 9) == DrawingStatus::drawn)) << "TMP needs no software renderer";
    EXPECT_TRUE((recorder.tile.sub_tile == 0 && recorder.tile.position == Point2D{5, 6} && recorder.tile.level == 2 &&
        recorder.tile.use_depth && recorder.tile.flag16 && recorder.tile.intensity == 777)) << "TMP semantic request";
    context.backend.shape = nullptr;
    EXPECT_TRUE((overlay.Draw(context, point, clip, 0) == DrawingStatus::unavailable)) << "missing backend";
    context.backend.shape = [](void*, const game::ShapeDrawingRequest&) { return DrawingStatus::unsupported; };
    struct Call { OverlayTypeClass* type; Point2D point; RectangleStruct clip; } call{&overlay, point, clip};
    EXPECT_TRUE((game::with_type_drawing(context, [](void* user) {
        auto& value = *static_cast<Call*>(user); value.type->Draw(&value.point, &value.clip, 0);
    }, &call) == DrawingStatus::unsupported)) << "legacy binding never retries an unsupported mode through EXE";
    context.backend.shape = [](void*, const game::ShapeDrawingRequest&) -> DrawingStatus { throw std::runtime_error("backend"); };
    EXPECT_TRUE((overlay.Draw(context, point, clip, 0) == DrawingStatus::backend_failure)) << "same-runtime exception containment";
    context.backend.shape = [](void*, const game::ShapeDrawingRequest&) { return static_cast<DrawingStatus>(999); };
    EXPECT_TRUE((overlay.Draw(context, point, clip, 0) == DrawingStatus::backend_failure)) << "invalid callback status";
    game::RasterDrawingRequest raster;
    raster.target=context.target; raster.position={-3,4}; raster.clip={0,0,20,30};
    raster.width=5; raster.height=2; raster.pitch=7;
    std::uint16_t pixels[12]{}; raster.pixels=pixels; raster.pixel_count=12;
    int rasters=0; context.backend_context=&rasters;
    context.backend.raster=[](void* pointer,const game::RasterDrawingRequest& r) {
        ++*static_cast<int*>(pointer);
        EXPECT_TRUE((r.position.X==-3 && r.pitch==7 && r.clip.Height==30)) << "generic raster preserves coordinates, pitch and clip";
        return DrawingStatus::drawn;
    };
    EXPECT_TRUE((game::submit_type_raster(context,raster)==DrawingStatus::drawn && rasters==1)) << "valid strided raster";
    raster.pixel_count=11;
    EXPECT_TRUE((game::submit_type_raster(context,raster)==DrawingStatus::invalid_argument && rasters==1)) << "last raster row capacity validated before callback";
    raster.pixels=nullptr; raster.color=0xffff;
    EXPECT_TRUE((game::submit_type_raster(context,raster)==DrawingStatus::drawn)) << "solid raster needs no pixel buffer";
    context.backend.raster=[](void*,const game::RasterDrawingRequest&) -> DrawingStatus { throw 1; };
    EXPECT_TRUE((game::submit_type_raster(context,raster)==DrawingStatus::backend_failure)) << "raster exception contained at ABI";
    context.backend.version = 7;
    EXPECT_TRUE((overlay.Draw(context, point, clip, 0) == DrawingStatus::unsupported)) << "old indexed ABI rejects the new voxel flags layout";
    context.backend.version = 8;
    EXPECT_EQ(overlay.Draw(context, point, clip, 0), DrawingStatus::unsupported) << "pre-tint SHP request ABI";
    context.backend.version = 9;
    EXPECT_EQ(overlay.Draw(context, point, clip, 0), DrawingStatus::unsupported) << "pre-tint indexed request ABI";
    context.backend.version = 10;
    EXPECT_EQ(overlay.Draw(context, point, clip, 0), DrawingStatus::unsupported) << "pre-ABuffer raster operations";
    context.backend.version = 11;
    EXPECT_EQ(overlay.Draw(context, point, clip, 0), DrawingStatus::unsupported) << "pre-resource lighting callback ABI";
    context.backend.version = 999;
    EXPECT_TRUE((overlay.Draw(context, point, clip, 0) == DrawingStatus::unsupported)) << "version check";
    iso.Image = nullptr; // Borrowed fixture storage outlives no deferred command.
}
TEST(TypeDrawing, LightingResourceAndRasterFallback) {
    using namespace game;
    std::vector<byte> storage(sizeof(SHPFile)+4);
    auto* image=new(storage.data()) SHPFile;
    image->Frames=1;
    image->FirstFrame={};
    image->FirstFrame.Left=-2;image->FirstFrame.Top=3;
    image->FirstFrame.Width=2;image->FirstFrame.Height=2;
    image->FirstFrame.Offset=sizeof(SHPFile);
    const byte samples[]{0,127,254,255};
    std::memcpy(storage.data()+sizeof(SHPFile),samples,4);
    TypeDrawingContext context;
    context.target=reinterpret_cast<DrawingTargetHandle*>(&context);
    LightingShapeDrawingRequest request{context.target,image,0,{5,7},{0,0,20,20},RasterBlendMode::alpha_shape};
    context.backend.lighting_shape=[](void*,const LightingShapeDrawingRequest& r) {
        EXPECT_EQ(r.position.X,5);EXPECT_EQ(r.position.Y,7);EXPECT_EQ(r.frame,0);
        EXPECT_EQ(r.operation,RasterBlendMode::alpha_shape);
        return DrawingStatus::drawn;
    };
    EXPECT_EQ(submit_type_lighting_shape(context,request),DrawingStatus::drawn);
    context.backend.lighting_shape=[](void*,const LightingShapeDrawingRequest&) -> DrawingStatus {throw 1;};
    EXPECT_EQ(submit_type_lighting_shape(context,request),DrawingStatus::backend_failure);
    context.backend.lighting_shape=nullptr;
    context.backend.raster=[](void*,const RasterDrawingRequest& r) {
        EXPECT_EQ(r.position.X,3);EXPECT_EQ(r.position.Y,10);
        EXPECT_EQ(r.pitch,2);EXPECT_EQ(r.pixel_count,4u);
        EXPECT_EQ(r.pixels[0],0);EXPECT_EQ(r.pixels[1],127);
        EXPECT_EQ(r.pixels[2],254);EXPECT_EQ(r.pixels[3],255);
        EXPECT_EQ(r.blend_mode,RasterBlendMode::alpha_shape);
        return DrawingStatus::drawn;
    };
    EXPECT_EQ(submit_type_lighting_shape(context,request),DrawingStatus::drawn);
    request.frame=1;
    EXPECT_EQ(submit_type_lighting_shape(context,request),DrawingStatus::invalid_argument);
    request.frame=0;request.operation=RasterBlendMode::copy;
    EXPECT_EQ(submit_type_lighting_shape(context,request),DrawingStatus::invalid_argument);
}
}
