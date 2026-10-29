// Original world transition actions, 0x006E1A40 / 0x006E21E0.
#include "yrpp/TActionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/ScenarioClass.h"
#include <cstdlib>

bool TActionClass::TeleportAllHouseObjectTo(HouseClass* house, ObjectClass*, TriggerClass*,
                                            const CellStruct&) noexcept {
    house->TeleportUnitsTo(ScenarioClass::Instance->GetWaypointCoords(Waypoint));
    return true;
}
bool TActionClass::ResizePlayerView(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    try {
        auto& map = MapClass::Instance;
        map.VisibleRect = Bounds;
        RadarClass::Instance.ResetRadar();
        map.CellIteratorReset();
        while (auto* cell = map.CellIteratorNext())
            cell->RecalcAttributes(-1);
        map.ResetAllZones();
        map.ResetAllSubzones();
        map.sub_657CE0();
        for (int i = BuildingClass::Array.Count - 1; i >= 0; --i)
            BuildingClass::Array[i]->RadarTrackingUpdate(true);
        return true;
    } catch (...) {
        std::abort();
    }
}
