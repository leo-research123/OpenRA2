// YR 0x00587410: bridge-hut cursor query, including the complete linked span.
#include "yrpp/MapClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/IsometricTileTypeClass.h"

#if !defined(RA2_YRPP_GAME)
bool MapClass::IsLinkedBridgeDestroyed(const CellStruct& cell) const {
    CellClass* candidate = nullptr;
    bool high = false, concrete = false;
    int variant = -1;
    auto& map = MapClass::Instance;
    // The original scans Y first and retains the last matching cell.
    for (int y = -2; y < 3; ++y) {
        for (int x = -2; x < 3; ++x) {
            const CellStruct at{short(cell.X + x), short(cell.Y + y)};
            auto* current = map.GetCellAt(at);
            const int tile = current->IsoTileTypeIndex;
            if (tile >= IsometricTileTypeClass::WoodBridgeSet && tile < IsometricTileTypeClass::WoodBridgeSet + 16) {
                candidate = current;
                high = true;
                variant = tile - IsometricTileTypeClass::WoodBridgeSet;
            } else if (tile >= IsometricTileTypeClass::BridgeSet && tile < IsometricTileTypeClass::BridgeSet + 16) {
                candidate = current;
                high = true;
                variant = tile - IsometricTileTypeClass::BridgeSet;
            } else {
                const int overlay = current->OverlayTypeIndex;
                if (overlay >= 74 && overlay <= 101) {
                    candidate = current;
                    high = false;
                    concrete = false;
                } else if (overlay >= 205 && overlay <= 232) {
                    candidate = current;
                    high = false;
                    concrete = true;
                }
            }
        }
    }
    if (high) {
        // Original tables at 0x0082AA04, 0x0082AA24 and 0x0082AA44.
        static constexpr short xs[]{1, 1, 0, 2, 2, 2, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2};
        static constexpr short ys[]{2, 2, 2, 1, 1, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0};
        static constexpr int directions[]{2, 2, 6, 4, 4, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
        const int columns = IsometricTileTypeClass::Array[candidate->IsoTileTypeIndex]->unk_2E4;
        const int subTile = static_cast<unsigned char>(candidate->Height);
        CellStruct endpoint{short(candidate->MapCoords.X - subTile % columns + xs[variant]),
                            short(candidate->MapCoords.Y - subTile / columns + ys[variant])};
        CellStruct search = endpoint;
        const int direction = directions[variant];
        if (direction == -1) return false;
        for (;;) {
            const int index = ZoneConnectionIndex(search, 3, 0);
            if (index < 0) return false;
            const auto& connection = ZoneConnections[index];
            if (!connection.IsPassable) return true;
            if (connection.FromMapCoords == endpoint)
                endpoint = connection.ToMapCoords;
            else if (connection.ToMapCoords == endpoint)
                endpoint = connection.FromMapCoords;
            else
                return true;
            const auto& offset = CellSpread::GetNeighbourOffset(direction & 7);
            search = {short(endpoint.X + offset.X), short(endpoint.Y + offset.Y)};
        }
    }
    if (!candidate) return false;
    const int overlay = candidate->OverlayTypeIndex;
    const int first = concrete ? 205 : 74;
    const int last = concrete ? 232 : 101;
    const int relative = overlay - first;
    const bool horizontal = (relative >= 0 && relative <= 8) || (relative >= 18 && relative <= 21) || relative == 26;
    // Horizontal scans west then east; vertical scans south then north.
    for (int side = 0; side < 2; ++side) {
        CellStruct at = candidate->MapCoords;
        for (;;) {
            const int value = map.GetCellAt(at)->OverlayTypeIndex;
            if (value < first || value > last) break;
            if (value == last - 1 || value == last) return true;
            if (horizontal)
                at.X = short(at.X + (side ? 1 : -1));
            else
                at.Y = short(at.Y + (side ? -1 : 1));
        }
    }
    return false;
}
#endif
