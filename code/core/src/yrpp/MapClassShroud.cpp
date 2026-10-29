// Original local shroud and two-pass refresh, YR 0x00567F70 / 0x00578100.
#include "background_invalidation.hpp"
#include "visibility_grid.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/HouseClass.h"
#include <bit>

void YRPP_STDCALL MapClass::ShroudArea(CoordStruct* coords, int radius) noexcept {
    game::visibility_flush();
    const auto project = [coords](int coordinate) {
        const int shift = TacticalClass::AdjustForZ(coords->Z) / -30;
        const int leptons = std::bit_cast<int>(unsigned(coordinate) + unsigned(shift) * 256u);
        return std::bit_cast<short>(static_cast<unsigned short>(leptons / 256));
    };
    const int x = project(coords->X), y = project(coords->Y);
    const int first = std::bit_cast<int>(0u - unsigned(radius) - 2u);
    const int end = std::bit_cast<int>(unsigned(radius) + 2u);
    const int limit = std::bit_cast<int>((unsigned(radius) + 1u) * (unsigned(radius) + 1u));
    for (int dx = first; dx < end; ++dx)
        for (int dy = first; dy < end; ++dy) {
            if (std::bit_cast<int>(unsigned(dx) * unsigned(dx) + unsigned(dy) * unsigned(dy)) >= limit) continue;
            const CellStruct at{std::bit_cast<short>(static_cast<unsigned short>(unsigned(x) + unsigned(dx))),
                                std::bit_cast<short>(static_cast<unsigned short>(unsigned(y) + unsigned(dy)))};
            auto* cell = Instance.GetCellAt(at);
            if (cell->ShroudCounter == 0) {
                cell->ShroudCounter = 1;
                cell->AltFlags &= ~AltCellFlags::Clear;
                game::invalidate_background_shroud(cell->MapCoords);
                game::visibility_cache_dirty(cell->MapCoords);
            }
        }
}

void MapClass::RecalculateShroud() noexcept {
    game::visibility_flush();
    CellIteratorReset();
    while (auto* cell = CellIteratorNext()) {
        if ((cell->Flags & CellFlags::FlagToShroud) == CellFlags{}) continue;
        cell->AltFlags &= ~AltCellFlags::Clear;
        game::invalidate_background_shroud(cell->MapCoords);
        cell->Flags &= ~(CellFlags::Revealed | CellFlags::FlagToShroud);
        cell->VisibilityChanged = true;
        TacticalClass::Instance->RegisterCellAsVisible(cell);
        if (cell->Visibility == char(-2)) RevealCheck(cell, HouseClass::CurrentPlayer, true);
    }
    // Finish clearing pending flags before consulting any neighbour mask.
    CellIteratorReset();
    while (auto* cell = CellIteratorNext()) {
        CoordStruct buffer;
        const auto* position = cell->GetCoords(&buffer);
        const CellStruct at{short(position->X / 256), short(position->Y / 256)};
        const char visibility = TacticalClass::Instance->GetOcclusion(at, false);
        if (visibility != cell->Visibility) {
            cell->Visibility = visibility;
            cell->VisibilityChanged = true;
            TacticalClass::Instance->RegisterCellAsVisible(cell);
            if (cell->Visibility == char(-2)) RevealCheck(cell, HouseClass::CurrentPlayer, true);
        }
    }
}
