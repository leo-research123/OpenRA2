// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp Collapse_Cliff, calibrated to YR 0x00581140.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/AnimClass.h"
#include <cstdlib>
#include <new>
#if !defined(RA2_YRPP_GAME)
void MapClass::DestroyCliff(CellClass* cliff) {
    try {
        const int first = IsometricTileTypeClass::DestroyableCliffs;
        const int tile = cliff->IsoTileTypeIndex;
        if (tile != first && tile != first + 1) return;
        const bool horizontal = tile == first;
        const int sub = cliff->Height;
        const CellStruct origin{short(cliff->MapCoords.X - (horizontal ? sub % 6 : sub & 3)),
                                short(cliff->MapCoords.Y - (horizontal ? sub / 6 : sub >> 2))};
        const auto coords = [](CellStruct c, int z = 0) { return CoordStruct{c.X * 256 + 128, c.Y * 256 + 128, z}; };
        auto* type = IsometricTileTypeClass::Array[tile];
        auto* old = type->CreateObject(nullptr);
        old->SetLocation(coords(origin));
        old->InLimbo = false;
        old->IsOnMap = true;
        old->Mark(MarkType::Up);
        delete old;
        const int slope = IsometricTileTypeClass::SlopeSetPieces;
        auto* a = IsometricTileTypeClass::Array[slope + (horizontal ? 0 : 3)]->CreateObject(nullptr);
        a->Unlimbo(coords(origin), DirType::North);
        const CellStruct other{short(origin.X + (horizontal ? 3 : 0)), short(origin.Y + (horizontal ? 0 : 3))};
        auto* b = IsometricTileTypeClass::Array[slope + (horizontal ? 1 : 2)]->CreateObject(nullptr);
        b->Unlimbo(coords(other), DirType::North);
        ResetAllZones();
        const auto* image = reinterpret_cast<const TMPStruct*>(type->GetImage());
        for (int pass = 0; pass < 3; ++pass)
            for (int y = 0; y < int(type->unk_2E8); ++y)
                for (int x = 0; x < int(type->unk_2E4); ++x) {
                    const TMPImage* record = nullptr;
                    if (!image->GetSubTile(x + y * int(type->unk_2E4), record) || !record) continue;
                    const CellStruct at{short(origin.X + x), short(origin.Y + y)};
                    if (!IsWithinUsableArea(at, true)) continue;
                    auto& zone = LevelAndPassabilityStruct2pointer_70[GetCellZoneIndex(at)];
                    if (pass == 0) {
                        zone.SubzoneIDs[0] = 0;
                        Instance.GetCellAt(at)->RecalcAttributes(-1);
                    } else if (pass == 1) {
                        if (zone.SubzoneIDs[0] == 0) RecalculateSubZones(at);
                    } else {
                        Instance.GetCellAt(at)->BecomeUntargetable();
                        RadarClass::Instance.RadarBackground(at);
                    }
                }
        Point2D p{}, q{};
        auto start = coords(origin, Unsorted::LevelHeight * Instance.GetCellAt(origin)->Level);
        const CellStruct end{short(origin.X + (horizontal ? 5 : 3)), short(origin.Y + (horizontal ? 3 : 5))};
        auto finish = coords(end, Unsorted::LevelHeight * Instance.GetCellAt(end)->Level);
        TacticalClass::Instance->CoordsToClient(&start, &p);
        TacticalClass::Instance->CoordsToClient(&finish, &q);
        // The NS branch intentionally anchors at the second projected corner.
        TacticalClass::Instance->RegisterDirtyArea(
            horizontal ? RectangleStruct{p.X - 120, p.Y - 60, q.X - p.X + 240, q.Y - p.Y + 120}
                       : RectangleStruct{q.X - 120, p.Y - 60, p.X - q.X + 240, q.Y - p.Y + 120},
            false);
        AnimTypeClass* debris[]{AnimTypeClass::Array[AnimTypeClass::FindIndex("XGRYMED1")],
                                AnimTypeClass::Array[AnimTypeClass::FindIndex("XGRYMED2")],
                                AnimTypeClass::Array[AnimTypeClass::FindIndex("XGRYSML1")]};
        auto& random = ScenarioClass::Instance->Random;
        for (int y = origin.Y; y < origin.Y + (horizontal ? 3 : 5); ++y)
            for (int x = origin.X; x < origin.X + (horizontal ? 5 : 3); ++x)
                for (int i = 0; i < 2; ++i) {
                    auto* animType = debris[random.RandomRanged(0, 2)];
                    if (auto* storage = YRMemory::Allocate(sizeof(AnimClass))) {
                        const int dy = random.RandomRanged(-8, 8), dx = random.RandomRanged(-12, 12);
                        const CellStruct at{short(x), short(y)};
                        auto position = coords(at, Unsorted::LevelHeight * Instance.GetCellAt(at)->Level);
                        position.X += dx;
                        position.Y += dy;
                        const int delay = random.RandomRanged(0, 2);
                        ::new (storage) AnimClass(animType, position, delay, 1, 0x600, 0, false);
                    }
                }
    } catch (...) {
        std::abort();
    }
}
#endif
