// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 isotile.cpp Mark/Unlimbo/Limbo, calibrated to YR.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/IsometricTileClass.h"
#include "visibility_grid.hpp"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TerrainClass.h"
#include <cstdlib>
#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<IsometricTileClass*> tiles;
}
DynamicVectorClass<IsometricTileClass*>& IsometricTileClass::Array = tiles;
IsometricTileClass::IsometricTileClass(int type, const CellStruct& at) noexcept
    : ObjectClass(), Type(IsometricTileTypeClass::Array[type]) {
    if (at != CellStruct::Empty) {
        InLimbo = false;
        SetLocation({at.X * 256 + 128, at.Y * 256 + 128, 0});
        Mark(MarkType::Down);
    }
    if (!Array.AddItem(this)) std::abort();
}
IsometricTileClass::~IsometricTileClass() {
    NotifyObjectExpired(true);
    Limbo();
    Type = nullptr;
    Array.Remove(this);
}
bool IsometricTileClass::Unlimbo(const CoordStruct& at, DirType) {
    try {
        InLimbo = false;
        SetLocation(at);
        return Mark(MarkType::Down);
    } catch (...) {
        std::abort();
    }
}
bool IsometricTileClass::Limbo() {
    if (!Game::IsActive || InLimbo) return false;
    try {
        Deselect();
        Disappear(true);
        Mark(MarkType::Up);
        InLimbo = true;
        NeedsRedraw = false;
        return true;
    } catch (...) {
        std::abort();
    }
}
bool IsometricTileClass::Mark(MarkType mark) {
    game::visibility_terrain_changed();
    try {
        const auto* image = reinterpret_cast<const TMPStruct*>(GetImage());
        if (!image || !ObjectClass::Mark(mark)) return false;
        for (int y = 0; y < int(Type->unk_2E8); ++y)
            for (int x = 0; x < int(Type->unk_2E4); ++x) {
                const auto origin = GetMapCoords();
                const CellStruct at{short(origin.X + x), short(origin.Y + y)};
                if (!MapClass::Instance.CoordinatesLegal(at)) continue;
                auto* c = MapClass::Instance.GetCellAt(at);
                const int sub = x + y * int(Type->unk_2E4);
                const TMPImage* record = nullptr;
                if (!image->GetSubTile(sub, record) || !record) continue;
                if (mark == MarkType::Up) {
                    if (c->IsoTileTypeIndex == Type->ArrayIndex && c->Height == sub) {
                        c->IsoTileTypeIndex = 0xFFFF;
                        c->Height = 0;
                        c->Level -= record->Height;
                    }
                } else if (mark == MarkType::Down || mark == MarkType::ChangeRedraw) {
                    if (Type->ArrayIndex == IsometricTileTypeClass::ClearTile) {
                        c->IsoTileTypeIndex = 0xFFFF;
                        c->Height = 0;
                    } else {
                        c->IsoTileTypeIndex = Type->ArrayIndex;
                        c->Height = static_cast<unsigned char>(sub);
                        c->SlopeIndex = static_cast<unsigned char>(Type->GetSlopeIndex(sub));
                        if (c->Height == 0 || c->Height == 3 || c->Height == 6 || c->Height == 9) {
                            if (c->IsoTileTypeIndex == IsometricTileTypeClass::SlopeSetPieces + 5) {
                                c->IsoTileTypeIndex = IsometricTileTypeClass::RampBase + 1;
                                c->Height = 0;
                            }
                            if (c->IsoTileTypeIndex == IsometricTileTypeClass::SlopeSetPieces2 + 5) {
                                c->IsoTileTypeIndex = IsometricTileTypeClass::MMRampBase + 1;
                                c->Height = 0;
                            }
                        }
                        if (c->Height <= 3) {
                            if (c->IsoTileTypeIndex == IsometricTileTypeClass::SlopeSetPieces + 8) {
                                c->IsoTileTypeIndex = IsometricTileTypeClass::RampBase;
                                c->Height = 0;
                            }
                            if (c->IsoTileTypeIndex == IsometricTileTypeClass::SlopeSetPieces2 + 8) {
                                c->IsoTileTypeIndex = IsometricTileTypeClass::MMRampBase;
                                c->Height = 0;
                            }
                        }
                    }
                    c->SmudgeTypeIndex = -1;
                    c->SmudgeData = 0;
                    if (c->OverlayTypeIndex >= 27 && c->OverlayTypeIndex < 38) {
                        c->Flags = static_cast<CellFlags>(unsigned(c->Flags) & ~0x20000u);
                        auto location = c->GetCoords();
                        location.X += 384;
                        location.Y += 384;
                        const auto* type = reinterpret_cast<const ObjectTypeClass*>(
                            OverlayTypeClass::Array[c->OverlayTypeIndex]->CellAnim);
                        for (int i = 0; i < TerrainClass::Array.Count; ++i) {
                            auto* terrain = TerrainClass::Array[i];
                            if (terrain->GetType() == type && terrain->Location == location) {
                                delete terrain;
                                break;
                            }
                        }
                    }
                    c->OverlayTypeIndex = -1;
                    c->OverlayData = 0;
                    c->Level += record->Height;
                    c->SetupLAT();
                    for (auto dir : {FacingType::North, FacingType::East, FacingType::South, FacingType::West})
                        c->GetNeighbourCell(dir)->SetupLAT();
                }
                c->RecalcAttributes(-1);
            }
        if (mark == MarkType::Down || mark == MarkType::ChangeRedraw) {
            IsOnMap = false;
            delete this;
        }
        return true;
    } catch (...) {
        std::abort();
    }
}
#endif
