// YR 0x4DF510: displace occupants, place the unit, then restore in reverse order.
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include <cstdlib>
#if !defined(RA2_YRPP_GAME)
bool FootClass::ForceCreate(CoordStruct& position, DWORD facing) noexcept {
    try {
        const CellStruct origin{short(position.X / 256), short(position.Y / 256)};
        auto* cell = MapClass::Instance.GetCellAt(origin);
        DynamicVectorClass<TechnoClass*> displaced;
        const auto restore = [&] {
            for (int i = displaced.Count - 1; i >= 0; --i) {
                auto* object = displaced[i];
                const auto old = object->Location;
                if (!object->Unlimbo(old, DirType::North)) object->UnInit();
            }
        };
        while (auto* object = cell->FirstObject) {
            if ((object->AbstractFlags & ::AbstractFlags::Techno) == ::AbstractFlags::None ||
                object->WhatAmI() == AbstractType::Building) {
                restore();
                return false;
            }
            (void)displaced.AddItem(static_cast<TechnoClass*>(object));
            object->Limbo();
        }
        if (!Unlimbo(position, static_cast<DirType>(facing))) {
            UnInit();
            restore();
            return false;
        }
        for (int i = displaced.Count - 1; i >= 0; --i) {
            auto* object = displaced[i];
            const auto old = object->GetMapCoords();
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
#endif
