// YR LineTrail 0x556A20..0x556DF0. No corresponding class exists in the
// fixed OpenTS 44fac744 baseline. Draw 0x556C00 uses DSurface 0x4BEAC0.
#include "yrpp/LineTrail.h"
#include "presentation/builders/line_trail.hpp"
#include "surface_drawing.hpp"
#include "yrpp/ObjectClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Memory.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include <algorithm>
#include <bit>

namespace game {
void build_line_trail(PresentationContext& ctx, const LineTrail& self) {
    int first = self.ActiveSlot;
    if (self.Trails[first].Position == CoordStruct::Empty) return;
    for (int next = (first + 1) % 32; next != self.ActiveSlot; next = (next + 1) % 32) {
        const auto& a = self.Trails[first];
        const auto& b = self.Trails[next];
        if (!a.Value || b.Position == CoordStruct::Empty) break;
        Point2D start{}, end{};
        TacticalClass::Instance->CoordsToClient(&a.Position, &start);
        TacticalClass::Instance->CoordsToClient(&b.Position, &end);
        // Headless map views have Tactical bounds without the UI surfaces.
        const auto bounds = DSurface::ViewBounds.Width > 0 ? DSurface::ViewBounds : TacticalClass::ViewBounds;
        game::submit_blended_line(ctx, bounds, start, end, self.Color, a.Value, -2 - TacticalClass::AdjustForZ(a.Position.Z),
                                  -2 - TacticalClass::AdjustForZ(b.Position.Z));
        first = next;
    }
}
void build_line_trails(PresentationContext& ctx) { for (int i=LineTrail::Array.Count-1;i>=0;--i) build_line_trail(ctx,*LineTrail::Array[i]); }
}
