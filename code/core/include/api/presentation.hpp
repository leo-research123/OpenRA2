#pragma once
#include "api/map_view.hpp"
#include "render/frame.hpp"
namespace game {
struct PresentationHandle;
struct PresentationFrameHandle;
// Frames retain copied resources independently of the world and the compiler.
// All entries are synchronous and contain exceptions on this side of the ABI.
// Reference mode retains software SHP/TMP/pixel requests instead of GPU-only work.
bool create_presentation(PresentationHandle*&, bool software_reference = false) noexcept;
void destroy_presentation(PresentationHandle*) noexcept;
void clear_presentation(PresentationHandle&) noexcept;
bool begin_presentation_frame(PresentationHandle&, std::uint64_t generation, int width, int height,
                               PresentationFrameHandle*&) noexcept;
DrawingStatus advance_presentation_frame(MapViewHandle&, PresentationFrameHandle&, double seconds,
                                          MapDrawStatistics&, bool& rendered) noexcept;
bool end_presentation_frame(PresentationHandle&, PresentationFrameHandle&, render::Frame&) noexcept;
void release_presentation_frame(PresentationFrameHandle*) noexcept;
const char* presentation_error(const PresentationHandle&) noexcept;
struct PresentationStatistics {
    std::uint64_t decoded_tiles = 0, prepared_palettes = 0, decoded_lighting = 0, shape_sequences = 0;
};
bool presentation_statistics(const PresentationHandle&, PresentationStatistics&) noexcept;
}
