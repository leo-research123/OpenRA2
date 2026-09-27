// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp Per_Cell_Process and sensor/exit queries.
// YR 0x4D85D0, 0x4DE7B0, 0x4DE940, 0x4DC790, 0x4D9FF0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Unsorted.h"

namespace {
int map_trigger_index=0;
CellStruct cell_of(const CoordStruct& at){return {short(at.X/256),short(at.Y/256)};}
CellStruct adjacent(CellStruct at,int face){const auto offset=Unsorted::AdjacentCell[face];return {short(at.X+offset.X),short(at.Y+offset.Y)};}
bool under_bridge(const CellClass* cell){return (static_cast<unsigned>(cell->Flags)&0x100u)!=0;}
void update_sensors(FootClass& foot,CellStruct at,bool add) {
    const int radius=foot.GetTechnoType()->SensorsSight;
    const int owner=foot.Owner->ArrayIndex;
    if(at==CellStruct::Empty)at=cell_of(foot.GetCoords());
    for(int y=-radius;y<radius;++y)for(int x=-radius;x<radius;++x) {
        if(x*x+y*y>=radius*radius)continue;
        auto* cell=MapClass::Instance.GetCellAt(CellStruct{short(at.X+x),short(at.Y+y)});
        if(add)cell->Sensors_AddOfHouse(owner);
        else {if(!cell->Sensors_InclHouse(owner))continue;cell->Sensors_RemOfHouse(owner);}
        for(auto* object=cell->FirstObject;object;object=object->NextObject) {
            const auto kind=object->WhatAmI();
            if(kind==AbstractType::Unit || kind==AbstractType::Infantry || kind==AbstractType::Aircraft)
                static_cast<TechnoClass*>(object)->Sensed();
        }
        auto* building=cell->GetBuilding();
        if(building && building->Owner!=HouseClass::CurrentPlayer
            && static_cast<int>(building->VisualCharacter(0,nullptr)))building->NeedsRedraw=true;
    }
}
}
int& FootClass::MapTriggerID=map_trigger_index;
void FootClass::AddSensorsAt(CellStruct at){update_sensors(*this,at,true);}
void FootClass::RemoveSensorsAt(CellStruct at){update_sensors(*this,at,false);}
CoordStruct* FootClass::vt_entry_4F0(CoordStruct* out){CoordStruct buffer;*out=*GetTargetCoords(&buffer);return out;}
bool FootClass::IsLeavingMap() const {
    if(!IsInPlayfield || (Team && !Team->IsLeavingMapNow()))return false;
    return !GetTechnoType()->HunterSeeker || !Destination || !MapClass::Instance.IsWithinUsableArea(Destination->GetCoords());
}

void FootClass::UpdatePosition(PCPType reason) {
    auto& map=MapClass::Instance;
    if(reason==PCPType::End) {
        unknown_bool_6B2=false;unknown_bool_6B0=false;
        if(GetTechnoType()->SensorsSight){RemoveSensorsAt(LastMapCoords);AddSensorsAt(CellStruct::Empty);}
        if(LastMapCoords!=CellStruct::Empty) {
            const int current=MapClass::CellRegion(GetMapCoords());
            if(MapClass::CellRegion(LastMapCoords)!=current){RemoveThreatFromCell(map.GetCellAt(LastMapCoords));AddThreatToCell(GetCell());}
            for(int face=0;face<8;++face)--map.GetCellAt(adjacent(LastMapCoords,face))->BlockedNeighbours;
            LastMapCoords=GetMapCoords();
            for(int face=0;face<8;++face)++map.GetCellAt(adjacent(LastMapCoords,face))->BlockedNeighbours;
        }
        if(CloakState==::CloakState::Cloaked)for(int face=0;face<8;++face) {
            const auto neighbor=adjacent(GetMapCoords(),face);
            if(!map.IsWithinUsableArea(neighbor,true))continue;
            auto* other=map.GetCellAt(neighbor)->FindTechnoNearestTo({0,0},false,nullptr);
            if(other && !other->Owner->IsAlliedWith(this)
                && (other->GetTechnoType()->Sensors || other->HasAbility(Ability::Sensors))){Reveal();break;}
        }
        const int weapon=SelectWeapon(Target);
        bool in_range=false;
        if(Target && (Target->AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None) {
            auto* foot=static_cast<FootClass*>(Target);
            if(GetTechnoType()->OpenTopped) {
                const auto target=foot->GetCoords(),here=GetCoords();
                const double x=double(here.X)-target.X,y=double(here.Y)-target.Y,z=double(here.Z)-target.Z;
                in_range=int(Math::sqrt(x*x+y*y+z*z))<GetWeaponRange(weapon);
            }else {CoordStruct predicted;in_range=IsCloseEnough3D(*foot->vt_entry_4F0(&predicted),weapon);}
        }
        if((GetCurrentMission()==Mission::Rescue || GetCurrentMission()==Mission::Area_Guard
            || GetCurrentMission()==Mission::Attack || GetCurrentMission()==Mission::Hunt)
            && in_range && !unknown_abstract_array_588.Count){SetDestination(nullptr,true);PathDirections[0]=-1;}
        const auto at=GetMapCoords();auto* cell=map.GetCellAt(at);auto* tag=cell->AttachedTag;
        if(!(static_cast<unsigned>(cell->Flags)&0x500u) || OnBridge)if(tag) {
            tag->RaiseEvent(TriggerEvent::EnteredBy,this,GetMapCoords());
            tag->RaiseEvent(static_cast<TriggerEvent>(59),this,GetMapCoords());
            if(WhatAmI()==AbstractType::Infantry && static_cast<InfantryClass*>(this)->Type->Agent) {
                tag->RaiseEvent(TriggerEvent::SpyAsHouse,this,GetMapCoords());
                tag->RaiseEvent(TriggerEvent::SpyAsInfantry,this,GetMapCoords());
            }
        }
        if((cell->Flags & CellFlags::HorizontalLineEventTag)!=CellFlags{}) {
            for(int x=0;x<map.MapRect.Width;++x) {
                cell=map.GetCellAt(CellStruct{short(map.MapRect.X+x),at.Y});tag=cell->AttachedTag;
                if(tag && tag->HasCrossesHorizontalLineEvent())tag->RaiseEvent(TriggerEvent::CrossesHorizontalLine,this,GetMapCoords());
            }
        }
        // The original uses the LAST horizontal-scan cell here and below.
        if((cell->Flags & CellFlags::VerticalLineEventTag)!=CellFlags{}) {
            for(int y=0;y<map.MapRect.Height;++y) {
                tag=map.GetCellAt(CellStruct{at.X,short(map.MapRect.Y+y)})->AttachedTag;
                if(tag && tag->HasCrossesVerticalLineEvent())tag->RaiseEvent(TriggerEvent::CrossesVerticalLine,this,GetMapCoords());
            }
        }
        for(MapTriggerID=0;MapTriggerID<MapClass::PendingTags.Count;++MapTriggerID) {
            tag=MapClass::PendingTags[MapTriggerID];
            if(tag && tag->HasZoneEntryByEvent()) {
                const auto dest=cell_of(GetDestination());
                const bool to_bridge=IsOnBridge(nullptr);CellStruct tag_at;
                const bool from_bridge=under_bridge(map.GetCellAt(*tag->GetDefaultCoords(&tag_at)));
                const auto movement=GetTechnoType()->MovementZone;CellStruct from;
                if(MapClass::IsSameCellZone(*tag->GetDefaultCoords(&from),dest,movement,from_bridge,to_bridge,false))
                    tag->RaiseEvent(TriggerEvent::ZoneEntryBy,this,cell_of(GetDestination()));
            }
        }
        if(auto* building=cell->GetBuilding();building && building->Type->LaserFence && building->LaserFenceFrame<8) {
            const auto kind=WhatAmI();
            if((kind==AbstractType::Unit || kind==AbstractType::Aircraft || kind==AbstractType::Infantry) && Health>0)
                ReceiveDamage(&Health,0,RulesClass::Instance->C4Warhead,building,true,true,nullptr);
        }
        if(OnBridge && !under_bridge(map.GetCellAt(Location)))DropAsBomb();
        if(!IsAlive)return;
        if(!map.IsWithinUsableArea(GetMapCoords(),true) && IsLeavingMap()){UnInit();return;}
        if(IsSelected && !Owner->IsControlledByCurrentPlayer() && map.IsLocationShrouded(Location))Deselect();
    }
    if(PlanningPathIdx!=-1) {
        auto* waypoint=HouseClass::CurrentPlayer->PlanningPaths[PlanningPathIdx]->GetWaypoint(WaypointIndex);
        if(waypoint) {
            const auto at=cell_of(DisplayClass::Instance.DraggedWaypoint==waypoint
                ?DisplayClass::Instance.DraggedWaypointCoords:waypoint->Coords);
            const CellStruct offset{short(at.X+WaypointNearbyAccessibleCellDelta.X),short(at.Y+WaypointNearbyAccessibleCellDelta.Y)};
            if(offset!=WaypointCell)FollowWaypoint(waypoint);
        }else {PlanningPathIdx=-1;WaypointIndex=0;WaypointNearbyAccessibleCellDelta=WaypointCell=CellStruct::Empty;}
    }
    TechnoClass::UpdatePosition(reason);
}
