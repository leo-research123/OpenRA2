#include "yrpp/ScenarioClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/TechnoClass.h"
#include <algorithm>

DynamicVectorClass<CellStruct>* YRPP_FASTCALL ScenarioClass::CollectStartingCells(
    DynamicVectorClass<CellStruct>* output) noexcept {
    if (!output || output->Count || output->Capacity || !Instance) return nullptr;
    try {
        const CellStruct invalid{0, 0};
        int count = 0;
        while (count < MaxPlayerSlots && Instance->Waypoints[count] != invalid) ++count;
        int spectators = 0;
        for (auto* player : NodeNameType::Array)
            if (player->SpectatorFlag == -1) ++spectators;
        count = std::max(count, SessionClass::Instance.Config.AIPlayers + NodeNameType::Array.Count - spectators);
        // Original multiplayer assignment has only sixteen starting-position slots.
        if (count < 0 || count > MaxStartingPositions) return nullptr;
        DynamicVectorClass<CellStruct> cells;
        for (int i = 0; i < count; ++i)
            if (Instance->Waypoints[i] != invalid && !cells.AddItem(Instance->Waypoints[i])) return nullptr;
        auto& map = MapClass::Instance;
        const auto& rect = map.MapCoordBounds;
        if (cells.Count < count && (rect.Right < 20 || rect.Bottom < 10)) return nullptr;
        // The original retries forever on an unusable map. Bound that native failure.
        int attempts = 0;
        while (cells.Count < count) {
            if (++attempts > 4096) return nullptr;
            const short y = static_cast<short>(Instance->Random.RandomRanged(0, rect.Bottom - 10));
            CellStruct cell{static_cast<short>(rect.Left + Instance->Random.RandomRanged(10, rect.Right - 10)),
                            static_cast<short>(y + rect.Top + 10)};
            CellStruct found;
            map.NearByLocation(found, cell, static_cast<SpeedType>(1), -1, static_cast<MovementZone>(0),
                               false, 8, 8, false, false, false, true, CellStruct{0, 0}, false, false);
            if (found != invalid && !cells.AddItem(found)) return nullptr;
        }
        *output = cells;
        return output;
    } catch (...) {
        return nullptr;
    }
}

int YRPP_FASTCALL ScenarioClass::PlaceStartingUnit(TechnoClass* object, const CellStruct* origin,
                                                 int radius) noexcept {
    if (!object || !origin || !Instance) return 0;
    try {
        auto& map = MapClass::Instance;
        const auto place = [&](const CellStruct& cell) {
            auto* occupied = map.GetCellAt(cell)->FindTechnoNearestTo(Point2D{0, 0}, false, nullptr);
            if (occupied && (occupied->WhatAmI() != AbstractType::Infantry ||
                             object->WhatAmI() != AbstractType::Infantry)) return false;
            CoordStruct location{cell.X * 256 + 128, cell.Y * 256 + 128, 0};
            location.Z = map.GetCellFloorHeight(location);
            return object->Unlimbo(location, DirType::North);
        };
        if (map.IsWithinUsableArea(*origin, true) && place(*origin)) return 1;
        const auto& rect = map.MapCoordBounds;
        const int maxX = rect.Left + rect.Right - 1;
        const int maxY = rect.Top + rect.Bottom - 1;
        if (rect.Right <= 0 || rect.Bottom <= 0 || radius < 0) return 0;
        static constexpr int dx[8]{0, 1, 1, 1, 0, -1, -1, -1};
        static constexpr int dy[8]{-1, -1, 0, 1, 1, 1, 0, -1};
        auto& random = Instance->Random;
        for (; radius < 32; ++radius) {
            int direction = random.RandomRanged(0, 7);
            for (int pass = 0; pass < 2; ++pass) {
                for (int i = 0; i < 8; ++i, direction = (direction + 1) & 7) {
                    CellStruct cell{
                        static_cast<short>(std::clamp(origin->X + dx[direction] * radius, rect.Left, maxX)),
                        static_cast<short>(std::clamp(origin->Y + dy[direction] * radius, rect.Top, maxY))};
                    if (pass) {
                        const int x = random.RandomRanged(0, 1);
                        cell.X = static_cast<short>(random.RandomRanged(0, 99) < 50 ?
                            std::min(int(cell.X) + x, maxX) : std::max(int(cell.X) - x, rect.Left));
                        const int y = random.RandomRanged(0, 1);
                        cell.Y = static_cast<short>(random.RandomRanged(0, 99) < 50 ?
                            std::min(int(cell.Y) + y, maxY) : std::max(int(cell.Y) - y, rect.Top));
                    }
                    if (map.IsWithinUsableArea(cell, true) && cell != *origin && place(cell)) return 1;
                }
            }
        }
        return 0;
    } catch (...) {
        return 0;
    }
}
