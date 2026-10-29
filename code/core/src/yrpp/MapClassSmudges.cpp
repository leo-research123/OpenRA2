// Original smudge footprint clearing, 0x588AC0.
#include "yrpp/MapClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "map_world.hpp"
#include "background_invalidation.hpp"

void MapClass::ClearSmudges() noexcept {
    CellIteratorReset();
    while (auto* cell = CellIteratorNext()) {
        if (cell->SmudgeTypeIndex == -1) continue;
        auto* type = SmudgeTypeClass::Array[cell->SmudgeTypeIndex];
        const CellStruct origin{short(cell->MapCoords.X - cell->SmudgeData % type->Width),
                                short(cell->MapCoords.Y - cell->SmudgeData / type->Width)};
        for (int y = 0; y < type->Height; ++y)
            for (int x = 0; x < type->Width; ++x) {
                auto* part = GetCellAt(CellStruct{short(origin.X + x), short(origin.Y + y)});
                part->SmudgeTypeIndex = -1;
                // Original trigger requests a full redraw; retain the changed cells.
                game::invalidate_background_cell(part->MapCoords, 0x1);
                game::map_resource_changed(*part);
            }
    }
}
