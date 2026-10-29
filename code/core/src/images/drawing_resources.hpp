#pragma once
#include "api/drawing_resources.hpp"
namespace game {
void invalidate_shape_resource(const SHPStruct*) noexcept;
std::uint64_t allocate_raster_content_identity() noexcept;
}
