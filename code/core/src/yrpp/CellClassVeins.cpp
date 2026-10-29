// Vein placement, YR 0x00485390 / 0x00485460 / 0x00485AF0.
#include "yrpp/CellClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include <bit>
#include <cstdint>

namespace {
constexpr int Veins = 126;
constexpr unsigned char FullVeins = 0x30;
bool veins_type(int overlay) { return OverlayTypeClass::Array[overlay]->IsVeins; }
bool vein_free_land(LandType land) {
    return land != LandType::Water && land != LandType::Rock && land != LandType::Ice && land != LandType::Beach;
}
// abs() of the scenario's next random number; abs(INT_MIN) wraps as on x86.
int random_magnitude() {
    const int value = ScenarioClass::Instance->Random.Random();
    return std::bit_cast<std::int32_t>(value < 0 ? 0u - static_cast<std::uint32_t>(value)
                                                 : static_cast<std::uint32_t>(value));
}
int wrapping_abs(int value) {
    return std::bit_cast<std::int32_t>(value < 0 ? 0u - static_cast<std::uint32_t>(value)
                                                 : static_cast<std::uint32_t>(value));
}
// The original queues the cell's shape and containing rectangles, in view
// coordinates, for the cached background redraw.
void register_redraw(CellClass& cell) {
    RectangleStruct shape, containing;
    cell.ShapeRect(&shape);
    cell.GetContainingRect(&containing);
    auto area = Drawing::Union(shape, containing);
    if (auto* tactical = TacticalClass::Instance) {
        area.X -= DSurface::ViewBounds.X;
        area.Y -= DSurface::ViewBounds.Y;
        tactical->RegisterDirtyArea(area, false);
    }
}
}

bool CellClass::CanHaveVeins() const {
    if (SlopeIndex > 4 || !vein_free_land(LandType)) return false;
    if (OverlayTypeIndex != -1 && !veins_type(OverlayTypeIndex)) return false;
    for (int direction = 0; direction < 8; direction += 2) {
        const auto* neighbour = GetNeighbourCell(static_cast<FacingType>(direction));
        if (neighbour->SlopeIndex > 4 && !SlopeIndex &&
            (neighbour->OverlayTypeIndex == -1 || !veins_type(neighbour->OverlayTypeIndex)))
            return false;
        if (!vein_free_land(neighbour->LandType)) return false;
        if (neighbour->OverlayTypeIndex != -1 && !veins_type(neighbour->OverlayTypeIndex)) return false;
    }
    return true;
}

int CellClass::VeinConnections() const {
    int connections = 0;
    for (int i = 0; i < 4; ++i) {
        const auto* neighbour = GetNeighbourCell(static_cast<FacingType>(2 * i));
        const int overlay = neighbour->OverlayTypeIndex;
        if (overlay == -1 || (neighbour->SlopeIndex && !veins_type(overlay))) continue;
        if (overlay == Veins ? neighbour->OverlayData >= FullVeins : veins_type(overlay)) connections |= 1 << i;
    }
    return connections;
}

void CellClass::PlaceVeins() {
    if (!CanHaveVeins()) return;
    auto& radar = RadarClass::Instance;
    radar.RadarBackground(MapCoords);
    OverlayTypeIndex = Veins;
    if (SlopeIndex) {
        OverlayData = static_cast<unsigned char>(2 * SlopeIndex + (random_magnitude() & 1) + 51);
    } else {
        OverlayData = static_cast<unsigned char>(random_magnitude() % 3 + FullVeins);
        for (int direction = 0; direction < 8; direction += 2) {
            auto* neighbour = GetNeighbourCell(static_cast<FacingType>(direction));
            const int overlay = neighbour->OverlayTypeIndex;
            const bool refit =
                overlay == Veins ? neighbour->OverlayData < FullVeins : overlay == -1 || !veins_type(overlay);
            if (!refit) continue;
            if (neighbour->SlopeIndex) {
                neighbour->OverlayTypeIndex = Veins;
                neighbour->OverlayData =
                    static_cast<unsigned char>(2 * neighbour->SlopeIndex + (random_magnitude() & 1) + 51);
                register_redraw(*neighbour);
            } else {
                const int connections = neighbour->VeinConnections();
                if (neighbour->OverlayTypeIndex == -1 ||
                    (connections <= 15 && neighbour->OverlayData / 3 != connections)) {
                    neighbour->OverlayTypeIndex = Veins;
                    // Unlike the centre, the remainder is taken before abs().
                    neighbour->OverlayData = static_cast<unsigned char>(
                        3 * connections + wrapping_abs(ScenarioClass::Instance->Random.Random() % 3));
                    neighbour->RecalcAttributes(-1);
                    register_redraw(*neighbour);
                }
            }
            radar.RadarBackground(neighbour->MapCoords);
        }
        ActivateVeins();
    }
    register_redraw(*this);
    RecalcAttributes(-1);
}
