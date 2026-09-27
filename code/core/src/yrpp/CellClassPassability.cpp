// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 cell.cpp Recalc_Passability, Is_Clear_To_Move and
// Can_Burrow_Here, calibrated to YR 0x483C80 / 0x4834A0 / 0x486FF0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/Unsorted.h"

// OpenTS Can_Burrow_Here, calibrated to YR 0x486FF0.
bool CellClass::CanBuildHere() const {
    // OpenTS Can_Build_Here; YR 0x487C10 checks the ground chain, not
    // a host building registry, and ignores its objects outside GameActive.
    if(SlopeIndex)return false;
    if(Game::IsActive)for(auto* object=FirstObject;object;object=object->NextObject)
        if(object->WhatAmI()==AbstractType::Building)return false;
    return GroundType::Array[static_cast<int>(LandType)].Buildable;
}

bool CellClass::CanBurrowHere() const {
    if(!MapClass::Instance.IsWithinUsableArea(const_cast<CellClass*>(this),true))return true;
    if((IsoTileTypeIndex>=0 && IsoTileTypeIndex<IsometricTileTypeClass::Array.Count
            && !IsometricTileTypeClass::Array[IsoTileTypeIndex]->AllowBurrowing)
        || SlopeIndex || (static_cast<unsigned>(Flags)&0x500u))return false;
    // Exact 0x47C520 / 0x47C550 ground-chain query, without the older native
    // resource helpers' registry/foundation fallback or live-object filtering.
    if(Game::IsActive)for(auto* object=FirstObject;object;object=object->NextObject)
        if(object->WhatAmI()==AbstractType::Building || object->WhatAmI()==AbstractType::Terrain)return false;
    return true;
}

bool CellClass::IsClearToMove(SpeedType speed,bool ignoreInfantry,bool ignoreVehicles,int zone,MovementZone movement,int level,bool bridge) {
    if(static_cast<int>(speed)==4)return true;
    if(zone!=-1 && zone!=MapClass::Instance.GetMovementZoneType(MapCoords,movement,bridge))return false;
    const bool under=(static_cast<unsigned>(Flags)&0x100u)!=0;
    const int floor=static_cast<signed char>(Level);
    if(level!=-1){
        if(level==floor){if(under && !bridge)return false;}
        else if(!under || level!=floor+4)return false;
    }
    const bool alt=under && (level==-1 || level==floor+4);
    unsigned bits=static_cast<unsigned char>(alt?AltOccupationFlags:OccupationFlags);
    if(ignoreInfantry)bits&=0xE0u;
    if(ignoreVehicles)bits&=0x5Fu;
    if(bits)return false;
    auto land=LandType;
    if(OverlayTypeIndex!=-1){const auto* overlay=OverlayTypeClass::Array[OverlayTypeIndex];
        if(overlay && overlay->Wall){const int zone_type=static_cast<int>(movement);
            if(zone_type!=2 && zone_type!=3 && zone_type!=8 && ((zone_type!=1 && zone_type!=4) || !overlay->Crushable) && zone_type!=12)return false;
            land=::LandType::Clear;
        }
    }
    return GroundType::Array[static_cast<int>(land)].Cost[static_cast<int>(speed)]!=0.0f || alt;
}
void CellClass::RecalcPassability() {
    const auto set=[&](int value){Passability=static_cast<PassabilityType>(value);};
    if(!MapClass::Instance.IsWithinUsableArea(MapCoords,true)){set(7);return;}
    if(OverlayTypeIndex!=-1){const auto* overlay=OverlayTypeClass::Array[OverlayTypeIndex];
        if(overlay->Crushable){set(1);return;}
        if(overlay->Wall){set(2);return;}
        if(GroundType::Array[static_cast<int>(overlay->LandType)].Cost[2]==0 || overlay->IsVeins){set(6);return;}
        if(overlay->IsVeinholeMonster){set(0);return;}
    }
    if(LandType==::LandType::Water){set(4);return;}
    if(LandType==::LandType::Beach){set(3);return;}
    if(GroundType::Array[static_cast<int>(LandType)].Cost[2]<=0.01){set(6);return;}
    for(auto* object=FirstObject;object;object=object->NextObject) {
        if(object->WhatAmI()==AbstractType::Building){const auto* building=static_cast<const BuildingClass*>(object);
            if(building->Type->FirestormWall){if(building->Owner->FirestormActive){set(6);return;}}
            else if(building->Type->LaserFence && building->LaserFenceFrame!=12 && building->LaserFenceFrame!=8)set(6);
            // Original continues, ultimately overwriting this laser-fence value.
        }else if(object->WhatAmI()==AbstractType::Terrain){const auto* terrain=static_cast<const TerrainClass*>(object);
            const int bits=ScenarioClass::Instance->Theater==TheaterType::Snow?terrain->Type->SnowOccupationBits:terrain->Type->TemperateOccupationBits;
            set(bits==7?2:5);return;
        }
    }
    set(0);
}
