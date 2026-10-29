// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Exit_Object/Find_Exit_Cell/Do_MISSION_UNLOAD.
// YR 0x443C60/0x44EFB0/0x44D880/0x449540. YR uses exit-list index 10,
// an X - 1 adjustment, and the rules ExitCoord (0x44F640), unlike TS.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include "RulesClassReaders.hpp"
#include "map_world.hpp"
#include <cstring>
#include <cstdlib>
namespace {
int radio_distance(const BuildingClass& building, const TechnoClass& sender) {
    const auto from = sender.GetCoords();
    const auto to = building.GetCoords();
    const auto difference = [](int a, int b) { return double(std::bit_cast<int>(unsigned(a) - unsigned(b))); };
    const double x = difference(to.X, from.X), y = difference(to.Y, from.Y), z = difference(to.Z, from.Z);
    return game::x87_integer(Math::sqrt(x * x + z * z + y * y));
}
CellStruct add(CellStruct a, CellStruct b) { return {short(a.X + b.X), short(a.Y + b.Y)}; }
CellStruct vehicle_exit(const BuildingClass& building) {
    const auto base = building.GetMapCoords();
    const auto delta = building.Type->FoundationOutside ? building.Type->FoundationOutside[10] : CellStruct{3, 0};
    return {short(base.X + delta.X - 1), short(base.Y + delta.Y)};
}
void start_factory_track(FootClass& unit, const BuildingClass& building, const CoordStruct& exit) {
    // YR 0x0044DE84..0x0044E163 queries the actual locomotor, not the type's
    // default CLSID: idle chrono miners may already have restored Teleport.
    constexpr GUID persistIID{0x109, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    constexpr GUID piggyIID{0x92FEA800, 0xA184, 0x11D1, {0xB7, 0x0A, 0, 0xA0, 0x24, 0xDD, 0xAF, 0xD1}};
    IPersist* persist = nullptr;
    GUID clsid{};
    if (!unit.Locomotor || unit.Locomotor->QueryInterface(persistIID, reinterpret_cast<void**>(&persist)) < 0 ||
        !persist)
        std::abort();
    const auto identified = persist->GetClassID(&clsid);
    persist->Release();
    if (identified < 0) std::abort();
    const auto is = [&](const GUID& id) { return !std::memcmp(&clsid, &id, sizeof(id)); };
    if (is(LocomotionClass::CLSIDs::Teleport) || is(LocomotionClass::CLSIDs::Tunnel)) {
        // The original tests Is_Piggybacking here, not Is_Ok_To_End. Unwrap any
        // existing wrapper before placing Drive over the underlying locomotor.
        IPiggyback* old = nullptr;
        const auto queried = unit.Locomotor->QueryInterface(piggyIID, reinterpret_cast<void**>(&old));
        if (queried < 0 && queried != static_cast<HRESULT>(0x80004002u)) std::abort();
        if (old && old->Is_Piggybacking()) {
#if defined(_MSC_VER)
            unit.Locomotor = nullptr;
            ILocomotion* restored = nullptr;
            if (old->End_Piggyback(&restored) < 0) std::abort();
            unit.Locomotor.Attach(restored);
#else
            unit.Locomotor->Release();
            unit.Locomotor = nullptr;
            if (old->End_Piggyback(&unit.Locomotor) < 0) std::abort();
#endif
        }
        if (old) old->Release();
        auto* drive = GameCreate<DriveLocomotionClass>();
        if (!drive) std::abort();
        drive->AddRef();
        if (drive->Link_To_Object(&unit) < 0 || drive->Begin_Piggyback(unit.Locomotor) < 0) std::abort();
#if defined(_MSC_VER)
        unit.Locomotor = static_cast<ILocomotion*>(drive);
#else
        drive->AddRef();
        unit.Locomotor->Release();
        unit.Locomotor = drive;
#endif
        drive->Release();
    } else if (!is(LocomotionClass::CLSIDs::Drive)) {
        // 0x0044DF1C: locomotors without a drive track receive a normal exit order.
        const auto cell = building.GetMapCoords();
        unit.SetDestination(MapClass::Instance.GetCellAt(CellStruct{short(cell.X + 4), short(cell.Y + 1)}), true);
        return;
    }
    unit.Locomotor->Force_Track(66, exit);
}
}
CellStruct BuildingClass::FindExitCell(FootClass* product, CellStruct preferred) const {
    const auto origin = GetMapCoords();
    auto& map = MapClass::Instance;
    const auto free = [&](CellStruct at, bool loco) {
        return map.CoordinatesLegal(at) &&
               product->IsCellOccupied(map.GetCellAt(at), FacingType(-1), -1, nullptr, loco) == Move::OK;
    };
    if (Type->GDIBarracks && free(add(origin, {1, 2}), true)) return add(origin, {1, 2});
    if (Type->NODBarracks && free(add(origin, {2, 2}), true)) return add(origin, {2, 2});
    if (Type->YuriBarracks && free(add(origin, {2, 1}), true)) return add(origin, {2, 1});
    if (Type->WeaponsFactory && Type->Naval) {
        const auto dock = CellClass::Coord2Cell(GetDockCoords(product));
        for (const auto offset : {CellStruct{1, 1}, CellStruct{1, 0}, CellStruct{0, 1}})
            if (free(add(dock, offset), false)) return add(dock, offset);
    }
    if (preferred != CellStruct::Empty && free(preferred, false)) return preferred;
    if (Type->FoundationOutside && !Type->Hospital) {
        for (auto* p = Type->FoundationOutside; *p != CellStruct{0x7FFF, 0x7FFF}; ++p)
            if (free(add(origin, *p), false)) return add(origin, *p);
    } else {
        const int w = Type->GetFoundationWidth(), h = Type->GetFoundationHeight(false);
        for (int x = -1; x <= w; ++x) {
            auto at = add(origin, {short(x), short(h)});
            if (free(at, true)) return at;
            at = add(origin, {short(x), -1});
            if (free(at, true)) return at;
        }
        for (int y = -1; y <= h; ++y) {
            auto at = add(origin, {short(w), short(y)});
            if (free(at, true)) return at;
            at = add(origin, {-1, short(y)});
            if (free(at, true)) return at;
        }
    }
    return CellStruct::Empty;
}
KickOutResult BuildingClass::KickOutUnit(TechnoClass* product, CellStruct preferred) {
    if (!product) return KickOutResult::Failed;
    product->IsInPlayfield = true;
    if (product->WhatAmI() == AbstractType::Building) {
        if (Owner->IsControlledByHuman()) return KickOutResult::Failed;
        Owner->AI_ProductionCompleted(AbstractType::Building);
        Owner->ProducingBuildingTypeIndex = -1;
        auto* building = static_cast<BuildingClass*>(product);
        auto* node = Owner->Base.FindNextNode(building->Type->ArrayIndex);
        CellStruct at{};
        CoordStruct where{};
        if (node && node->MapCoords != CellStruct::Empty) {
            at = node->MapCoords;
            if (!*building->Type->PowersUpBuilding && !Owner->IsNearBase(building->Type, at)) {
                Owner->FindBaseBuildingSite(&at, building->Type, HouseClass::ScoreBaseCell, -1);
                node->MapCoords = at;
            }
            where = {at.X * 256 + 128, at.Y * 256 + 128, 0};
        } else {
            if (*building->Type->PowersUpBuilding) {
                Owner->FindBaseUpgradeSite(&at, building->Type);
                if (at != CellStruct::Empty) where = {at.X * 256 + 128, at.Y * 256 + 128, 0};
            } else {
                Owner->FindBaseBuildingSite(&at, building->Type, HouseClass::ScoreBaseCell, -1);
                // The original converts an empty ordinary site to (128,128,0)
                // and lets CheckFoundation decide; upgrades keep an empty coord.
                where = {at.X * 256 + 128, at.Y * 256 + 128, 0};
            }
            if (node && where != CoordStruct::Empty) node->MapCoords = at;
        }
        if (where == CoordStruct::Empty) {
            if (*building->Type->PowersUpBuilding && Owner->Base.FindNextNode(-1) == node && node)
                Owner->Base.BaseNodes.RemoveItem(static_cast<int>(node - Owner->Base.BaseNodes.Items));
            return KickOutResult::Failed;
        }
        const int foundation = building->Type->CheckFoundation(at, Owner);
        if (foundation == 1) {
            const int attempts = Owner->Base.FailedToPlaceNode(node);
            if (SessionClass::Instance.GameMode != GameMode::Campaign &&
                attempts > RulesClass::Instance->MaximumBuildingPlacementFailures && node)
                Owner->Base.BaseNodes.RemoveItem(static_cast<int>(node - Owner->Base.BaseNodes.Items));
            return KickOutResult::Busy;
        }
        if (foundation != 0 && foundation != 2) return KickOutResult::Failed;
        if (foundation == 0 && building->Unlimbo(where, DirType::North)) {
            // 0x445329 checks the stored mission, not GetCurrentMission's queued fallback.
            if (building->CurrentMission == Mission::None && building->QueuedMission == Mission::Construction)
                building->NextMission();
            // 0x4453B0..0x445408: fill ordinary wall gaps after the first segment
            // lands, while its deferred product still exists, before factory completion.
            if (!building->Type->FirestormWall && building->Type->ToOverlay && building->Type->ToOverlay->Wall)
                MapClass::Instance.BuildingToWall(at, Owner, building->Type);
            game::map_object_changed();
            return KickOutResult::Succeeded;
        }
        if (!node) return KickOutResult::Failed;
        if (building->Type->Wall || building->Type->Gate)
            Owner->Base.BaseNodes.RemoveItem(static_cast<int>(node - Owner->Base.BaseNodes.Items));
        else
            for (auto& other : Owner->Base.BaseNodes)
                if (other.MapCoords == at) other.MapCoords = CellStruct::Empty;
        return KickOutResult::Failed;
    }
    if ((product->AbstractFlags & ::AbstractFlags::Foot) == ::AbstractFlags::None) return KickOutResult::Failed;
    auto* foot = static_cast<FootClass*>(product);
    // 0x443C81 precedes the mobile-product branches, including busy exits.
    foot->IsInPlayfield = true;
    const auto kind = product->WhatAmI();
    if (kind == AbstractType::Aircraft) {
        Owner->AI_ProductionCompleted(AbstractType::Aircraft);
        Owner->ProducingAircraftTypeIndex = -1;
        auto* aircraft = static_cast<AircraftClass*>(product);
        // 0x53A130 always returns false in YR. A free docking link admits
        // either aircraft kind; only a non-airport aircraft can spawn off-map.
        if (HasFreeLink(aircraft)) {
            aircraft->SetZ(0);
            ++Unsorted::ScenarioInit;
            const auto at = GetDockCoords(aircraft);
            if (!aircraft->Unlimbo(at, static_cast<DirType>(RulesClass::Instance->PoseDir))) {
                --Unsorted::ScenarioInit;
                return KickOutResult::Failed;
            }
            SendCommand(RadioCommand::RequestLink, aircraft);
            SendCommand(RadioCommand::RequestTether, aircraft);
            aircraft->SetLocation(GetDockCoords(aircraft));
            aircraft->DockNowHeadingTo = this;
            if (ArchiveTarget && !aircraft->Type->AirportBound) {
                aircraft->SetDestination(ArchiveTarget, true);
                aircraft->QueueMission(Mission::Move, false);
            }
            --Unsorted::ScenarioInit;
            return KickOutResult::Succeeded;
        }
        if (aircraft->Type->AirportBound) return KickOutResult::Failed;
        const auto& map = MapClass::Instance;
        const auto& visible = map.VisibleRect;
        CellStruct edge{short(visible.X + visible.Y + 1), short(map.MapRect.Width - visible.X + visible.Y)};
        const auto center = GetCoords();
        const int side = int(short(short(center.X / 256) - edge.X)) -
                         int(short(short(center.Y / 256) - edge.Y));
        edge.X = short(edge.X - 1);
        if (side > visible.Width) {
            edge.X = short(edge.X + visible.Width);
            edge.Y = short(edge.Y - visible.Width);
        }
        const int offset = ScenarioClass::Instance->Random.RandomRanged(0, visible.Height);
        edge.X = short(edge.X + offset);
        edge.Y = short(edge.Y + offset);
        const CoordStruct at{int(edge.X) * 256 + 128, int(edge.Y) * 256 + 128, 0};
        ++Unsorted::ScenarioInit;
        if (!aircraft->Unlimbo(at, DirType::North)) {
            --Unsorted::ScenarioInit;
            return KickOutResult::Failed;
        }
        AbstractClass* target = ArchiveTarget;
        if (!target) {
            CellStruct nearby;
            aircraft->NearbyLocation(&nearby, this);
            if (nearby != CellStruct::Empty) target = MapClass::Instance.GetCellAt(nearby);
        }
        aircraft->SetDestination(target, true);
        aircraft->ForceMission(Mission::Move);
        --Unsorted::ScenarioInit;
        return KickOutResult::Succeeded;
    } else if (kind == AbstractType::Unit || kind == AbstractType::Infantry) {
        if (!Type->Hospital && !Type->Armory && !Type->WeaponsFactory && !HasFreeLink())
            return KickOutResult::Busy;
        // 0x444096..0x444131 exempts hospital/armory clients, not barracks.
        // Complete an ordinary production request before selecting its exit.
        if (!Type->Hospital && !Type->Armory) {
            Owner->AI_ProductionCompleted(kind);
            if (kind == AbstractType::Unit) Owner->ProducingUnitTypeIndex = -1;
            if (kind == AbstractType::Infantry) Owner->ProducingInfantryTypeIndex = -1;
        }
    }
    if (Type->WeaponsFactory && Type->Naval) {
        Owner->ProducingUnitTypeIndex = -1;
        if (!HasAnyLink()) QueueMission(Mission::Unload, false);
        auto& map = MapClass::Instance;
        auto at = CellClass::Coord2Cell(GetCoords());
        if (ArchiveTarget) {
            const auto rally = CellClass::Coord2Cell(ArchiveTarget->GetCoords());
            const auto origin = GetMapCoords();
            const DirStruct direction(rule_integer(
                (Math::atan2(double(origin.Y) - rally.Y, double(rally.X) - origin.X) - 1.5707963267948966) *
                -10430.060040584269));
            const auto facing = static_cast<FacingType>(direction.GetValue<3>());
            while (map.GetCellAt(at)->GetBuilding() == this)
                at = map.GetCellAt(at)->GetNeighbourCell(facing)->MapCoords;
        }
        auto* cell = ArchiveTarget ? map.GetCellAt(at) : nullptr;
        if (!ArchiveTarget || cell->LandType != LandType::Water || cell->FindTechnoNearestTo({0, 0}, false, nullptr) ||
            !map.IsWithinUsableArea(at, true)) {
            at = map.NearByLocation(CellClass::Coord2Cell(GetCoords()), foot->GetTechnoType()->SpeedType, -1,
                                    MovementZone::Normal, false, 1, 1, false, false, false, true, CellStruct{0, 0},
                                    false, false);
            cell = map.GetCellAt(at);
        }
        if (!foot->Unlimbo(cell->GetCoords(), DirType::East)) return KickOutResult::Failed;
        if (ArchiveTarget) {
            foot->SetDestination(ArchiveTarget, true);
            foot->QueueMission(Mission::Move, false);
        }
        foot->Mark(MarkType::Up);
        foot->SetLocation(cell->GetCellCoords());
        foot->Mark(MarkType::Down);
        game::map_object_changed();
        return KickOutResult::Succeeded;
    }
    foot->SetArchiveTarget(ArchiveTarget);
    if (Type->WeaponsFactory && !Type->Naval) {
        if (GetCurrentMission() == Mission::Unload) {
            for (auto* other : Owner->Buildings)
                if (other != this && other->Type == Type && other->GetCurrentMission() == Mission::Guard &&
                    !other->Factory) {
                    // 0x44451F..0x444552 lends the controller only for the recursive exit.
                    auto* factory = Factory;
                    other->Factory = factory;
                    Factory = nullptr;
                    const auto result = other->KickOutUnit(product, CellStruct::Empty);
                    other->Factory = nullptr;
                    Factory = factory;
                    return result;
                }
            return KickOutResult::Busy;
        }
        // 0x44F640: nonzero ExitCoord is relative to Location, not GetCoords' center.
        const auto at = Type->ExitCoord != CoordStruct::Empty ? Location + Type->ExitCoord : GetCoords();
        ++Unsorted::ScenarioInit;
        const bool placed = foot->Unlimbo(at, DirType::East);
        if (!placed) {
            --Unsorted::ScenarioInit;
            return KickOutResult::Failed;
        }
        foot->Mark(MarkType::Up);
        foot->SetLocation(at);
        foot->Mark(MarkType::Down);
        SendCommand(RadioCommand::RequestLink, foot);
        SendCommand(RadioCommand::RequestTether, foot);
        // 0x444575..0x444979 retains ScenarioInit through radio and mission setup.
        QueueMission(Mission::Unload, false);
        --Unsorted::ScenarioInit;
    } else {
        if (HasAnyLink()) return KickOutResult::Busy;
        const auto exit = FindExitCell(foot, preferred);
        if (exit == CellStruct::Empty) return KickOutResult::Failed;
        const auto origin = GetMapCoords();
        auto inside = exit;
        if (inside.X >= origin.X + Type->GetFoundationWidth())
            --inside.X;
        else if (inside.X < origin.X)
            ++inside.X;
        if (inside.Y >= origin.Y + Type->GetFoundationHeight(false))
            --inside.Y;
        else if (inside.Y < origin.Y)
            ++inside.Y;
        CoordStruct at{inside.X * 256 + 128, inside.Y * 256 + 128, 0};
        if ((Type->GDIBarracks && exit == add(origin, {1, 2})) || (Type->NODBarracks && exit == add(origin, {2, 2})) ||
            (Type->YuriBarracks && exit == add(origin, {2, 1})))
            at += Type->ExitCoord;
        auto* cell = MapClass::Instance.GetCellAt(exit);
        DirStruct facing;
        GetDirectionTo(&facing, cell);
        ++Unsorted::ScenarioInit;
        const bool placed = foot->Unlimbo(at, static_cast<DirType>(facing.GetValue<8>()));
        --Unsorted::ScenarioInit;
        if (!placed) return KickOutResult::Failed;
        foot->QueueMission(Mission::Move, false);
        foot->SetDestination(cell, true);
        if (SendCommand(RadioCommand::RequestLink, foot) == RadioCommand::AnswerPositive)
            SendCommand(RadioCommand::RequestTether, foot);
    }
    if (LogicClass::Instance.FindItemIndex(foot) < 0) LogicClass::Instance.AddObject(foot, false);
    game::map_object_changed();
    return KickOutResult::Succeeded;
}
bool BuildingClass::ClearFactoryBib() {
    if (!Type->WeaponsFactory) return false;
    auto* cell = MapClass::Instance.GetCellAt(vehicle_exit(*this));
    bool blocked = false;
    for (auto* object = cell->FirstObject; object; object = object->NextObject)
        if (object != this && object != GetNthLink()) {
            blocked = true;
            break;
        }
    if (!blocked) return false;
    cell->ScatterContent(CoordStruct::Empty, true, true, false);
    const auto from = cell->GetCoords();
    for (int i = 0; i < 8; ++i)
        cell->GetNeighbourCell(static_cast<FacingType>(i))->ScatterContent(from, true, true, false);
    return true;
}
int BuildingClass::MissionFactoryUnload() {
    enum FactoryUnloadState : int { Initial = 0, ClearBib = 1, Open = 2, Leave = 3, Close = 4 };
    auto* unit = HasAnyLink() ? static_cast<FootClass*>(GetNthLink()) : nullptr;
    switch (MissionStatus) {
        case FactoryUnloadState::Initial:
            if (unit) {
                unit->QueueMission(Mission::Guard, false);
                unit->NextMission();
            }
            UnloadTimer.StartTimer11(Type->DeployTime);
            MissionStatus = Type->Naval ? FactoryUnloadState::Close : FactoryUnloadState::ClearBib;
            NeedsRedraw = true;
            DestroyNthAnim(BuildingAnimSlot::Idle);
            PlayNthAnim(BuildingAnimSlot::Production, !IsGreenHP(), false);
            break;
        case FactoryUnloadState::ClearBib:
            if (!ClearFactoryBib()) MissionStatus = FactoryUnloadState::Open;
            break;
        case FactoryUnloadState::Open:
            if (UnloadTimer.AreStates01()) {
                if (unit) {
                    // 0x44DE73 queues the exit order; the unit update admits it normally.
                    unit->QueueMission(Mission::Move, false);
                    const auto at = vehicle_exit(*this);
                    CoordStruct coord{at.X * 256 + 128, at.Y * 256 + 128, 0};
                    start_factory_track(*unit, *this, coord);
                    unit->SetSpeedPercentage(0.5);
                    MissionStatus = FactoryUnloadState::Leave;
                } else {
                    UnloadTimer.StartTimer10(Type->DeployTime);
                    MissionStatus = FactoryUnloadState::Close;
                }
            }
            break;
        case FactoryUnloadState::Leave:
            if (!IsTether) {
                UnloadTimer.StartTimer10(Type->DeployTime);
                MissionStatus = FactoryUnloadState::Close;
            }
            break;
        case FactoryUnloadState::Close:
            if (UnloadTimer.AreStates00() || Type->Naval) {
                EnterIdleMode(false, true);
                NeedsRedraw = true;
            }
            break;
        default:
            break;
    }
    game::map_object_changed();
    return rule_integer(CurrentMissionControl()->Rate * 900.0) + ScenarioClass::Instance->Random.RandomRanged(0, 2);
}

// Ground factory arms of YR 0x43C2D0 / OpenTS Receive_Message.
RadioCommand BuildingClass::ReceiveCommand(TechnoClass* sender, RadioCommand command, AbstractClass*& data) {
    // 0x43C620/0x43CA2F: ordinary aircraft pads use the common admission
    // checks, then the Helipad handshake rather than refinery unloading.
    if (sender && Type->UnitReload && !Type->UnitRepair && !Type->Bunker &&
        !Type->UnitAbsorb && !Type->InfantryAbsorb && !Type->Hospital && !Type->Armory) {
        switch (command) {
            case RadioCommand::QueryCanEnter:
                TechnoClass::ReceiveCommand(sender, command, data);
                if (!Owner->IsAlliedWith(sender)) return RadioCommand::AnswerInvalid;
                if (GetCurrentMission() == Mission::Construction || GetCurrentMission() == Mission::Selling ||
                    !BState || (!Unsorted::ScenarioInit && !HasFreeLink(sender)))
                    return RadioCommand::AnswerNegative;
                if (sender->GetTechnoType()->MovementZone != MovementZone::Amphibious &&
                    GetTechnoType()->Naval != sender->GetTechnoType()->Naval)
                    return RadioCommand::AnswerNegative;
                if (sender->GetTechnoType()->BalloonHover || !HasPower) return RadioCommand::AnswerNegative;
                if (Type->Helipad)
                    return sender->WhatAmI() != AbstractType::Aircraft ? RadioCommand::AnswerNegative
                                                                  : RadioCommand::AnswerPositive;
                return RadioCommand::AnswerInvalid;
            case RadioCommand::RequestLoading:
                TechnoClass::ReceiveCommand(sender, command, data);
                if (!HasPower) return RadioCommand::AnswerNegative;
                if (!ContainsLink(sender) && HasFreeLink(sender)) {
                    SendCommand(RadioCommand::RequestLink, sender);
                    ContainsLink(sender);
                }
                if (SendCommand(RadioCommand::QueryMoving, sender) == RadioCommand::AnswerPositive) {
                    data = this;
                    if (Type->Helipad && SendCommandWithData(RadioCommand::RequestMoveTo, data, sender) ==
                                             RadioCommand::AnswerAwaiting)
                        SendToFirstLink(RadioCommand::RequestTether);
                }
                return RadioCommand::AnswerPositive;
            case RadioCommand::RequestCompleteEnter:
                if (GetCurrentMission() == Mission::Selling) return RadioCommand::AnswerNegative;
                IsReadyToCommence = true;
                QueueMission(Mission::Repair, false);
                sender->QueueMission(Mission::Sleep, false);
                return RadioCommand::AnswerPositive;
            default:
                break;
        }
    }
    if (sender && (Type->UnitAbsorb || Type->InfantryAbsorb || Type->Bunker || Type->Grinding)) {
        switch (command) {
            case RadioCommand::QueryCanEnter: {
                TechnoClass::ReceiveCommand(sender, command, data);
                if (!Owner->IsAlliedWith(sender)) return RadioCommand::AnswerInvalid;
                if (GetCurrentMission() == Mission::Construction || GetCurrentMission() == Mission::Selling || !BState)
                    return RadioCommand::AnswerNegative;
                if (!Unsorted::ScenarioInit && !HasFreeLink(sender) && !Type->UnitAbsorb && !Type->InfantryAbsorb)
                    return RadioCommand::AnswerNegative;
                if (sender->GetTechnoType()->MovementZone != MovementZone::Amphibious &&
                    ((GetTechnoType()->Naval && !sender->GetTechnoType()->Naval) ||
                     (!GetTechnoType()->Naval && sender->GetTechnoType()->Naval)))
                    return RadioCommand::AnswerNegative;
                if (sender->GetTechnoType()->BalloonHover || !HasPower) return RadioCommand::AnswerNegative;
                if (Type->UnitAbsorb || Type->InfantryAbsorb) {
                    if ((sender->WhatAmI() == AbstractType::Unit && !Type->UnitAbsorb) ||
                        (sender->WhatAmI() == AbstractType::Infantry && !Type->InfantryAbsorb))
                        return RadioCommand::AnswerNegative;
                    if (sender->CaptureManager && sender->CaptureManager->IsControllingSomething())
                        return RadioCommand::AnswerNegative;
                    if (Passengers.NumPassengers + 1 <= Type->Passengers &&
                        sender->GetTechnoType()->Size <= Type->SizeLimit)
                        return RadioCommand::AnswerPositive;
                }
                if (Type->Grinding) return RadioCommand::AnswerPositive;
                if (Type->Bunker)
                    return sender->CanEnterTankBunker() &&
                                   SendCommand(RadioCommand::QueryOnBuilding, sender) != RadioCommand::AnswerPositive
                               ? RadioCommand::AnswerPositive
                               : RadioCommand::AnswerNegative;
                if (Type->UnitRepair)
                    return (sender->WhatAmI() == AbstractType::Unit || sender->WhatAmI() == AbstractType::Aircraft) &&
                                   SendCommand(RadioCommand::QueryOnBuilding, sender) != RadioCommand::AnswerPositive
                               ? RadioCommand::AnswerPositive
                               : RadioCommand::AnswerNegative;
                // Combined types retain the original fallthrough to other admission modes.
                if (!Type->UnitRepair && !Type->UnitReload && !Type->Hospital && !Type->Armory && !Type->DockUnload)
                    return RadioCommand::AnswerInvalid;
                break;
            }
            case RadioCommand::RequestLoading: {
                TechnoClass::ReceiveCommand(sender, command, data);
                if (!HasPower) return RadioCommand::AnswerNegative;
                if (Type->UnitRepair && ContainsLink(sender) &&
                    SendCommand(RadioCommand::QueryNeedRepair, sender) == RadioCommand::AnswerNegative)
                    return RadioCommand::AnswerNegative;
                if (Type->Bunker && ContainsLink(sender) && !sender->CanEnterTankBunker())
                    return RadioCommand::AnswerNegative;
                if (!Type->Hospital && !Type->Armory) {
                    if (!ContainsLink(sender) && HasFreeLink(sender)) {
                        SendCommand(RadioCommand::RequestLink, sender);
                        ContainsLink(sender);
                    }
                    const bool distant = GetNthLink() && (Type->UnitRepair || Type->Bunker) &&
                                         radio_distance(*this, *GetNthLink()) > 128;
                    if (SendCommand(RadioCommand::QueryMoving, sender) == RadioCommand::AnswerPositive || distant)
                        data = this;
                    return RadioCommand::AnswerPositive;
                }
                break;
            }
            case RadioCommand::QueryCanUnload:
                return !Passengers.FirstPassenger && HasFreeLink(sender) && !InLimbo &&
                               Owner == sender->GetOwningHouse() &&
                               (Type->Hospital || Type->UnitRepair || Type->Refinery)
                           ? RadioCommand::AnswerPositive
                           : RadioCommand::AnswerNegative;
            case RadioCommand::RequestCompleteEnter:
                if (GetCurrentMission() == Mission::Selling) return RadioCommand::AnswerNegative;
                if (Type->UnitAbsorb || Type->InfantryAbsorb) return RadioCommand::AnswerPositive;
                if (Type->UnitRepair || Type->UnitReload || Type->Hospital || Type->Armory) {
                    IsReadyToCommence = true;
                    QueueMission(Mission::Repair, false);
                    sender->QueueMission(Mission::Sleep, false);
                    return RadioCommand::AnswerPositive;
                }
                if (Type->Bunker) {
                    IsReadyToCommence = true;
                    QueueMission(Mission::Repair, false);
                    return RadioCommand::AnswerPositive;
                }
                break;
            default:
                break;
        }
    }
    if (command == RadioCommand::NotifyUnlink) {
        // 0x43C2D0 case 3: finish the body-mode handoff before clearing the link.
        // Placement sends this to the new building while Construction is queued.
        BeginMode(BStateType::Idle);
        TechnoClass::ReceiveCommand(sender, command, data);
        return RadioCommand::AnswerPositive;
    }
    if (command == RadioCommand::RequestBeginProduction) {
        QueueMission(Mission::Repair, false);
        TechnoClass::ReceiveCommand(sender, command, data);
        return RadioCommand::AnswerPositive;
    }
    if (command == RadioCommand::RequestEndProduction) {
        if (GetCurrentMission() != Mission::Selling) {
            QueueMission(Mission::Guard, false);
            if (Type->ConstructionYard) {
                DestroyNthAnim(BuildingAnimSlot::PreProduction);
                DestroyNthAnim(static_cast<BuildingAnimSlot>(18));
                PlayNthAnim(BuildingAnimSlot::Production, !IsGreenHP(), false);
            }
        }
        TechnoClass::ReceiveCommand(sender, command, data);
        return RadioCommand::AnswerPositive;
    }

    if (Type->DockUnload && sender && sender->WhatAmI() == AbstractType::Unit &&
        static_cast<UnitClass*>(sender)->Type->Harvester) {
        switch (command) {
            case RadioCommand::QueryCanEnter:
                if (!Owner->IsAlliedWith(sender)) return RadioCommand::AnswerInvalid;
                if (InLimbo || !IsAlive || !HasPower || GetCurrentMission() == Mission::Construction ||
                    GetCurrentMission() == Mission::Selling || (!Unsorted::ScenarioInit && !HasFreeLink(sender)))
                    return RadioCommand::AnswerNegative;
                return RadioCommand::AnswerPositive;
            case RadioCommand::RequestLoading: {
                if (!HasPower || !HasFreeLink(sender)) return RadioCommand::AnswerNegative;
                if (!ContainsLink(sender) &&
                    SendCommand(RadioCommand::RequestLink, sender) != RadioCommand::AnswerPositive)
                    return RadioCommand::AnswerNegative;
                const auto at = GetMapCoords();
                data = MapClass::Instance.GetCellAt(CellStruct{short(at.X + 3), short(at.Y + 1)});
                if (SendCommandWithData(RadioCommand::RequestMoveTo, data, sender) == RadioCommand::AnswerAwaiting) {
                    SendCommand(RadioCommand::RequestTether, sender);
                    SendCommand(RadioCommand::RequestDockRefinery, sender);
                }
                return RadioCommand::AnswerPositive;
            }
            case RadioCommand::RequestCompleteEnter:
                sender->QueueMission(Mission::Unload, false);
                return RadioCommand::AnswerPositive;
            default:
                break;
        }
    }
    if (Type->UnitRepair && sender) {
        switch (command) {
            case RadioCommand::QueryCanEnter:
                TechnoClass::ReceiveCommand(sender, command, data);
                if (!Owner->IsAlliedWith(sender)) return RadioCommand::AnswerInvalid;
                if (GetCurrentMission() == Mission::Construction || GetCurrentMission() == Mission::Selling ||
                    !BState || (!Unsorted::ScenarioInit && !HasFreeLink(sender)))
                    return RadioCommand::AnswerNegative;
                if (sender->GetTechnoType()->MovementZone != MovementZone::Amphibious &&
                    Type->Naval != sender->GetTechnoType()->Naval)
                    return RadioCommand::AnswerNegative;
                if (sender->GetTechnoType()->BalloonHover || !HasPower) return RadioCommand::AnswerNegative;
                return (sender->WhatAmI() == AbstractType::Unit || sender->WhatAmI() == AbstractType::Aircraft) &&
                               SendCommand(RadioCommand::QueryOnBuilding, sender) != RadioCommand::AnswerPositive
                           ? RadioCommand::AnswerPositive
                           : RadioCommand::AnswerNegative;
            case RadioCommand::RequestLoading:
                TechnoClass::ReceiveCommand(sender, command, data);
                if (!HasPower) return RadioCommand::AnswerNegative;
                if (ContainsLink(sender) &&
                    SendCommand(RadioCommand::QueryNeedRepair, sender) == RadioCommand::AnswerNegative)
                    return RadioCommand::AnswerNegative;
                if (!ContainsLink(sender) && HasFreeLink(sender)) SendCommand(RadioCommand::RequestLink, sender);
                {
                    const bool distant = GetNthLink() && DistanceFrom3D(GetNthLink()) > 128;
                    if (SendCommand(RadioCommand::QueryMoving, sender) == RadioCommand::AnswerPositive || distant)
                        data = this;
                }
                return RadioCommand::AnswerPositive;
            case RadioCommand::QueryCanUnload:
                return !Passengers.FirstPassenger && HasFreeLink(sender) && !InLimbo &&
                               Owner == sender->GetOwningHouse()
                           ? RadioCommand::AnswerPositive
                           : RadioCommand::AnswerNegative;
            case RadioCommand::RequestCompleteEnter:
                if (GetCurrentMission() == Mission::Selling) return RadioCommand::AnswerNegative;
                IsReadyToCommence = true;
                QueueMission(Mission::Repair, false);
                sender->QueueMission(Mission::Sleep, false);
                return RadioCommand::AnswerPositive;
            default:
                break;
        }
    }
    if (command == RadioCommand::NotifyUnloaded) {
        if ((Type->UnitRepair || Type->Bunker) && DistanceFrom3D(sender) < 384) return RadioCommand::AnswerPositive;
        TechnoClass::ReceiveCommand(sender, command, data);
        return Type->WeaponsFactory || Type->UnitRepair || Type->Bunker ? RadioCommand::AnswerLeave
                                                                        : RadioCommand::AnswerPositive;
    }
    return TechnoClass::ReceiveCommand(sender, command, data);
}

bool BuildingClass::SetPrimary(bool overrideExisting) {
    if (Type->Factory == AbstractType::None) return false;
    const bool previous = IsPrimaryFactory;
    bool found = false;
    for (auto* building : Owner->Buildings) {
        if (!building->InLimbo && building->IsPrimaryFactory && building->Type->Factory == Type->Factory &&
            building->Type->Naval == Type->Naval) {
            found = true;
            if (!overrideExisting) break;
            building->IsPrimaryFactory = false;
            break;
        }
    }
    if (!found || overrideExisting) IsPrimaryFactory = true;
    return previous != IsPrimaryFactory;
}
