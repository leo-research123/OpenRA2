#pragma once
// Private host metadata. Pixel operations remain in the GPU shader.
#include "api/type_drawing.hpp"
#include <array>
#include <vector>
namespace game {
using LightingParameters = std::array<std::int32_t,20>;
struct LightingImageData {
    RectangleStruct bounds{};
    std::vector<std::uint32_t> texels;
};
DrawingStatus decode_lighting_image(SHPStruct*,int,LightingImageData&) noexcept;
DrawingStatus prepare_lighting_parameters(const LightingShapeDrawingRequest&,const RectangleStruct& image_bounds,
    int width,int height,LightingParameters&) noexcept;
inline constexpr int LightingBinSize=32;
// Appends offset/count pairs followed by ordered packet indices. Only the
// native, nonrotating lighting target is batched; legacy packets stay separate.
int append_lighting_bins(const LightingParameters*,int first,int count,int width,int height,
    std::vector<std::int32_t>& bins);
}
