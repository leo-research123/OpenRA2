#include "yrpp/AircraftClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/WeaponTypeClass.h"

int AircraftClass::Mission_SpyPlaneApproach() {
    const int distance = DistanceFrom(Target);
    if (Target) {
        if (Destination) {
            if (distance <= GetWeapon(0)->WeaponType->Range) {
                vt_entry_48C(false, 0, false, nullptr);
                UpdateSight(false, 0, false, nullptr, GetWeapon(0)->WeaponType->Damage);
                auto at = Location;
                MapClass::Instance.RevealArea3(&at, 0, LastSightRange + 3, false);
                at = Location;
                VocClass::PlayAt(RulesClass::Instance->SpyPlaneCamera, at);
            }
        } else {
            SetDestination(Target, true);
        }
    } else {
        SetDestination(nullptr, true);
        QueueMission(Mission::Retreat, false);
    }
    if (distance <= 768) {
        QueueMission(Mission::SpyplaneOverfly, false);
        IsLocked = true;
        // 0x50DAC0 uses the opposite house Edge, independently of StartingEdge.
        const auto edge = Owner->Edge == Edge::North ? Edge::South : Owner->Edge == Edge::East ? Edge::West :
                          Owner->Edge == Edge::West ? Edge::East : Edge::North;
        CellStruct at;
        MapClass::Instance.PickCellOnEdge(at, edge, CellStruct::Empty, CellStruct::Empty, SpeedType::Winged,
                                        true, MovementZone::Normal);
        if (at != CellStruct::Empty) SetDestination(MapClass::Instance.GetCellAt(at), true);
    }
    return RulesClass::Instance->SpyPlaneCameraFrames;
}

int AircraftClass::Mission_SpyPlaneOverfly() {
    const int distance = DistanceFrom(Target);
    if (distance <= GetWeapon(0)->WeaponType->Range) {
        vt_entry_48C(false, 0, false, nullptr);
        UpdateSight(false, 0, false, nullptr, GetWeapon(0)->WeaponType->Damage);
        auto at = Location;
        MapClass::Instance.RevealArea3(&at, 0, LastSightRange + 3, false);
    }
    if (!Destination) {
        const auto edge = Owner->Edge == Edge::North ? Edge::South : Owner->Edge == Edge::East ? Edge::West :
                          Owner->Edge == Edge::West ? Edge::East : Edge::North;
        CellStruct at;
        MapClass::Instance.PickCellOnEdge(at, edge, CellStruct::Empty, CellStruct::Empty, SpeedType::Winged,
                                        true, MovementZone::Normal);
        if (at != CellStruct::Empty) SetDestination(MapClass::Instance.GetCellAt(at), true);
    }
    return 3;
}
