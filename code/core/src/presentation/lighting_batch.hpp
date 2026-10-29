#pragma once
// Private host metadata. Pixel operations remain in the GPU shader.
#include "api/type_drawing.hpp"
#include "frame_plan.hpp"
#include <array>
#include <vector>
namespace game {
using LightingParameters = std::array<std::int32_t, 20>;
struct LightingImageData {
    RectangleStruct bounds{};
    std::vector<std::uint32_t> texels;
};
DrawingStatus decode_lighting_image(SHPStruct*, int, LightingImageData&) noexcept;
DrawingStatus prepare_lighting_parameters(const LightingShapeDrawingRequest&, const RectangleStruct& image_bounds,
                                          int width, int height, LightingParameters&) noexcept;
inline constexpr int LightingBinSize = 32;
// Appends offset/count pairs followed by ordered packet indices. Only the
// native, nonrotating lighting target is batched; legacy packets stay separate.
using render::append_lighting_bins;

// Per-pixel resolve of one lighting batch (map_light_field.glsl): a grid of
// shroud cells, each drawn once inside its own 60x30 footprint, followed by
// other producers handled like batched lighting packets. A pixel lies in at
// most two footprints, one per iso row, so it reads at most two cells. The
// planner accepts only batches with that structure; any other batch is
// submitted request by request, which is what the resolve must equal.
struct LightFieldPlan {
    int kx = 0, ky = 0;                         // footprint of cell (a,s): (kx + 30a, ky + 15s)
    int a0 = 0, s0 = 0, columns = 0, rows = 0;  // a = x - y, s = x + y
    std::vector<std::int32_t> grid;             // rows * columns cell entry indices, or -1
    // Per cell entry: clip x, y, width, height, position x, y, frame.
    std::vector<std::int32_t> cells;
    SHPStruct* cell_image = nullptr;
    int frames = 0;                 // one past the highest frame used
    std::uint32_t first_other = 0;  // entries [first_other, count) follow the grid
};
inline constexpr int LightFieldCellWords = 7;
// Returns false when the batch does not have the grid structure.
bool plan_light_field(const LightingShapeBatchRequest&, LightFieldPlan&) noexcept;
}
