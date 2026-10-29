// Original MapClass::BuildingToWall, 0x00588750.
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/OverlayTypeClass.h"

void MapClass::BuildingToWall(const CellStruct& origin, HouseClass* owner, BuildingTypeClass* type) {
    auto* overlay = type->ToOverlay;
    if (!overlay) return;
    for (int direction = 0; direction < 8; direction += 2) {
        const int range = type->GuardRange >> 8;
        const auto step = Unsorted::AdjacentCell[direction];
        auto at = origin;
        int count = 0;
        bool connected = false;
        while (count < range) {
            at = {short(at.X + step.X), short(at.Y + step.Y)};
            // 0x5887DD reads the global map, even if the caller passes another this.
            auto* cell = MapClass::Instance.GetCellAt(at);
            if (cell->OverlayTypeIndex == overlay->ArrayIndex && cell->WallOwnerIndex == owner->ArrayIndex) {
                connected = true;
                break;
            }
            if (!cell->CanThisExistHere(type->SpeedType, type, owner)) break;
            ++count;
        }
        if (!connected) continue;
        at = origin;
        for (int i = 0; i < count; ++i) {
            at = {short(at.X + step.X), short(at.Y + step.Y)};
            // The original creates each segment through the building type and
            // its wall Unlimbo, without another purchase or recursive gap fill.
            auto* segment = type->CreateObject(owner);
            // 0x588907 dereferences the allocation result unconditionally.
            // Native CreateObject can return null; contain that host failure.
            if (!segment) break;
            segment->Unlimbo({at.X * 256 + 128, at.Y * 256 + 128, 0}, DirType::North);
        }
    }
}
