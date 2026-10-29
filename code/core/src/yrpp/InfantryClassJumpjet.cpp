// YR 0x522A60: INoticeSink + 0x8 receives factory-clear and crash notices.
#include "yrpp/InfantryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/AStarClass.h"
#include <algorithm>
bool InfantryClass::INoticeSink_Unknown(DWORD code, DWORD) {
    if ((Type->BalloonHover || Type->JumpJet) && code == 0x117B && HasAnyLink()) {
        SendToFirstLink(RadioCommand::NotifyUnloaded);
        if (!ArchiveTarget || ArchiveTarget == Destination) {
            AbortMotion();
            Scatter(CoordStruct::Empty, true, false);
        } else
            SetDestination(ArchiveTarget, true);
        return true;
    }
    if (Type->Crashable && code == 0x117C) {
        SetHeight(GetCell()->ContainsBridge() ? 4 * Unsorted::LevelHeight : 0);
        PlayAnim(Sequence(36), true, false);
        return true;
    }
    return false;
}

// YR 0x5221D0: original height, tunnel, zone and short-path flight decision.
bool InfantryClass::ShouldJumpJetFly(const CellStruct& from, const CellStruct& to) {
    GetHeight();  // The original performs this virtual query twice.
    if (GetHeight() > 0) return true;
    auto& map = MapClass::Instance;
    if (Owner->IsControlledByHuman())
        if (auto* cell = map.GetCellAt(from); cell && cell->IsNearTunnelNW()) return false;
    const bool bridge = map.GetCellAt(to)->ContainsBridge();
    if (!MapClass::IsSameCellZone(from, to, MovementZone(7), OnBridge, bridge, false)) return true;
    if (from == to) return false;
    const int distance = std::max(std::abs(int(to.X) - from.X), std::abs(int(to.Y) - from.Y));
    if (distance == 1) return false;
    if (distance >= 4 || !map.IsWithinUsableArea(from, true) || !map.IsWithinUsableArea(to, true)) return true;
    auto start = from, end = to;
    return AStarClass::Instance.AttemptPath(&start, &end, this, OnBridge, bridge, MovementZone(7)) > 7;
}
