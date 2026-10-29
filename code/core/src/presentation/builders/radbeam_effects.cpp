// YR 0x6591B0: geometry is evaluated on a local record, never on simulation objects.
#include "presentation/builders/radbeam.hpp"
#include "radbeam_state.hpp"
#include "surface_drawing.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include <bit>
namespace game {
void draw_radbeam_segment(void* user, const RadBeam& beam) {
    auto& ctx = *static_cast<PresentationContext*>(user);
    const CoordStruct from{std::bit_cast<int>(beam.unknown_9C), std::bit_cast<int>(beam.unknown_A0),
                           std::bit_cast<int>(beam.unknown_A4)};
    const CoordStruct to{std::bit_cast<int>(beam.unknown_A8), std::bit_cast<int>(beam.unknown_AC),
                         std::bit_cast<int>(beam.unknown_B0)};
    Point2D a, b;
    TacticalClass::Instance->CoordsToClient(&from, &a);
    TacticalClass::Instance->CoordsToClient(&to, &b);
    submit_color_line(ctx, DSurface::ViewBounds, a, b,
                     WORD(Drawing::RGB_To_Int(beam.unknown_C0, beam.unknown_C1, beam.unknown_C2)),
                     std::bit_cast<int>(beam.unknown_C) - TacticalClass::AdjustForZ(from.Z) - 2,
                     -2 - TacticalClass::AdjustForZ(to.Z));
}
void build_radbeams(PresentationContext& ctx) {
    for (int i = RadBeam::Array.Count - 1; i >= 0; --i) {
        auto working = *RadBeam::Array[i];
        advance_radbeam(working, draw_radbeam_segment, &ctx);
    }
}
}
