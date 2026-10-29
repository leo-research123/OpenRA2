// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp::Get_Z_Adjust and techno.cpp Z fudge helpers;
// calibrated to YR 0x4DAFC0, 0x703CC0, 0x703E70, 0x704000, 0x704240.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "techno_visual_queries.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include <algorithm>
namespace {
CellClass* neighbor(CellStruct at, FacingType direction) {
    constexpr short offsets[8][2] = {{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};
    const auto& offset = offsets[unsigned(direction)];
    return MapClass::Instance.TryGetCellAt(CellStruct{short(at.X + offset[0]), short(at.Y + offset[1])});
}
bool bridge_extension(const FootClass& actor) {
    const auto at = actor.GetMapCoords();
    const auto* cell = MapClass::Instance.TryGetCellAt(at);
    if (!cell || actor.OnBridge) return false;
    // 0x703CC0 fetches every cardinal neighbor before inspecting flags.
    const auto* south = neighbor(at, FacingType::South);
    const auto* north = neighbor(at, FacingType::North);
    const auto* east = neighbor(at, FacingType::East);
    const auto* west = neighbor(at, FacingType::West);
    if ((unsigned(cell->Flags) & 0x400u) != 0) return true;
    const auto extension = [](const CellClass* cell, bool vertical) {
        if (!cell) return false;
        const auto flags = unsigned(cell->Flags);
        return (flags & 0x400u) && bool(flags & 0x800u) == vertical;
    };
    return extension(south, true) || extension(west, false) || extension(east, false) || extension(north, true);
}
int column_fudge(const FootClass& actor) {
    const auto at = actor.GetMapCoords();
    if (!MapClass::Instance.TryGetCellAt(at) || (!actor.IsUnderBridge() && !bridge_extension(actor))) return 0;
    const CellClass* neighbors[]{neighbor(at, FacingType::South), neighbor(at, FacingType::East),
                                 neighbor(at, FacingType::SouthEast)};
    for (const auto* cell : neighbors) if (!cell) return 0;
    int result = 0;
    for (int i = 0; i < 3; ++i) {
        const int tile = neighbors[i]->IsoTileTypeIndex;
        if (tile == 0xFFFF || tile == 0xFF) continue;
        const int relative = tile - IsometricTileTypeClass::BridgeSet + 1;
        if (relative >= 7 && relative <= 16) {
            if (i == 2) ++result;
            else result = 1;
        }
    }
    return result;
}
int tunnel_fudge(const FootClass& actor) {
    const auto at = actor.GetMapCoords();
    if (actor.OnBridge) return 0;
    const auto* north = neighbor(at, FacingType::North);
    const auto* west = neighbor(at, FacingType::West);
    const auto* cell = MapClass::Instance.TryGetCellAt(at);
    if (!north || !west || !cell) return 0;
    const CellClass* mouth = cell->Tile_Is_Tunnel() ? cell : north->Tile_Is_Tunnel() ? north : west->Tile_Is_Tunnel() ? west : nullptr;
    if (!mouth) return 0;
    north = mouth->GetNeighbourCell(FacingType::North);
    west = mouth->GetNeighbourCell(FacingType::West);
    if (!north || !west) return 0;
    north = north->GetNeighbourCell(FacingType::North);
    west = west->GetNeighbourCell(FacingType::West);
    return north && west && (west->Tile_Is_Tunnel() || north->Tile_Is_Tunnel()) ? 1 : 0;
}
int cliff_fudge(const FootClass& actor) {
    const auto* cell = MapClass::Instance.TryGetCellAt(actor.GetMapCoords());
    if (actor.OnBridge) return 0;
    const auto* first = neighbor(actor.GetMapCoords(), FacingType::SouthEast);
    if (!first || !cell) return 0;
    int result = static_cast<signed char>(first->Level) - static_cast<signed char>(cell->Level) >= 4 ? 2 : 0;
    const auto* second = first->GetNeighbourCell(FacingType::SouthEast);
    if (second && static_cast<signed char>(second->Level) - static_cast<signed char>(cell->Level) >= 4) result = 1;
    return result;
}
}
int game::foot_z_adjustment(const FootClass& self, const TechnoTypeClass& type) {
    const int locomotor_z = self.Locomotor ? self.Locomotor->Z_Adjust() : 0;
    const int column = column_fudge(self) * type.ZFudgeColumn;
    const int tunnel = tunnel_fudge(self) * type.ZFudgeTunnel;
    const int cliff = cliff_fudge(self) * type.ZFudgeCliff;
    const int fudge = std::max({column, tunnel, cliff, self.IsUnderBridge() ? type.ZFudgeBridge : 0});
    return techno_z_adjustment(self, type) + locomotor_z + fudge;
}
int FootClass::GetZAdjustment() const {
    const int locomotor_z = Locomotor ? Locomotor->Z_Adjust() : 0;
    const auto* column_type = GetTechnoType();
    const int column = column_fudge(*this) * column_type->ZFudgeColumn;
    const auto* tunnel_type = GetTechnoType();
    const int tunnel = tunnel_fudge(*this) * tunnel_type->ZFudgeTunnel;
    const auto* cliff_type = GetTechnoType();
    const int cliff = cliff_fudge(*this) * cliff_type->ZFudgeCliff;
    const int bridge = IsUnderBridge() ? GetTechnoType()->ZFudgeBridge : 0;
    return locomotor_z + std::max({column, tunnel, cliff, bridge}) + TechnoClass::GetZAdjustment();
}
