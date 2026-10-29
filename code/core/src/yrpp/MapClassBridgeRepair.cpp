// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp bridge repair; calibrated to YR 0x00570050 /
// 0x00573540 and their span, tile-state and pathfinding dependencies.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "background_invalidation.hpp"
#include "visibility_grid.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/OverlayClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/Randomizer.h"
#include "yrpp/Surface.h"
#include "yrpp/TacticalClass.h"
#include <algorithm>
#include <cstdlib>
#include <new>

#if !defined(RA2_YRPP_GAME)
namespace {
CellStruct offset(CellStruct cell, int x, int y) { return {short(cell.X + x), short(cell.Y + y)}; }
CellStruct adjacent(CellStruct cell, int facing) {
    const auto& step = CellSpread::GetNeighbourOffset(facing & 7);
    return offset(cell, step.X, step.Y);
}
CellClass* fetch(CellStruct cell) { return MapClass::Instance.GetCellAt(cell); }
unsigned flags(const CellClass* cell) { return static_cast<unsigned>(cell->Flags); }
bool low(CellStruct cell, bool wood) {
    const int overlay = fetch(cell)->OverlayTypeIndex;
    return wood ? overlay >= 74 && overlay <= 101 : overlay >= 205 && overlay <= 232;
}
RectangleStruct united(RectangleStruct a, RectangleStruct b) {
    if (a.Width <= 0 || a.Height <= 0) return b;
    if (b.Width <= 0 || b.Height <= 0) return a;
    if (a.X > b.X) {
        a.Width += a.X - b.X;
        a.X = b.X;
    }
    if (a.Y > b.Y) {
        a.Height += a.Y - b.Y;
        a.Y = b.Y;
    }
    if (a.X + a.Width < b.X + b.Width) a.Width = b.X - a.X + b.Width + 1;
    if (a.Y + a.Height < b.Y + b.Height) a.Height = b.Y - a.Y + b.Height + 1;
    return a;
}
RectangleStruct overlay_rectangle(CellClass* cell) {
    RectangleStruct shadow{}, body{};
    cell->ShapeRect(&shadow);
    cell->GetContainingRect(&body);
    return united(body, shadow);
}
void dirty(RectangleStruct area) {
    if (TacticalClass::Instance) TacticalClass::Instance->RegisterDirtyArea(area, false);
}
void radar(CellStruct cell) { RadarClass::Instance.RadarBackground(cell); }
void recalc_rectangle(MapClass& map, RectangleStruct area) {
    DynamicVectorClass<CellStruct> cells;
    for (int x = area.X; x < area.X + area.Width; ++x)
        for (int y = area.Y; y < area.Y + area.Height; ++y)
            cells.AddItem({short(x), short(y)});
    map.Update_Pathfinding_2(cells);
}
// YR 0x00598030 uses Global, an unsigned floating-point pick, not RandomRanged.
int low_variant() {
    unsigned result;
    do {
        result =
            static_cast<unsigned>(static_cast<unsigned>(Randomizer::Global.Random()) * 4.0 * 2.328306437080797e-10);
    } while (result > 3);
    return static_cast<int>(result);
}
// YR 0x0057F6A0 / 0x0057FBC0 / 0x005800D0 / 0x00580600.
void repair_low_run(MapClass& map, CellStruct cell, bool wood, bool horizontal) {
    const int base = wood ? 74 : 205;
    const int start = horizontal ? 0 : 9;
    bool changed = false;
    RectangleStruct recalc{};
    do {
        cell = horizontal ? offset(cell, -1, 0) : offset(cell, 0, -1);
    } while (low(cell, wood));
    cell = horizontal ? offset(cell, 1, 0) : offset(cell, 0, 1);
    do {
        auto* center = fetch(cell);
        auto* first = fetch(horizontal ? offset(cell, 0, -1) : offset(cell, -1, 0));
        auto* second = fetch(horizontal ? offset(cell, 0, 1) : offset(cell, 1, 0));
        auto draw = overlay_rectangle(center);
        const int old = center->OverlayTypeIndex;
        const int relative = old - base;
        int value = old;
        if (relative == start + 4 || relative == start + 5 || relative == start + 6 || relative == start + 7 ||
            relative == start + 8 || relative == (horizontal ? 26 : 27)) {
            value = base + start + low_variant();
            recalc = united(recalc, horizontal ? RectangleStruct{cell.X, cell.Y - 1, 1, 3}
                                               : RectangleStruct{cell.X - 1, cell.Y, 3, 1});
            changed = true;
        } else if (relative == (horizontal ? 18 : 22) || relative == (horizontal ? 19 : 23)) {
            value = base + (horizontal ? 18 : 22);
        } else if (relative == (horizontal ? 20 : 24) || relative == (horizontal ? 21 : 25)) {
            value = base + (horizontal ? 20 : 24);
        }
        if (value != old) {
            center->OverlayTypeIndex = first->OverlayTypeIndex = second->OverlayTypeIndex = value;
            draw = united(draw, overlay_rectangle(center));
            draw.X -= DSurface::ViewBounds.X;
            draw.Y -= DSurface::ViewBounds.Y;
            dirty(draw);
            if (relative == (horizontal ? 26 : 27)) {
                radar(cell);
                radar(horizontal ? offset(cell, 0, 1) : offset(cell, 1, 0));
                radar(horizontal ? offset(cell, 0, -1) : offset(cell, -1, 0));
            }
            center->RecalcAttributes(-1);
            first->RecalcAttributes(-1);
            second->RecalcAttributes(-1);
            center->KillIllegalOccupiers(false);
            first->KillIllegalOccupiers(false);
            second->KillIllegalOccupiers(false);
        }
        cell = horizontal ? offset(cell, 1, 0) : offset(cell, 0, 1);
    } while (low(cell, wood));
    if (changed) map.ResetAllZones();
    if (recalc.Width > 0 && recalc.Height > 0) recalc_rectangle(map, recalc);
}
// YR 0x0057F200 / 0x0057F440 resolves the center line across the three-cell deck.
void repair_low(MapClass& map, CellStruct cell, bool wood) {
    const int relative = fetch(cell)->OverlayTypeIndex - (wood ? 74 : 205);
    const bool horizontal = (relative >= 0 && relative <= 8) || (relative >= 18 && relative <= 21) || relative == 26;
    const CellStruct before = horizontal ? offset(cell, 0, -1) : offset(cell, -1, 0);
    const CellStruct before2 = horizontal ? offset(cell, 0, -2) : offset(cell, -2, 0);
    if (!low(before, wood))
        cell = horizontal ? offset(cell, 0, 1) : offset(cell, 1, 0);
    else if (low(before2, wood))
        cell = before;
    repair_low_run(map, cell, wood, horizontal);
}
// YR 0x0056E990, tile-wide end damage bit propagation.
void set_end(CellStruct cell, bool damaged, bool recursive = false) {
    auto* current = fetch(cell);
    if (!recursive) {
        const int tile = current->IsoTileTypeIndex;
        if (tile == 0xFFFF || tile == 255 ||
            !IsometricTileTypeClass::Array[tile]->IsRandomized(static_cast<unsigned char>(current->Height)))
            return;
        const CoordStruct world{cell.X * 256 + 128, cell.Y * 256 + 128, 0};
        Point2D point{};
        TacticalClass::Instance->CoordsToClient(&world, &point);
        point.Y -= 15 * current->Level;
        dirty({point.X - 128, point.Y - 128, 256, 256});
    }
    if (damaged == ((flags(current) & 0x2000u) != 0)) return;
    const int old = current->IsoTileTypeIndex;
    current->Flags = static_cast<CellFlags>((flags(current) & ~0x2000u) | (damaged ? 0x2000u : 0u));
    // Recursive clusters can extend beyond the original root rectangle.
    game::invalidate_background_cell(current->MapCoords, 0x1F);
    radar(current->MapCoords);
    for (int i = 0; i < 8; ++i) {
        const auto next = adjacent(cell, i);
        if (fetch(next)->IsoTileTypeIndex == old) set_end(next, damaged, true);
    }
}
// YR 0x0056EB80, same tile cluster with an attribute/radar update per cell.
void set_middle(CellStruct cell, int tile, int match = -1, int level = -1, bool recursive = false) {
    auto* current = fetch(cell);
    if (!recursive) {
        const CoordStruct world{cell.X * 256 + 128, cell.Y * 256 + 128, 0};
        Point2D point{};
        TacticalClass::Instance->CoordsToClient(&world, &point);
        point.Y -= 15 * current->Level;
        dirty({point.X - 128, point.Y - 128, 256, 256});
        match = current->IsoTileTypeIndex;
        if (tile == match) return;
    }
    if (current->IsoTileTypeIndex == tile) return;
    current->IsoTileTypeIndex = tile;
    current->RecalcAttributes(level);
    radar(current->MapCoords);
    for (int i = 0; i < 8; ++i) {
        const auto next = adjacent(cell, i);
        if (fetch(next)->IsoTileTypeIndex == match) set_middle(next, tile, match, level, true);
    }
}
// YR 0x005851B0 appends to the live graph, unlike the hash staging used by ResetSubzone.
void register_connection(MapClass& map, const ZoneConnectionClass& connection) {
    const auto from = connection.FromMapCoords, to = connection.ToMapCoords;
    auto* cell = fetch(from);
    CellStruct enter1{}, enter2{}, end1{}, end2 = CellStruct::Empty;
    if (cell->Tile_Is_Bridge() || cell->Tile_Is_WoodBridge()) {
        static constexpr int sides[]{0, 0, -1, 2, 2, -1, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2};
        const int base =
            cell->Tile_Is_Bridge() ? IsometricTileTypeClass::BridgeSet : IsometricTileTypeClass::WoodBridgeSet;
        const int side = sides[cell->IsoTileTypeIndex - base];
        enter1 = adjacent(from, side);
        enter2 = adjacent(from, side - 4);
        end1 = adjacent(to, side);
        end2 = adjacent(to, side - 4);
    }
    for (int level = 0; level < 3; ++level) {
        const auto link = [&](CellStruct a, CellStruct b) {
            const int first = map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex(a)].SubzoneIDs[level];
            const int second = map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex(b)].SubzoneIDs[level];
            auto& tracking = map.SubzoneTracking[level];
            tracking[first].SubzoneConnections.AddItem({static_cast<unsigned>(second), false});
            tracking[second].SubzoneConnections.AddItem({static_cast<unsigned>(first), false});
        };
        link(from, to);
        link(enter1, end1);
        link(enter2, end2);
    }
}
// YR 0x0056DB70, all overlapping bridge connections, in original vector order.
bool restore_connections(MapClass& map, CellStruct cell) {
    int index = map.ZoneConnectionIndex(cell, 3, 0);
    if (index == -1) {
        map.ComputeZoneConnections();
        index = map.ZoneConnectionIndex(cell, 3, 0);
        if (index == -1) return false;
    }
    bool reset = false;
    do {
        auto& connection = map.ZoneConnections[index];
        if (!connection.IsPassable) {
            connection.IsPassable = true;
            register_connection(map, connection);
            if (!MapClass::IsSameCellZone(connection.FromMapCoords, connection.ToMapCoords, MovementZone::Normal, false,
                                          false, false))
                reset = true;
        }
        index = map.ZoneConnectionIndex(cell, 3, index + 1);
    } while (index != -1);
    return reset;
}
void collect(DynamicVectorClass<CellStruct>& cells, CellStruct center, bool horizontal) {
    for (int a = 0; a < 2; ++a)
        for (int b = -2; b < 3; ++b)
            cells.AddItem(horizontal ? offset(center, a, b) : offset(center, b, a));
}
void raise(CellStruct center, bool horizontal) {
    game::visibility_terrain_changed();
    auto* middle = fetch(center);
    middle->Level = char(middle->Level + 4);
    for (int direction : {horizontal ? 0 : 2, horizontal ? 4 : 6}) {
        auto* side = fetch(adjacent(center, direction));
        side->Level = char(side->Level + 4);
    }
}
// YR 0x00568E40 / 0x00569760.
void repair_span(MapClass& map, CellClass* origin, int direction, RectangleStruct* area, bool wood) {
    const int base = wood ? IsometricTileTypeClass::WoodBridgeSet : IsometricTileTypeClass::BridgeSet;
    const int middle = direction == 2 ? IsometricTileTypeClass::BridgeMiddle1 : IsometricTileTypeClass::BridgeMiddle2;
    bool reset = false;
    DynamicVectorClass<CellStruct> cells;
    CellStruct at = origin->MapCoords;
    int length = 1;
    for (; length < 30; ++length) {
        at = adjacent(at, direction);
        auto* cell = fetch(at);
        const int relative = cell->IsoTileTypeIndex - base + 1;
        const auto subTile = static_cast<unsigned char>(cell->Height);
        const bool end = direction == 2 ? ((relative == IsometricTileTypeClass::BridgeBottomRight1 ||
                                            relative == IsometricTileTypeClass::BridgeBottomRight2) &&
                                           subTile == 4)
                                        : ((relative == IsometricTileTypeClass::BridgeBottomLeft1 ||
                                            relative == IsometricTileTypeClass::BridgeBottomLeft2) &&
                                           subTile == 2);
        if (end) {
            set_end(at, false);
            reset |= restore_connections(map, adjacent(at, direction - 4));
            break;
        }
        if (relative >= middle && relative <= middle + 4 && subTile == (direction == 2 ? 4 : 2)) {
            set_middle(at, base + middle - 1);
            reset |= restore_connections(map, at);
            if (relative == middle + 4) {
                raise(at, direction == 2);
                collect(cells, at, direction == 2);
            }
        }
    }
    if (length == 30) return;
    at = origin->MapCoords;
    Point2D start{};
    if (area) {
        const CoordStruct world{at.X * 256 + 128, at.Y * 256 + 128, 104 * origin->Level};
        TacticalClass::Instance->CoordsToClient(&world, &start);
    }
    for (int n = 0; n < length; ++n) {
        auto* cell = fetch(at);
        const int relative = cell->IsoTileTypeIndex - base + 1;
        const auto subTile = static_cast<unsigned char>(cell->Height);
        if ((relative != IsometricTileTypeClass::BridgeMiddle1 || (subTile & 1)) &&
            (relative != IsometricTileTypeClass::BridgeMiddle2 || subTile >= 5)) {
            if (auto* memory = YRMemory::Allocate(sizeof(OverlayClass)))
                ::new (memory) OverlayClass(OverlayTypeClass::Array[direction == 2 ? 24 : 25], cell->MapCoords, -1);
        }
        at = adjacent(at, direction);
    }
    if (area) {
        const CoordStruct world{at.X * 256 + 128, at.Y * 256 + 128, 104 * fetch(at)->Level};
        Point2D end{};
        TacticalClass::Instance->CoordsToClient(&world, &end);
        *area = {std::min(start.X, end.X) - 64, std::min(start.Y, end.Y) - 64, std::abs(start.X - end.X) + 128,
                 std::abs(start.Y - end.Y) + 128};
    }
    if (reset) map.ResetAllZones();
    if (cells.Count) map.Update_Pathfinding_2(cells);
}
void repair_bridge(MapClass& map, const CellStruct& supplied, bool wood) {
    // Repair's X-major first match differs from the query's Y-major last match.
    for (int x = -2; x < 3; ++x)
        for (int y = -2; y < 3; ++y) {
            const auto at = offset(supplied, x, y);
            if (low(at, wood)) {
                repair_low(map, at, wood);
                return;
            }
        }
    auto* cell = fetch(supplied);
    if (!(flags(cell) & 0x500u)) {
        for (int direction = 0; direction < 8; ++direction) {
            auto at = supplied;
            for (int n = 0; n < 3; ++n) {
                at = adjacent(at, direction);
                cell = fetch(at);
                if (flags(cell) & 0x500u) break;
            }
            if (flags(cell) & 0x500u) break;
        }
    }
    const auto state = flags(cell);
    if (!(state & 0x500u)) return;
    CellStruct at;
    if (state & 0x100u)
        at = (state & 0x80u) ? cell->MapCoords : cell->BridgeOwnerCell->MapCoords;
    else {
        const int direction = (state & 0x800u) ? 4 : 2;
        at = cell->MapCoords;
        int count = 0;
        for (;;) {
            at = adjacent(at, direction);
            if (!(flags(fetch(at)) & 0x400u)) break;
            if (++count >= 4) return;
        }
        at = adjacent(adjacent(at, direction - 4), direction - 4);
    }
    const int base = wood ? IsometricTileTypeClass::WoodBridgeSet : IsometricTileTypeClass::BridgeSet;
    const int advance = (state & 0x800u) ? 6 : 0;
    DynamicVectorClass<CellStruct> cells;
    bool reset = false;
    const auto in_bounds = [&](CellStruct c) {
        if (wood)
            return c.X + c.Y > MapClass::Instance.MapRect.Width && c.X - c.Y < MapClass::Instance.MapRect.Width &&
                   c.Y - c.X < MapClass::Instance.MapRect.Width &&
                   c.X + c.Y <= MapClass::Instance.MapRect.Width + 2 * MapClass::Instance.MapRect.Height;
        return c.X >= map.MapCoordBounds.Left && c.X <= map.MapCoordBounds.Left + map.MapCoordBounds.Right &&
               c.Y >= map.MapCoordBounds.Top && c.Y <= map.MapCoordBounds.Top + map.MapCoordBounds.Bottom;
    };
    while (in_bounds(at)) {
        if (map.Cells.Items[MapClass::GetCellIndex(at)]) {
            auto* span = fetch(at);
            const int relative = span->IsoTileTypeIndex - base + 1;
            const auto subTile = static_cast<unsigned char>(span->Height);
            const bool horizontal = (relative == IsometricTileTypeClass::BridgeTopLeft1 ||
                                     relative == IsometricTileTypeClass::BridgeTopLeft2) &&
                                    subTile == 8;
            const bool vertical = (relative == IsometricTileTypeClass::BridgeTopRight1 ||
                                   relative == IsometricTileTypeClass::BridgeTopRight2) &&
                                  subTile == 12;
            RectangleStruct area{};
            if (horizontal || vertical) {
                set_end(at, false);
                repair_span(map, fetch(at), horizontal ? 2 : 4, &area, wood);
                dirty(area);
                break;
            }
            const bool middle1 = relative >= IsometricTileTypeClass::BridgeMiddle1 &&
                                 relative <= IsometricTileTypeClass::BridgeMiddle1 + 4 && subTile == 5;
            const bool middle2 = relative >= IsometricTileTypeClass::BridgeMiddle2 &&
                                 relative <= IsometricTileTypeClass::BridgeMiddle2 + 4 && subTile == 7;
            if (middle1 || middle2) {
                const int middle =
                    middle1 ? IsometricTileTypeClass::BridgeMiddle1 : IsometricTileTypeClass::BridgeMiddle2;
                if (relative == middle + 4) {
                    set_middle(at, base + middle - 1);
                    const auto center = adjacent(at, middle1 ? 6 : 0);
                    raise(center, middle1);
                    reset = restore_connections(map, center);
                    collect(cells, center, middle1);
                }
                repair_bridge(map, middle1 ? offset(at, -2, 0) : offset(at, 0, -2), wood);
                repair_span(map, fetch(at), middle1 ? 2 : 4, &area, wood);
                dirty(area);
                break;
            }
        }
        at = adjacent(at, advance);
    }
    if (reset) map.ResetAllZones();
    if (cells.Count) map.Update_Pathfinding_2(cells);
}
}
void MapClass::RepairWoodBridgeAt(const CellStruct& cell) {
    try {
        repair_bridge(*this, cell, true);
    } catch (...) {
        std::abort();
    }
}
void MapClass::RepairConcreteBridgeAt(const CellStruct& cell) {
    try {
        repair_bridge(*this, cell, false);
    } catch (...) {
        std::abort();
    }
}
// Shared path-list invalidation is implemented in MapClassBridgeDamage.cpp.
#endif
