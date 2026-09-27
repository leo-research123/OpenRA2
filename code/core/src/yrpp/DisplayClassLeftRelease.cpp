// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 display.cpp Mouse_Left_Release.
// Copyright Electronic Arts / OpenTS; EA Section 7: third_party/opents/LICENSE.md.
// YR 0x004AB9B0: placement handoff, type selection, planning and beacon branches.
#include "yrpp/DisplayClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/VoxClass.h"
#include <new>

#if !defined(RA2_YRPP_GAME)
void DisplayClass::LeftMouseButtonUp(const CoordStruct&, const CellStruct& cell,
                                    ObjectClass* object, Action action, DWORD miniMap) noexcept {
    // Every original return clears attack-move mode, including failed placement;
    // only non-placement paths clear the tentative-selection flag at the tail.
    const auto release = [&] {
        const auto placementCell=[&] {
            return CellStruct{static_cast<short>(int(cell.X)+CurrentFoundation_TopLeftOffset.X),
                              static_cast<short>(int(cell.Y)+CurrentFoundation_TopLeftOffset.Y)};
        };
        if(object && Unsorted::ScenarioStarted && Game::IsActive
            && !Unsorted::ArmageddonMode && object->VisualCharacter(false,nullptr)==VisualType::Hidden)
            object=nullptr;

        if(CurrentBuilding) {
            if(CurrentBuildingType->WhatAmI()==AbstractType::BuildingType) {
                CellStruct at=placementCell();
                unknown_1180=PassesProximityCheck(CurrentBuildingType,CurrentBuildingOwnerArrayIndex,
                                                CurrentFoundation_Data,&at);
            }
            if(CurrentBuildingType->WhatAmI()==AbstractType::BuildingType && object
                && object->WhatAmI()==AbstractType::Building
                && static_cast<BuildingClass*>(object)->CanUpgrade(
                    static_cast<BuildingTypeClass*>(CurrentBuildingType),HouseClass::CurrentPlayer))
                unknown_1180=true;
            if(!unknown_1180 || !unknown_1181) {
                VoxClass::Play("EVA_CannotDeployHere",-1,-1);
                return;
            }

            const auto* technoType=dynamic_cast<TechnoTypeClass*>(CurrentBuildingType);
            const bool naval=technoType && technoType->Naval;
            const CellStruct at=placementCell();
            // Original snapshots the object before the virtual type query.
            auto* pending=CurrentBuilding;
            const int typeIndex=CurrentBuildingType->GetArrayIndex();
            const auto rtti=pending->WhatAmI();
            EventClass event; // Only unused original stack bytes are initialized.
            ::new(&event) EventClass(HouseClass::CurrentPlayer->ArrayIndex,
                                    EventType::Place,rtti,typeIndex,naval,at);
            EventClass::OutList.Add(event);
            // Re-read after event construction/queueing, as in the original.
            pending=CurrentBuilding;
            Game::ClearSidebarTabObject(pending && pending->WhatAmI()==AbstractType::Building
                                        ? static_cast<BuildingClass*>(pending):nullptr);
            if(Unsorted::ArmageddonMode)return;

            CurrentBuildingTypeCopy=CurrentBuildingType;
            CurrentBuildingCopy=CurrentBuilding;
            CurrentFoundationCopy_CenterCell=cell;
            CurrentFoundationCopy_TopLeftOffset=CurrentFoundation_TopLeftOffset;
            CurrentFoundationCopy_Data=CurrentFoundation_Data;
            CurrentBuildingOwnerArrayIndexCopy=CurrentBuildingOwnerArrayIndex;
            // Foundation calls and final pointer writes target the global
            // display, even if this entry is invoked on another instance.
            if(CurrentBuildingType)Instance.SetActiveFoundationCopy(CurrentBuildingType->GetFoundationData(true));
            Instance.SetActiveFoundation(nullptr);
            Instance.CurrentBuilding=nullptr;
            Instance.CurrentBuildingType=nullptr;
            Instance.CurrentBuildingOwnerArrayIndex=-1;
            return;
        }

        if(LeftPressAndDraggingRectangle) {
            TacticalClass::Instance->Redrawing=true;
            bool continueClick=false;
            if(!InputManagerClass::Instance->IsKeyPressed(16)) {
                if(TacticalClass::Instance->HasBandObjects())MapClass::UnselectAll();
                else continueClick=true;
            }
            TacticalClass::Instance->SelectRubberBand(BandboxSelectionCallback);
            TacticalClass::StartDrawActionLineTimer();
            LeftPressAndDraggingRectangle=false;
            SetCursor(MouseCursorType::Default,static_cast<bool>(static_cast<BYTE>(miniMap)));
            unknown_bool_11D0=false;
            Unsorted::DragSelectAborted=true;
            if(!continueClick)return;
        }

        bool toggled=false;
        if(action==Action::ToggleSelect) {
            if(object && ObjectClass::CurrentObjects.Count
                && ObjectClass::CurrentObjects[0]->GetOwningHouse()->IsControlledByCurrentPlayer()) {
                if(object->IsSelected) {
                    if(Game::IsTypeSelecting())Game::UICommands_TypeDeselect(object->GetType()->ID);
                    else object->Deselect();
                } else {
                    if(Game::IsTypeSelecting())Game::UICommands_TypeSelect_7327D0(object->GetType()->ID);
                    else object->Select();
                }
                toggled=true; // This branch dispatches ActiveClick without planning checks.
            } else action=Action::Select;
        }

        if(action==Action::None || action==Action::Select) {
            if(object && (action==Action::Select || (object->CanBeSelected() && !object->IsSelected))) {
                bool selectable=object->WhatAmI()!=AbstractType::Building;
                if(!selectable) {
                    const auto* building=static_cast<BuildingClass*>(object);
                    selectable=building->Owner->IsControlledByCurrentPlayer() || building->Translucency!=15
                        || building->IsSensorVisibleToPlayer();
                }
                if(selectable) {
                    MapClass::UnselectAll();
                    if(Game::IsTypeSelecting())Game::UICommands_TypeSelect_7327D0(object->GetType()->ID);
                    else object->Select();
                    SetCursor(MouseCursorType::Default,static_cast<bool>(static_cast<BYTE>(miniMap)));
                    TacticalClass::StartDrawActionLineTimer();
                }
            }
        } else if(action==Action::SelectBeacon) {
            const int z=static_cast<signed char>(MapClass::Instance.GetCellAt(cell)->Level)*Unsorted::LevelHeight;
            BeaconManagerClass::Instance.SelectBeacon(int(cell.X)*256+128,int(cell.Y)*256+128,z);
        } else {
            bool allowed=toggled;
            switch(action) {
            // 0x004ABEDC explicitly uses action 10 (Eaten), not action 11 (Repair).
            case Action::Eaten: case Action::Sell: case Action::SellUnit:
            case Action::NoSell: case Action::NoRepair: case Action::TogglePower:
            case Action::NoTogglePower: case Action::Nuke: case Action::IronCurtain:
            case Action::ForceShield: case Action::NoForceShield: case Action::LightningStorm:
            case Action::ChronoSphere: case Action::ChronoWarp: case Action::ParaDrop:
            case Action::AmerParaDrop: case Action::PsychicDominator: case Action::SpyPlane:
            case Action::GeneticConverter: case Action::PsychicReveal: case Action::SelectNode:
            case Action::PlaceBeacon: allowed=true; break;
            default: break;
            }
            if(!allowed) {
                const bool selection=Game::PlanningManager_CheckSelection();
                const bool capacity=Game::PlanningManager_CheckCapacity();
                allowed=selection && capacity; // Both queries run even if the first fails.
            }
            if(allowed) {
                ActiveClick(object,cell,action);
                TacticalClass::StartDrawActionLineTimer();
                const auto queueTarget=[&](EventType type) {
                    const TargetClass target(object);
                    EventClass event;
                    ::new(&event) EventClass(HouseClass::CurrentPlayer->ArrayIndex,type,target.m_ID,target.m_RTTI);
                    EventClass::OutList.Add(event);
                };
                if(action==Action::TogglePower) {
                    if(object->WhatAmI()==AbstractType::Building)
                        queueTarget(static_cast<BuildingClass*>(object)->HasPower ? EventType::PowerOff:EventType::PowerOn);
                } else if(action==Action::PlaceBeacon) {
                    if(!Unsorted::MuteSWLaunches) {
                        const int z=static_cast<signed char>(MapClass::Instance.GetCellAt(cell)->Level)*Unsorted::LevelHeight;
                        BeaconManagerClass::Instance.PlaceBeacon(HouseClass::CurrentPlayer->ArrayIndex,
                            {int(cell.X)*256+128,int(cell.Y)*256+128,z},-1);
                    }
                    Instance.SetBeaconMode(0);
                    SetCursor(MouseCursorType::Default,static_cast<bool>(static_cast<BYTE>(miniMap)));
                } else if(action==Action::Eaten) {
                    if(object->WhatAmI()==AbstractType::Building)queueTarget(EventType::Repair);
                } else if(action==Action::SellUnit) {
                    if(object) {
                        const int rtti=static_cast<int>(object->WhatAmI());
                        if(rtti>0 && rtti<=2)queueTarget(EventType::Sell);
                    }
                } else if(action==Action::Sell) {
                    if(object)queueTarget(EventType::Sell);
                    else {
                        EventClass event;
                        ::new(&event) EventClass(HouseClass::CurrentPlayer->ArrayIndex,EventType::SellCell,cell);
                        EventClass::OutList.Add(event);
                    }
                }
                if(auto* type=SuperWeaponTypeClass::FindFirstOfAction(action)) {
                    EventClass event;
                    ::new(&event) EventClass(HouseClass::CurrentPlayer->ArrayIndex,EventType::SpecialPlace,type->ArrayIndex,cell);
                    EventClass::OutList.Add(event);
                }
            }
        }
        unknown_bool_11D0=false;
    };
    release();
    Game::AttackMoveMode=false;
}
#endif
