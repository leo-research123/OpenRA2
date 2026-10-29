#pragma once
// Owned presentation results of the render-phase UI state step. Original UI
// objects keep all input/animation state; these values only describe a picture.
#include "api/map_view.hpp"
#include "type_drawing_context.hpp"
#include <unordered_map>
#include <vector>
class TextLabelClass;
namespace game {
struct SidebarMoviePicture {
    std::vector<WORD> pixels;
    Point2D position{};
    int width = 0, height = 0, pitch = 0;
    std::uint64_t identity = 0, version = 0;
};
struct UiRenderState {
    SidebarMoviePicture movie;
    std::unordered_map<const TextLabelClass*, bool> label_caret_suppressed;
    RectangleStruct initialized_bounds{};
    int initialized_side = -1;
    bool initialized = false;
    DrawingStatus radar_status = DrawingStatus::skipped;
    bool radar_active = false, radar_names = false, radar_movie = false, radar_viewport = false;
    int radar_frame = 0;
};
bool ui_label_suppressed(const UiRenderState*, const TextLabelClass&) noexcept;
}
