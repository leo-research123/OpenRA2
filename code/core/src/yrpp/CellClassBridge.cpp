// Cell-owned overlay placement, following the existing CellClass model.
// YR 0x0047E040 / 0x0047E470, intact map-loading branch. The two routines
// have identical state writes; destruction's BlowUpBridge call is out of scope.
#include "yrpp/CellClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include <cstdint>
#include <bit>
#include "yrpp/TubeClass.h"

bool CellClass::Tile_Is_Tunnel() const { return TubeIndex>=0 && TubeIndex<TubeClass::Array.Count && LandType==::LandType::Tunnel; }
// OpenTS 44fac744 cell.cpp Is_Near_Tunnel_NW; YR 0x484AE0.
bool CellClass::IsNearTunnelNW() const {
    if(Tile_Is_Tunnel())return true;
    for(const auto direction:{FacingType::North,FacingType::West}){
        const auto* first=GetNeighbourCell(direction);if(!first)continue;
        const auto* second=first->GetNeighbourCell(direction);if(!second)continue;
        if(first->Tile_Is_Tunnel()&&!second->Tile_Is_Tunnel())return true;
        const auto* third=second->GetNeighbourCell(direction);
        if(third&&second->Tile_Is_Tunnel()&&!third->Tile_Is_Tunnel())return true;
    }
    return false;
}
bool CellClass::Tile_Is_DestroyableCliff() const {
    const int first=IsometricTileTypeClass::DestroyableCliffs;
    return IsoTileTypeIndex==first || IsoTileTypeIndex==std::bit_cast<int>(static_cast<unsigned>(first)+1u);
}

bool CellClass::Tile_Is_Bridge() const {
    const int first=IsometricTileTypeClass::BridgeSet;
    return first!=-1 && IsoTileTypeIndex>=first && IsoTileTypeIndex<std::bit_cast<int>(static_cast<unsigned>(first)+16u);
}
bool CellClass::Tile_Is_WoodBridge() const {
    const int first=IsometricTileTypeClass::WoodBridgeSet;
    return first!=-1 && IsoTileTypeIndex>=first && IsoTileTypeIndex<std::bit_cast<int>(static_cast<unsigned>(first)+16u);
}

void CellClass::InitializeBridge(FacingType direction) noexcept {
    if (direction != FacingType::North && direction != FacingType::West) return;
    const auto orientation = direction == FacingType::North ? 0x800u : 0u;
    const auto frame = direction == FacingType::North ? 0 : 9;
    auto update = [orientation, frame](CellClass& cell, unsigned clear, unsigned set) {
        cell.Flags = static_cast<CellFlags>((static_cast<unsigned>(cell.Flags) & ~clear) | set | orientation);
        cell.OverlayData = static_cast<unsigned char>(frame);
    };
    // Owner, two cells along the width, and the cell behind the owner.
    // Preserve unrelated flags and the original owner pointer on the owner.
    update(*this, 0x11F80u, 0x11380u);
    auto* next = GetNeighbourCell(direction);
    next->BridgeOwnerCell = this;
    update(*next, 0x11F00u, 0x11300u);
    next = next->GetNeighbourCell(direction);
    next->BridgeOwnerCell = this;
    update(*next, 0x11F00u, 0x11100u);
    next = next->GetNeighbourCell(direction);
    next->Flags = static_cast<CellFlags>(static_cast<unsigned>(next->Flags) | 0x1000u);
    auto* behind = GetNeighbourCell(static_cast<FacingType>((static_cast<unsigned>(direction) + 4u) & 7u));
    behind->BridgeOwnerCell = this;
    update(*behind, 0x11F00u, 0x10300u);
    if (direction == FacingType::West) {
        auto* edge = behind->GetNeighbourCell(FacingType::East);
        edge->BridgeOwnerCell = this;
        edge->Flags = static_cast<CellFlags>(static_cast<unsigned>(edge->Flags) | 0x10000u);
    }
}
