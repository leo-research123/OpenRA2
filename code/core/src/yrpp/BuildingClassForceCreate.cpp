// Original forced placement and displaced-unit recovery, 0x458A80.
#include "background_invalidation.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "map_world.hpp"
#include <cstdlib>

bool BuildingClass::ForceCreate(CoordStruct& position, DWORD facing) noexcept {
    try {
        const CellStruct origin{short(position.X / 256), short(position.Y / 256)};
        DynamicVectorClass<TechnoClass*> displaced;
        const auto restore = [&] {
            for (int i = displaced.Count - 1; i >= 0; --i) {
                auto* object = displaced[i];
                const auto old = object->Location;
                if (!object->Unlimbo(old, DirType::North)) object->UnInit();
            }
        };
        for (auto* part = GetFoundationData(false); part->X != 0x7FFF || part->Y != 0x7FFF; ++part) {
            auto* cell = MapClass::Instance.GetCellAt(CellStruct{short(origin.X + part->X), short(origin.Y + part->Y)});
            if (cell->OverlayTypeIndex != -1) {
                // Notify even if forced placement subsequently fails.
                game::invalidate_background_cell(cell->MapCoords, 0x6);
                cell->OverlayTypeIndex = -1;
                cell->OverlayData = 0;
                cell->Rubble = nullptr;
                game::map_resource_changed(*cell);
            }
            while (auto* object = cell->FirstObject) {
                if ((object->AbstractFlags & ::AbstractFlags::Techno) == ::AbstractFlags::None ||
                    object->WhatAmI() == AbstractType::Building) {
                    restore();
                    return false;
                }
                (void)displaced.AddItem(static_cast<TechnoClass*>(object));
                object->Limbo();
            }
        }
        if (!Unlimbo(position, static_cast<DirType>(facing))) {
            UnInit();
            restore();
            return false;
        }
        for (int i = displaced.Count - 1; i >= 0; --i) {
            auto* object = displaced[i];
            CellStruct output;
            const auto old = *object->GetMapCoords(&output);
            CellStruct destination;
            MapClass::Instance.NearByLocation(destination, old, object->GetTechnoType()->SpeedType, -1,
                                              MovementZone::Normal, false, 1, 1, false, false, false, true,
                                              CellStruct::Empty, false, false);
            const auto at = MapClass::Instance.GetCellAt(destination)->GetCoords();
            if (!object->Unlimbo(at, DirType::North)) object->UnInit();
        }
        return true;
    } catch (...) {
        std::abort();
    }
}
