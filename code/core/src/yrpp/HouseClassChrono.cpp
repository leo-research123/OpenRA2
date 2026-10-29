// Original House relocation for trigger action 128, 0x0050D6D0.
#include "yrpp/HouseClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/CellSpread.h"
#include <bit>
#include <cstdlib>

void HouseClass::TeleportUnitsTo(CellStruct destination) noexcept {
    try {
        if (destination == CellStruct::Empty) return;
        unsigned index = 0;
        const auto target = [&] {
            const auto offset = CellSpread::GetCell(index);
            return CellStruct{
                std::bit_cast<short>(static_cast<unsigned short>(unsigned(destination.X) + unsigned(offset.X))),
                std::bit_cast<short>(static_cast<unsigned short>(unsigned(destination.Y) + unsigned(offset.Y)))};
        };
        for (int i = FootClass::Array.Count - 1; i >= 0; --i) {
            auto* object = FootClass::Array[i];
            auto cell = target();
            if (object->Owner != this) continue;
            const bool unit = object->WhatAmI() == AbstractType::Unit && !object->Transporter;
            if (!unit && (object->WhatAmI() != AbstractType::Infantry || object->Transporter)) continue;
            auto* foot = static_cast<FootClass*>(object);
            while (foot->IsCellOccupied(MapClass::Instance.GetCellAt(cell), FacingType(-1), -1, nullptr, true) !=
                   Move::OK) {
                if (++index > 350) return;
                cell = target();
            }
            auto* at = MapClass::Instance.GetCellAt(cell);
            CoordStruct buffer;
            auto coords = *at->GetCoords(&buffer);
            if (!unit) {
                CoordStruct output;
                coords =
                    *MapClass::Instance.GetCellAt(cell)->FindInfantrySubposition(&output, coords, false, false, false);
            }
            if (foot->ChronoWarpTo(coords) && ++index > 350) return;
        }
    } catch (...) {
        std::abort();
    }
}
