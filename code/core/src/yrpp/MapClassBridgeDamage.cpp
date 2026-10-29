// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp high/low bridge damage and subzone disconnection.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md. Calibrated to YR.
#include "background_invalidation.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TagClass.h"
#include <algorithm>
#include <cstdlib>

#if !defined(RA2_YRPP_GAME)
namespace {
CellStruct offset(CellStruct c, int x, int y) { return {short(c.X + x), short(c.Y + y)}; }
CellStruct adjacent(CellStruct c, int facing) {
    const auto& d = CellSpread::GetNeighbourOffset(facing & 7);
    return offset(c, d.X, d.Y);
}
CellClass* fetch(CellStruct c) { return MapClass::Instance.GetCellAt(c); }
unsigned flags(const CellClass* c) { return unsigned(c->Flags); }
bool deck(const CellClass* c) { return (flags(c) & 0x80u) != 0; }
bool under(const CellClass* c) { return (flags(c) & 0x100u) != 0; }
bool former(const CellClass* c) { return (flags(c) & 0x400u) != 0; }
bool east_west(const CellClass* c) { return (flags(c) & 0x800u) != 0; }
int bridge_base(bool wood) { return wood ? IsometricTileTypeClass::WoodBridgeSet : IsometricTileTypeClass::BridgeSet; }
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
RectangleStruct overlay_rectangle(CellClass* c) {
    RectangleStruct shadow{}, body{};
    c->ShapeRect(&shadow);
    c->GetContainingRect(&body);
    return united(body, shadow);
}
void dirty(RectangleStruct r) { TacticalClass::Instance->RegisterDirtyArea(r, false); }
void radar(CellStruct c) { RadarClass::Instance.RadarBackground(c); }
void overlay_dirty(RectangleStruct before, CellClass* c) {
    auto r = united(before, overlay_rectangle(c));
    r.X -= DSurface::ViewBounds.X;
    r.Y -= DSurface::ViewBounds.Y;
    dirty(r);
}
void recalc_rectangle(MapClass& map, RectangleStruct r) {
    DynamicVectorClass<CellStruct> cells;
    for (int x = r.X; x < r.X + r.Width; ++x)
        for (int y = r.Y; y < r.Y + r.Height; ++y)
            cells.AddItem({short(x), short(y)});
    map.Update_Pathfinding_2(cells);
}
// 0x0056E990 / 0x0056EB80: spread within the original tile cluster.
void set_end(CellStruct at, bool damaged, bool recursive) {
    auto* c = fetch(at);
    if (!recursive) {
        const int tile = c->IsoTileTypeIndex;
        if (tile == 0xFFFF || tile == 255 ||
            !IsometricTileTypeClass::Array[tile]->IsRandomized(static_cast<unsigned char>(c->Height)))
            return;
        CoordStruct coord{int(at.X) * 256 + 128, int(at.Y) * 256 + 128, 0};
        Point2D p{};
        TacticalClass::Instance->CoordsToClient(&coord, &p);
        p.Y -= 15 * c->Level;
        dirty({p.X - 128, p.Y - 128, 256, 256});
    }
    if (damaged == ((flags(c) & 0x2000u) != 0)) return;
    const int old = c->IsoTileTypeIndex;
    c->Flags = static_cast<CellFlags>((flags(c) & ~0x2000u) | (damaged ? 0x2000u : 0u));
    // Recursive clusters can extend beyond the original root rectangle.
    game::invalidate_background_cell(c->MapCoords, 0x1F);
    radar(c->MapCoords);
    for (int i = 0; i < 8; ++i) {
        const auto n = adjacent(at, i);
        if (fetch(n)->IsoTileTypeIndex == old) set_end(n, damaged, true);
    }
}
void set_middle(CellStruct at, int tile, int match, int level, bool recursive) {
    auto* c = fetch(at);
    if (!recursive) {
        CoordStruct coord{int(at.X) * 256 + 128, int(at.Y) * 256 + 128, 0};
        Point2D p{};
        TacticalClass::Instance->CoordsToClient(&coord, &p);
        p.Y -= 15 * c->Level;
        dirty({p.X - 128, p.Y - 128, 256, 256});
        match = c->IsoTileTypeIndex;
        if (tile == match) return;
    }
    if (c->IsoTileTypeIndex == tile) return;
    c->IsoTileTypeIndex = tile;
    c->RecalcAttributes(level);
    radar(c->MapCoords);
    for (int i = 0; i < 8; ++i) {
        const auto n = adjacent(at, i);
        if (fetch(n)->IsoTileTypeIndex == match) set_middle(n, tile, match, level, true);
    }
}
// 0x00584E50 removes the first matching edge, including its cross-block bit.
void unregister_connection(MapClass& map, const ZoneConnectionClass& c) {
    CellStruct from1 = CellStruct::Empty, from2 = CellStruct::Empty;
    CellStruct to1 = CellStruct::Empty, to2 = CellStruct::Empty;
    auto* from = fetch(c.FromMapCoords);
    if (from->Tile_Is_Bridge() || from->Tile_Is_WoodBridge()) {
        static constexpr int sides[]{0, 0, -1, 2, 2, -1, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2};
        const int base = from->Tile_Is_Bridge() ? bridge_base(false) : bridge_base(true);
        const int side = sides[from->IsoTileTypeIndex - base];
        from1 = adjacent(c.FromMapCoords, side);
        from2 = adjacent(c.FromMapCoords, side - 4);
        to1 = adjacent(c.ToMapCoords, side);
        to2 = adjacent(c.ToMapCoords, side - 4);
    }
    for (int level = 0; level < 3; ++level) {
        const auto unlink = [&](CellStruct a, CellStruct b) {
            const int first = map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex(a)].SubzoneIDs[level];
            const int second = map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex(b)].SubzoneIDs[level];
            auto& v = map.SubzoneTracking[level];
            v[first].SubzoneConnections.Remove({static_cast<unsigned>(second), false});
            v[second].SubzoneConnections.Remove({static_cast<unsigned>(first), false});
        };
        unlink(c.FromMapCoords, c.ToMapCoords);
        unlink(from1, to1);
        unlink(from2, to2);
    }
}
bool unregister_connections(MapClass& map, CellStruct at) {
    int index = map.ZoneConnectionIndex(at, 3, 0);
    if (index == -1) {
        map.ComputeZoneConnections();
        index = map.ZoneConnectionIndex(at, 3, 0);
        if (index == -1) return false;
    }
    bool changed = false;
    do {
        auto& c = map.ZoneConnections[index];
        if (c.IsPassable) {
            unregister_connection(map, c);
            c.IsPassable = false;
            changed = true;
        }
        index = map.ZoneConnectionIndex(at, 3, index + 1);
    } while (index != -1);
    return changed;
}
// 0x00575EE0: exclude the forward_side endpoint, visiting all four deck-width cells.
void spring_bridge(CellStruct first, CellStruct last) {
    const bool horizontal = first.Y == last.Y;
    if (horizontal ? first.X > last.X : first.Y > last.Y) std::swap(first, last);
    const auto spring = [](CellClass* c) {
        if (c->AttachedTag)
            c->AttachedTag->RaiseEvent(TriggerEvent::AllBridgesDestroyed, nullptr, CellStruct::Empty, false, nullptr);
    };
    for (auto at = first; at != last; at = offset(at, horizontal ? 1 : 0, horizontal ? 0 : 1)) {
        spring(fetch(at));
        spring(fetch(adjacent(at, horizontal ? 4 : 2)));
        auto* second = fetch(adjacent(at, horizontal ? 0 : 6));
        spring(second);
        spring(fetch(adjacent(second->MapCoords, horizontal ? 0 : 6)));
    }
}
bool low(CellStruct at, bool wood) {
    const int value = fetch(at)->OverlayTypeIndex, base = wood ? 74 : 205;
    return value >= base && value <= base + 27;
}
int low_mask(CellStruct at, bool wood, bool horizontal) {
    const int base = wood ? 74 : 205;
    int forward = fetch(offset(at, horizontal ? 1 : 0, horizontal ? 0 : -1))->OverlayTypeIndex - base;
    int backward = fetch(offset(at, horizontal ? -1 : 0, horizontal ? 0 : 1))->OverlayTypeIndex - base;
    const int start = horizontal ? 0 : 9, gap = horizontal ? 26 : 27;
    int mask = 0;
    if (forward == start + 4 || forward == start + 6 || forward == start + 8 || forward == (horizontal ? 19 : 23))
        mask |= 1;
    if (forward == start + 7 || forward == gap) mask |= 2;
    if (backward == start + 5 || backward == start + 6 || backward == start + 7 || backward == (horizontal ? 21 : 25))
        mask |= 4;
    if (backward == start + 8 || backward == gap) mask |= 8;
    return mask;
}
// 0x0057DD50 / 0x0057E2A0 / 0x0057E7A0 / 0x0057ED00.
void damage_low_piece(CellStruct at, bool wood, bool horizontal) {
    if (!low(at, wood)) return;
    auto* center = fetch(at);
    auto* first = fetch(offset(at, horizontal ? 0 : -1, horizontal ? -1 : 0));
    auto* second = fetch(offset(at, horizontal ? 0 : 1, horizontal ? 1 : 0));
    const auto before = overlay_rectangle(center);
    const int mask = low_mask(at, wood, horizontal);
    const int base = wood ? 74 : 205, start = horizontal ? 0 : 9, gap = horizontal ? 26 : 27;
    const int current = center->OverlayTypeIndex - base;
    if (mask <= 0) return;
    static constexpr int lookup[]{-1, 5, 8, -1, 4, 6, 8, -1, 7, 7, 26, -1, -1, -1, -1, -1};
    int value;
    const int head = horizontal ? 18 : 22;
    if (current >= head) {
        if (current == head || current == head + 2)
            value = base + current + 1;
        else
            return;
    } else {
        const int relative = lookup[mask];
        value = relative == -1 ? -1 : relative == 26 ? base + gap : base + start + relative;
        if (value == center->OverlayTypeIndex) return;
    }
    center->OverlayTypeIndex = first->OverlayTypeIndex = second->OverlayTypeIndex = value;
    overlay_dirty(before, center);
    const bool destroyed = value == base + gap;
    if (destroyed) {
        radar(at);
        radar(second->MapCoords);
        radar(first->MapCoords);
    }
    center->RecalcAttributes(-1);
    first->RecalcAttributes(-1);
    second->RecalcAttributes(-1);
    center->KillIllegalOccupiers(destroyed);
    first->KillIllegalOccupiers(destroyed);
    second->KillIllegalOccupiers(destroyed);
}
void spring_low(CellStruct at, bool wood, bool horizontal) {
    auto first = at, last = at;
    do {
        first = offset(first, horizontal ? -1 : 0, horizontal ? 0 : -1);
    } while (low(first, wood));
    first = offset(first, horizontal ? 1 : 0, horizontal ? 0 : 1);
    do {
        last = offset(last, horizontal ? 1 : 0, horizontal ? 0 : 1);
    } while (low(last, wood));
    last = offset(last, horizontal ? -1 : 0, horizontal ? 0 : -1);
    spring_bridge(first, last);
}
bool damage_low_span(MapClass& map, CellStruct at, bool wood, bool horizontal) {
    auto* center = fetch(at);
    auto* first = fetch(offset(at, horizontal ? 0 : -1, horizontal ? -1 : 0));
    auto* second = fetch(offset(at, horizontal ? 0 : 1, horizontal ? 1 : 0));
    const int base = wood ? 74 : 205, start = horizontal ? 0 : 9, gap = horizontal ? 26 : 27;
    const int current = center->OverlayTypeIndex - base, head = horizontal ? 18 : 22;
    const auto before = overlay_rectangle(center);
    bool destroyed = false;
    int value;
    if (current == head || current == head + 2)
        value = base + current + 1;
    else if (current < start + 6)
        value = base + start + 6;
    else if (current < start + 9) {
        value = base + gap;
        destroyed = true;
    } else
        return false;
    center->OverlayTypeIndex = first->OverlayTypeIndex = second->OverlayTypeIndex = value;
    if (destroyed) {
        radar(first->MapCoords);
        radar(second->MapCoords);
        radar(at);
    }
    const auto preceding = offset(at, horizontal ? -1 : 0, horizontal ? 0 : -1);
    const auto following = offset(at, horizontal ? 1 : 0, horizontal ? 0 : 1);
    if (current != head + 2) damage_low_piece(preceding, wood, horizontal);
    if (current != head) damage_low_piece(following, wood, horizontal);
    if (destroyed) spring_low(at, wood, horizontal);
    overlay_dirty(before, center);
    center->RecalcAttributes(-1);
    first->RecalcAttributes(-1);
    second->RecalcAttributes(-1);
    center->KillIllegalOccupiers(destroyed);
    first->KillIllegalOccupiers(destroyed);
    second->KillIllegalOccupiers(destroyed);
    if (destroyed) {
        map.ResetAllZones();
        recalc_rectangle(map, {at.X - 1, at.Y - 1, 3, 3});
    }
    return destroyed;
}
bool damage_low(MapClass& map, CellStruct at, bool wood) {
    const int relative = fetch(at)->OverlayTypeIndex - (wood ? 74 : 205);
    const bool horizontal = (relative >= 0 && relative <= 8) || (relative >= 18 && relative <= 21) || relative == 26;
    const bool vertical = (relative >= 9 && relative <= 17) || (relative >= 22 && relative <= 25) || relative == 27;
    if (!horizontal && !vertical) return false;
    const auto before = offset(at, horizontal ? 0 : -1, horizontal ? -1 : 0);
    if (!low(before, wood))
        at = offset(at, horizontal ? 0 : 1, horizontal ? 1 : 0);
    else if (low(offset(before, horizontal ? 0 : -1, horizontal ? -1 : 0), wood))
        at = before;
    return damage_low_span(map, at, wood, horizontal);
}

int relative(CellClass* c, bool wood) { return c->IsoTileTypeIndex - bridge_base(wood) + 1; }
bool middle(int tile, int first) { return tile >= first && tile <= first + 3; }
void clear_deck(CellClass* c, bool wood, bool horizontal) {
    const auto dir = horizontal ? FacingType::North : FacingType::West;
    if (wood)
        c->SetWoodBridge(dir, false);
    else
        c->SetBridge(dir, false);
    c->OverlayData = 0;
    c->OverlayTypeIndex = -1;
}
// The four original side routines differ in orientation and transition/end frame.
void high_side(CellStruct at, int dir, bool wood, bool horizontal, bool forward_side, bool destroy) {
    at = adjacent(at, dir);
    auto* c = fetch(at);
    const int start = horizontal ? 0 : 9;
    const int transition = forward_side ? (horizontal ? 4 : 5) : (horizontal ? 5 : 4);
    const int end = forward_side ? (horizontal ? 7 : 8) : (horizontal ? 8 : 7);
    if (deck(c) && (horizontal || c->OverlayData >= start)) {
        const int stage = c->OverlayData - start;
        if (!destroy) {
            if (stage <= 3)
                c->OverlayData = start + transition;
            else if (stage == (transition == 4 ? 5 : 4))
                c->OverlayData = start + 6;
        } else if (stage <= 6)
            c->OverlayData = start + end;
        else if (stage == (end == 7 ? 8 : 7)) {
            high_side(at, dir, wood, horizontal, forward_side, true);
            clear_deck(c, wood, horizontal);
            radar(c->MapCoords);
        }
    }
    const int tile = relative(c, wood);
    const int first = horizontal ? IsometricTileTypeClass::BridgeMiddle1 : IsometricTileTypeClass::BridgeMiddle2;
    const int end1 =
        horizontal
            ? (forward_side ? IsometricTileTypeClass::BridgeBottomRight1 : IsometricTileTypeClass::BridgeTopLeft1)
            : (forward_side ? IsometricTileTypeClass::BridgeBottomLeft1 : IsometricTileTypeClass::BridgeTopRight1);
    const int end2 =
        horizontal
            ? (forward_side ? IsometricTileTypeClass::BridgeBottomRight2 : IsometricTileTypeClass::BridgeTopLeft2)
            : (forward_side ? IsometricTileTypeClass::BridgeBottomLeft2 : IsometricTileTypeClass::BridgeTopRight2);
    if (tile == end1 || tile == end2) {
        set_end(at, true, false);
        return;
    }
    if (!destroy) {
        if (tile == first || tile == first + (forward_side ? 2 : 1))
            set_middle(at,
                       bridge_base(wood) + first + (forward_side ? (tile == first ? 0 : 2) : (tile == first ? 1 : 2)),
                       -1, -1, false);
    } else if (tile == first || tile == first + (forward_side ? 2 : 1)) {
        set_middle(at, bridge_base(wood) + first + 2, -1, -1, false);
    } else if (tile == first + 3) {
        high_side(at, dir, wood, horizontal, forward_side, true);
        auto origin = offset(at, horizontal && (c->Height & 1) ? -1 : 0, !horizontal && c->Height > 4 ? -1 : 0);
        // The original side routines visit their three occupants in this order.
        if (horizontal) {
            fetch(origin)->BlowUpBridge();
            fetch(offset(origin, 0, -1))->BlowUpBridge();
            fetch(offset(origin, 0, 1))->BlowUpBridge();
        } else {
            fetch(offset(origin, -1, 0))->BlowUpBridge();
            fetch(origin)->BlowUpBridge();
            fetch(offset(origin, 1, 0))->BlowUpBridge();
        }
        set_middle(at, bridge_base(wood) + first + 3, -1, fetch(origin)->Level - 4, false);
    }
}
bool destroy_span(CellStruct at, int dir, bool wood, RectangleStruct* area) {
    auto* c = fetch(at);
    int count = 1;
    const auto first = adjacent(c->MapCoords, dir);
    auto last = first;
    const bool horizontal = dir == 2;
    const int mid = horizontal ? IsometricTileTypeClass::BridgeMiddle1 : IsometricTileTypeClass::BridgeMiddle2;
    const int end1 =
        horizontal ? IsometricTileTypeClass::BridgeBottomRight1 : IsometricTileTypeClass::BridgeBottomLeft1;
    const int end2 =
        horizontal ? IsometricTileTypeClass::BridgeBottomRight2 : IsometricTileTypeClass::BridgeBottomLeft2;
    for (;;) {
        last = adjacent(c->MapCoords, dir);
        c = fetch(last);
        const int t = relative(c, wood);
        if ((t == end1 || t == end2 || middle(t, mid)) && c->Height == (horizontal ? 4 : 2)) break;
        if (++count >= 30) break;
    }
    if (count == 30) return false;
    c = fetch(at);
    if (area) {
        CoordStruct a{at.X * 256 + 128, at.Y * 256 + 128, Unsorted::LevelHeight * c->Level};
        CoordStruct b{last.X * 256 + 128, last.Y * 256 + 128, a.Z};
        Point2D pa{}, pb{};
        TacticalClass::Instance->CoordsToClient(&a, &pa);
        TacticalClass::Instance->CoordsToClient(&b, &pb);
        *area = united(*area, {std::min(pa.X, pb.X) - 64, std::min(pa.Y, pb.Y) - 64, std::abs(pa.X - pb.X) + 128,
                               std::abs(pa.Y - pb.Y) + 128});
    }
    bool was_owner = false, sprung = false;
    CellStruct owner{-1, -1};
    for (int i = 0; i < count; ++i) {
        if (deck(c) && was_owner) owner = c->MapCoords;
        if (!deck(c) && !was_owner && owner != CellStruct{-1, -1}) {
            auto* behind = fetch(adjacent(c->MapCoords, dir - 4));
            clear_deck(behind, wood, horizontal);
            radar(behind->MapCoords);
            destroy_span(at, dir, wood, area);
            return true;
        }
        was_owner = !deck(c);
        if (was_owner && !sprung) {
            spring_bridge(first, last);
            sprung = true;
        }
        c = fetch(adjacent(c->MapCoords, dir));
    }
    return false;
}
void destroy_connections(MapClass& map, CellStruct at, bool wood) {
    CellClass* c = &MapClass::InvalidCell;
    for (int i = 0; i < 8; ++i) {
        c = fetch(adjacent(at, i));
        if (flags(c) & 0x500u) break;
    }
    if (!(flags(c) & 0x500u)) return;
    CellStruct origin;
    if (under(c))
        origin = deck(c) ? c->MapCoords : c->BridgeOwnerCell->MapCoords;
    else {
        auto walk = c->MapCoords;
        int count = 0;
        const int step = east_west(c) ? 4 : 2;
        for (;;) {
            walk = adjacent(walk, step);
            if (!former(fetch(walk))) break;
            if (++count >= 4) return;
        }
        origin = adjacent(adjacent(walk, step - 4), step - 4);
    }
    const int advance = east_west(c) ? 6 : 0;
    RectangleStruct area{};
    for (auto current = origin;; current = adjacent(current, advance)) {
        if (wood) {
            const auto& r = map.MapCoordBounds;
            if (current.X < r.Left || current.X > r.Left + r.Right || current.Y < r.Top || current.Y > r.Top + r.Bottom)
                return;
        } else if (!MapClass::Instance.CoordinatesLegal(current))
            return;
        const int index = MapClass::GetCellIndex(current);
        if (!map.Cells.Items[index]) continue;
        auto* span = fetch(current);
        const int t = relative(span, wood);
        int dir = -1;
        if (((t == IsometricTileTypeClass::BridgeTopLeft1 || t == IsometricTileTypeClass::BridgeTopLeft2) &&
             span->Height == 8) ||
            (middle(t, IsometricTileTypeClass::BridgeMiddle1) && span->Height == 5))
            dir = 2;
        if (((t == IsometricTileTypeClass::BridgeTopRight1 || t == IsometricTileTypeClass::BridgeTopRight2) &&
             span->Height == 12) ||
            (middle(t, IsometricTileTypeClass::BridgeMiddle2) && span->Height == 7))
            dir = 4;
        if (dir != -1) {
            if (destroy_span(current, dir, wood, &area) && area.Width > 0 && area.Height > 0) dirty(area);
            return;
        }
    }
}
bool damage_high(MapClass& map, CellStruct at, bool wood) {
    auto* c = fetch(at);
    const int tile = relative(c, wood);
    const int mid1 = IsometricTileTypeClass::BridgeMiddle1, mid2 = IsometricTileTypeClass::BridgeMiddle2;
    if (!under(c)) {
        const bool horizontal = middle(tile, mid1);
        if (!horizontal && !middle(tile, mid2)) return false;
        int sub = c->Height;
        if (horizontal ? (sub & 1) : sub > 4) return false;
        auto center = c->MapCoords;
        const int target = horizontal ? 4 : 2;
        while (sub != target) {
            center = adjacent(center, horizontal ? (sub < target ? 4 : 0) : (sub < target ? 2 : 6));
            sub = fetch(center)->Height;
        }
        c = fetch(center);
        const int mid = horizontal ? mid1 : mid2, dir = horizontal ? 2 : 4;
        if (tile == mid + 3) {
            auto zone = c->MapCoords;
            if (horizontal ? (c->Height & 1) : c->Height > 4)
                zone = offset(zone, horizontal ? -1 : 0, horizontal ? 0 : -1);
            // Root routines visit the width from its negative edge.
            for (int i = -1; i <= 1; ++i)
                fetch(offset(zone, horizontal ? 0 : i, horizontal ? i : 0))->BlowUpBridge();
            set_middle(center, bridge_base(wood) + mid + 3, -1, fetch(zone)->Level - 4, false);
            high_side(center, dir, wood, horizontal, true, true);
            high_side(center, dir - 4, wood, horizontal, false, true);
            destroy_connections(map, adjacent(center, dir - 4), wood);
            destroy_connections(map, adjacent(center, dir), wood);
            if (unregister_connections(map, zone)) map.ResetAllZones();
            DynamicVectorClass<CellStruct> cells;
            for (int major = 0; major < 2; ++major)
                for (int minor = -2; minor < 3; ++minor)
                    cells.AddItem(offset(zone, horizontal ? major : minor, horizontal ? minor : major));
            map.Update_Pathfinding_2(cells);
            // YR's wood variant returns false even after lowering a TMP span.
            return !wood;
        }
        set_middle(center, bridge_base(wood) + mid + 2, -1, -1, false);
        high_side(center, dir, wood, horizontal, true, false);
        high_side(center, dir - 4, wood, horizontal, false, false);
        return false;
    }
    if (!deck(c)) c = c->BridgeOwnerCell;
    const int stage = c->OverlayData;
    const bool horizontal = stage < 9;
    const int rel = horizontal ? stage : stage - 9, dir = horizontal ? 2 : 4;
    if (rel >= 0 && rel <= 5) {
        c->OverlayData = horizontal ? 6 : 15;
        high_side(c->MapCoords, dir, wood, horizontal, true, false);
        high_side(c->MapCoords, dir - 4, wood, horizontal, false, false);
        return false;
    }
    if (rel < 6 || rel > 8) return false;
    if (rel == 6 || rel == (horizontal ? 7 : 8)) high_side(c->MapCoords, dir, wood, horizontal, true, true);
    if (rel == 6 || rel == (horizontal ? 8 : 7)) high_side(c->MapCoords, dir - 4, wood, horizontal, false, true);
    clear_deck(c, wood, horizontal);
    destroy_connections(map, at, wood);
    if (unregister_connections(map, c->MapCoords)) map.ResetAllZones();
    return true;
}

}  // namespace

bool MapClass::DamageLowBridgeAt(const CellStruct& at) {
    try {
        return damage_low(*this, at, true);
    } catch (...) {
        std::abort();
    }
}
bool MapClass::DamageLowWoodBridgeAt(const CellStruct& at) {
    try {
        return damage_low(*this, at, false);
    } catch (...) {
        std::abort();
    }
}
bool MapClass::DamageBridgeAt(const CellStruct& at) {
    try {
        CellStructs1.Clear();
        auto* c = fetch(at);
        const int overlay = c->OverlayTypeIndex;
        bool result = false;
        if (overlay >= 74 && overlay <= 99)
            result = damage_low(*this, at, true);
        else if (overlay >= 205 && overlay <= 230)
            result = damage_low(*this, at, false);
        else {
            if (under(c)) c = fetch((deck(c) ? c : c->BridgeOwnerCell)->MapCoords);
            const int concrete = relative(c, false), wood = relative(c, true);
            const int m1 = IsometricTileTypeClass::BridgeMiddle1, m2 = IsometricTileTypeClass::BridgeMiddle2;
            if (c->OverlayTypeIndex == 24 || c->OverlayTypeIndex == 25 || middle(concrete, m1) || middle(concrete, m2))
                result = damage_high(*this, c->MapCoords, false);
            else if (c->OverlayTypeIndex == 237 || c->OverlayTypeIndex == 238 || middle(wood, m1) || middle(wood, m2))
                result = damage_high(*this, c->MapCoords, true);
        }
        for (int i = 0; i < CellStructs1.Count; ++i)
            fetch(CellStructs1[i])->OnBridgeCollapse();
        CellStructs1.Clear();
        return result;
    } catch (...) {
        std::abort();
    }
}
// YR 0x00586990: invalidate attributes first, rebuild affected subzones second.
void MapClass::Update_Pathfinding_2(const DynamicVectorClass<CellStruct>& where) {
    for (int i = where.Count - 1; i >= 0; --i)
        if (IsWithinUsableArea(where[i], true)) {
            LevelAndPassabilityStruct2pointer_70[GetCellZoneIndex(where[i])].SubzoneIDs[0] = 0;
            MapClass::Instance.GetCellAt(where[i])->RecalcAttributes(-1);
        }
    for (int i = where.Count - 1; i >= 0; --i)
        if (IsWithinUsableArea(where[i], true) &&
            !LevelAndPassabilityStruct2pointer_70[GetCellZoneIndex(where[i])].SubzoneIDs[0])
            RecalculateSubZones(where[i]);
}
#endif
