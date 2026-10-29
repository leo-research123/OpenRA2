// Automatic base construction action, YR 0x6E0EF0 and inline 0x6DE21x.
#include "yrpp/TActionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
bool TActionClass::SetAutoBaseBuilding(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    try {
        // The EXE tests the low byte of Value, not the entire integer.
        house->AutoBaseBuilding = static_cast<unsigned char>(Value) != 0;
        if (house->AutoBaseBuilding) {
            if (house->ConYards.Count > 0) {
                const auto& where = house->ConYards[0]->Location;
                const CellStruct cell{short(where.X / 256), short(where.Y / 256)};
                house->BaseSpawnCell = house->Base.Center = cell;
            }
            if (!house->Base.BaseNodes.Count) house->InitializeBase();
        }
        return true;
    } catch (...) {
        return false;
    }
}
