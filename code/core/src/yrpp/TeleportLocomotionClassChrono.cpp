#include "yrpp/TeleportLocomotionClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SlaveManagerClass.h"

void YRPP_STDCALL TeleportLocomotionClass::ProcessChronoWait() {
    if (Timer.GetTimeLeft()) return;
    LinkedTo->WarpingOut = false;
    if (!LinkedTo->Target) {
        LinkedTo->ShortenTargetingDelay();
        bool acquired = false;
        if (LinkedTo->CanOpportunityFire()) {
            auto* old = LinkedTo->Target;
            LinkedTo->unknown_4FC = Unsorted::CurrentFrame;
            auto at = LinkedTo->GetCoords();
            acquired = LinkedTo->TargetAndEstimateDamage(at, static_cast<ThreatType>(1));
            if (acquired && old != LinkedTo->Target)
                LinkedTo->ShouldLoseTargetNow = (LinkedTo->ShouldLoseTargetNow & ~0xFFu) | 1u;
        }
        if (!acquired) LinkedTo->EnterIdleMode(false, true);
    }
    if (State > 0) ++State;
}

bool TeleportLocomotionClass::PrepareChronoDestination(CoordStruct destination, bool force) {
    auto& map = MapClass::Instance;
    if (force) {
        LinkedTo->UnmarkAllOccupationBits(LastCoords == CoordStruct::Empty ? LinkedTo->Location : LastCoords);
        LastCoords = LinkedTo->ChronoDestCoords;
        LastCoords.Z = map.GetCellFloorHeight(LastCoords);
        if ((unsigned(map.GetCellAt(LastCoords)->Flags) & 0x100u) && !LinkedTo->OnBridge) {
            LinkedTo->OnBridge = true;
            LastCoords.Z += CellClass::BridgeHeight;
        } else
            LinkedTo->OnBridge = false;
        LinkedTo->MarkAllOccupationBits(LastCoords);
    } else {
        auto* cell = map.GetCellAt(destination);
        bool blocked = false;
        auto* object = (unsigned(cell->Flags) & 0x100u) ? cell->AltObject : cell->FirstObject;
        for (; object; object = object->NextObject) {
            const auto kill = [&](ObjectClass* target) {
                int damage = target->GetType()->Strength;
                target->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, false, nullptr);
            };
            if (!object->IsIronCurtained() && object->WhatAmI() == AbstractType::Infantry &&
                LinkedTo->WhatAmI() == AbstractType::Infantry) {
                if (object->GetCoords() == destination) kill(object);
            } else if (object->IsIronCurtained())
                kill(LinkedTo);
            else if ((object->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None)
                blocked = true;
            else
                kill(object);
        }
        if ((unsigned(cell->Flags) & 0x100u) && !(unsigned(cell->Flags) & 0x200u)) blocked = true;
        // The target tests the non-null address of 0x483610 here, not its result.
        if (blocked) {
            auto zone = LinkedTo->GetTechnoType()->MovementZone;
            if (int(zone) == 9 || int(zone) == 2)
                zone = MovementZone::Normal;
            else if (int(zone) == 3)
                zone = static_cast<MovementZone>(5);
            const bool bridge = (unsigned(cell->Flags) & 0x100u) != 0;
            const int index = map.GetMovementZoneType(LinkedTo->GetCell()->MapCoords, zone, true);
            const auto nearby =
                map.NearByLocation(CellClass::Coord2Cell(destination), SpeedType::Track, index, zone, bridge, 1, 1,
                                   false, false, false, true, CellStruct::Empty, false, false);
            const auto old = cell->GetCoords(), next = map.GetCellAt(nearby)->GetCoords();
            LinkedTo->ChronoDestCoords = {next.X + destination.X - old.X, next.Y + destination.Y - old.Y,
                                          next.Z + destination.Z - old.Z};
            return false;
        }
    }
    LinkedTo->MarkAllOccupationBits(LastCoords == CoordStruct::Empty ? LinkedTo->Location : LastCoords);
    return true;
}

void TeleportLocomotionClass::CheckChronoLanding(CoordStruct destination) {
    auto& map = MapClass::Instance;
    auto* cell = map.GetCellAt(destination);
    const auto kill = [&] {
        int damage = LinkedTo->GetTechnoType()->Strength;
        LinkedTo->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, false, nullptr);
    };
    for (auto* object = cell->FirstObject; object; object = object->NextObject)
        if (object->IsIronCurtained()) kill();
    auto* type = LinkedTo->GetTechnoType();
    const bool naval = type->Naval;
    // 0x71885B compares the raw land value with 1 (Road).
    const bool groundedBoat = naval && ((unsigned(cell->Flags) & 0x100u) || cell->LandType == LandType::Road);
    const bool hover =
        type->SpeedType == SpeedType::Hover && (!type->PoweredUnit || LinkedTo->Owner->HasPoweredCenter(type));
    const auto sink = [&] {
        LinkedTo->IsSinking = true;
        LinkedTo->Stun();
        if (LinkedTo->SlaveManager) {
            LinkedTo->SlaveManager->Killed(LinkedTo->BeingManipulatedBy, LinkedTo->ChronoWarpedByHouse);
            if (LinkedTo->SlaveManager) GameDelete(LinkedTo->SlaveManager);
            LinkedTo->SlaveManager = nullptr;
        }
    };
    if (cell->LandType == LandType::Water && !hover && !naval && LinkedTo->WhatAmI() != AbstractType::Infantry &&
        !(unsigned(cell->Flags) & 0x100u)) {
        sink();
        if (LinkedTo->BeingManipulatedBy)
            LinkedTo->RegisterDestruction(LinkedTo->BeingManipulatedBy);
        else if (LinkedTo->ChronoWarpedByHouse)
            LinkedTo->RegisterKill(LinkedTo->ChronoWarpedByHouse);
    } else if (LinkedTo->IsCellOccupied(cell, FacingType(-1), -1, nullptr, true) == Move::No || groundedBoat) {
        if (cell->Tile_Is_Wet() && LinkedTo->WhatAmI() != AbstractType::Infantry)
            sink();
        else
            kill();
    }
}

// Select and reserve a destination before the ordinary teleport starts.
bool TeleportLocomotionClass::SelectChronoMoveDestination(const CoordStruct& destination) {
    auto& map = MapClass::Instance;
    LinkedTo->UnmarkAllOccupationBits(LastCoords == CoordStruct::Empty ? LinkedTo->Location : LastCoords);
    if (destination == CoordStruct::Empty) {
        LastCoords = destination;
    } else {
        const bool bridge = (unsigned(map.GetCellAt(destination)->Flags) & 0x100u) &&
                            LinkedTo->Location.Z > CellClass::BridgeHeight + map.GetCellFloorHeight(destination);
        if (LinkedTo->WhatAmI() == AbstractType::Infantry) {
            bool ignoreContents = false;
            if (LinkedTo->ReadyToNextMission()) LinkedTo->NextMission();
            if (LinkedTo->GetCurrentMission() == Mission::Capture ||
                LinkedTo->GetCurrentMission() == Mission::Eaten ||
                LinkedTo->GetCurrentMission() == Mission::Enter ||
                LinkedTo->GetCurrentMission() == Mission::Patrol) {
                auto* target = LinkedTo->Destination;
                if (target && target->WhatAmI() == AbstractType::Unit &&
                    static_cast<ObjectClass*>(target)->GetMapCoords() == CellClass::Coord2Cell(destination))
                    ignoreContents = true;
                target = LinkedTo->Destination;
                if (target && target->WhatAmI() == AbstractType::Aircraft &&
                    static_cast<ObjectClass*>(target)->GetMapCoords() == CellClass::Coord2Cell(destination))
                    ignoreContents = true;
                target = LinkedTo->Destination;
                if (target && target->WhatAmI() == AbstractType::Building &&
                    static_cast<ObjectClass*>(map.GetCellAt(destination)->GetBuilding()) == target)
                    ignoreContents = true;
            }
            const auto previous = LastCoords;
            LastCoords = map.GetCellAt(destination)->FindInfantrySubposition(destination, ignoreContents, bridge, false);
            if (LinkedTo->IsCellOccupied(map.GetCellAt(CellClass::Coord2Cell(LastCoords)), FacingType(-1), -1, nullptr, true) != Move::OK)
                LastCoords = CoordStruct::Empty;
            // The original virtual query is observable even when its result is unused.
            CoordStruct buffer;
            map.GetCellAt(previous)->GetCoords(&buffer);
            if (LastCoords == CoordStruct::Empty) {
                auto* target = LinkedTo->Destination;
                if (target && (target->AbstractFlags & AbstractFlags::Object) != AbstractFlags::None) {
                    const auto movement = LinkedTo->GetTechnoType()->MovementZone;
                    const auto targetCell = CellClass::Coord2Cell(target->GetDestination());
                    const auto* originCell = map.GetCellAt(LinkedTo->GetDestination());
                    const bool onBridge = LinkedTo->IsOnBridge(nullptr);
                    const bool targetBridge = (unsigned(map.GetCellAt(targetCell)->Flags) & 0x100u) != 0;
                    const auto zone = map.GetMovementZoneType(originCell->MapCoords, movement, onBridge);
                    const auto nearby = map.NearByLocation(targetCell, LinkedTo->GetTechnoType()->SpeedType, zone,
                        movement, targetBridge, 1, 1, false, true, false, true, CellStruct::Empty, false, false);
                    if (nearby != CellStruct::Empty) {
                        CoordStruct at;
                        map.GetCellAt(nearby)->GetCoords(&at);
                        LastCoords = map.GetCellAt(nearby)->FindInfantrySubposition(at, ignoreContents, bridge, false);
                    }
                }
            }
        } else if (LinkedTo->IsImmobilized) {
            LinkedTo->MarkAllOccupationBits(LastCoords);
        } else {
            const auto movement = LinkedTo->GetTechnoType()->MovementZone;
            auto cell = CellClass::Coord2Cell(destination);
            if (cell != CellStruct::Empty &&
                LinkedTo->IsCellOccupied(map.GetCellAt(cell), FacingType(-1), -1, nullptr, true) != Move::OK) {
                const auto zone = map.GetMovementZoneType(cell, movement, bridge);
                cell = map.NearByLocation(cell, LinkedTo->GetTechnoType()->SpeedType, zone, movement,
                                         bridge, 1, 1, false, false, false, true, CellStruct::Empty, false, false);
            }
            if (cell != CellStruct::Empty) {
                LinkedTo->UnmarkAllOccupationBits(LastCoords);
                CoordStruct at;
                map.GetCellAt(cell)->GetCoords(&at);
                LastCoords = {int(short(at.X / 256)) * 256 + 128, int(short(at.Y / 256)) * 256 + 128, 0};
                LastCoords.Z = map.GetCellFloorHeight(LastCoords);
                LinkedTo->MarkAllOccupationBits(LastCoords);
            }
        }
    }
    LinkedTo->MarkAllOccupationBits(LastCoords == CoordStruct::Empty ? LinkedTo->Location : LastCoords);
    return LastCoords != CoordStruct::Empty;
}
