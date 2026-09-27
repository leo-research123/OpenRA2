// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 cell.cpp: Occupy_Down/Up, Spot_Index, Closest_Free_Spot.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR differences: three assignable spots, original RNG consumption, gate test.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/CellClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "scenario_runtime.hpp"
#include <bit>
#include <cstdlib>

TubeClass* CellClass::GetTunnel() { return TubeClass::Array.GetItemOrDefault(TubeIndex); }
bool CellClass::IsOnFloor() const {
    return IsoTileTypeIndex<IsometricTileTypeClass::WaterSet
        || IsoTileTypeIndex>=IsometricTileTypeClass::WaterSet+14;
}
bool CellClass::ConnectsToOverlay(int overlay,int direction) const {
    if(OverlayTypeIndex==overlay&&overlay!=-1)return true;
    if(overlay==-1)return OverlayTypeIndex==2||OverlayTypeIndex==26||OverlayTypeIndex==243;
    if(overlay!=0&&overlay!=2&&overlay!=26)return false;
    auto* rules=RulesClass::Instance;
    for(auto* object=FirstObject;object;object=object->NextObject){
        if(object->WhatAmI()!=AbstractType::Building||object->Health<=0)continue;
        auto* type=static_cast<BuildingClass*>(object)->Type;
        if(overlay==26){
            if((type==rules->NodGateOne&&(direction==2||direction==6))
                ||(type==rules->NodGateTwo&&(direction==0||direction==4)))return true;
        }else if((type==rules->GDIGateOne&&(direction==2||direction==6))
            ||(type==rules->GDIGateTwo&&(direction==0||direction==4))||type==rules->WallTower)return true;
    }
    return false;
}

// OpenTS Cell_Unit/Cell_Aircraft/Cell_Infantry; YR 0x47EBA0/0x47EBF0/0x47EC40.
// These scan exactly one original chain, including inactive entries, and are
// disabled outside GameActive. Do not reuse the native foundation fallback.
namespace {
ObjectClass* typed_content(const CellClass& cell,AbstractType type,bool alt) {
    if(!Game::IsActive)return nullptr;
    for(auto* object=alt?cell.AltObject:cell.FirstObject;object;object=object->NextObject)
        if(object->WhatAmI()==type)return object;
    return nullptr;
}
}
UnitClass* CellClass::GetUnit(bool alt) const {return static_cast<UnitClass*>(typed_content(*this,AbstractType::Unit,alt));}
AircraftClass* CellClass::GetAircraft(bool alt) const {return static_cast<AircraftClass*>(typed_content(*this,AbstractType::Aircraft,alt));}
InfantryClass* CellClass::GetInfantry(bool alt) const {return static_cast<InfantryClass*>(typed_content(*this,AbstractType::Infantry,alt));}

// OpenTS Cell_Object; YR 0x47C5A0 deliberately searches the ground chain
// for aircraft/terrain, with only the middle techno query using alt.
ObjectClass* CellClass::GetSomeObject(const Point2D& offset,bool alt) const {
    if(auto* aircraft=typed_content(*this,AbstractType::Aircraft,false))return aircraft;
    if(auto* techno=FindTechnoNearestTo(offset,alt,nullptr))return techno;
    return typed_content(*this,AbstractType::Terrain,false);
}

TechnoClass* CellClass::FindTechnoNearestTo(const Point2D& offset, bool alt, const TechnoClass* exclude) const {
    TechnoClass* nearest = nullptr;
    int distance = 0;
    const auto wrap = [](unsigned v) { return std::bit_cast<int>(v); };
    const int x = wrap(7u * static_cast<unsigned>(offset.X));
    const int y = wrap(7u * static_cast<unsigned>(offset.Y));
    for (auto* object = alt ? AltObject : FirstObject; object; object = object->NextObject) {
        if ((static_cast<unsigned>(object->AbstractFlags) & 1u) && object != exclude) {
            const auto at = object->GetCoords();
            const int dx = wrap(static_cast<unsigned>(static_cast<unsigned char>(at.X)) - static_cast<unsigned>(x));
            const int dy = wrap(static_cast<unsigned>(static_cast<unsigned char>(at.Y)) - static_cast<unsigned>(y));
            const double length = Math::sqrt(double(dx)*dx + double(dy)*dy);
            const int next = length >= 2147483648.0 ? INT32_MIN : static_cast<int>(length);
            if (!nearest || next < distance) { nearest = static_cast<TechnoClass*>(object); distance = next; }
        }
    }
    return nearest;
}

bool CellClass::CollectCrate(FootClass* collector) {
    if (!collector || OverlayTypeIndex == -1 || !OverlayTypeClass::Array[OverlayTypeIndex]->Crate) return true;
    // The effect branch of 0x481A00 is still unported (money/unit/weapon effects).
    // Do not silently consume a crate or pretend it was empty in a native game.
    std::abort();
}

void CellClass::AddContent(ObjectClass* object, bool bridge) {
    if (!object) return;
    auto*& head = bridge ? AltObject : FirstObject;
    if (object->WhatAmI() == AbstractType::Building && head) {
        auto* tail = head;
        while (tail->NextObject) tail = tail->NextObject;
        tail->NextObject = object;
        object->NextObject = nullptr;
    } else if (!head || head->NextObject != object) {
        object->NextObject = head;
        head = object;
    }
    const auto coord = GetCoords();
    const auto& runtime = game::scenario_runtime();
    const int mode = runtime.session_mode(runtime.context);
    if ((!MapClass::Instance.IsLocationShrouded(coord) &&
         !MapClass::Instance.IsLocationFogged(coord)) ||
        (mode >= 0 && mode != static_cast<int>(GameMode::Campaign)))
        object->DiscoveredBy(HouseClass::CurrentPlayer);
    // Existing YRpp name for original Occupies_Cells at virtual slot 0xC0.
    // Infantry reserves its destination separately in the driver.
    if (object->WhatAmI() != AbstractType::Infantry && object->IsStandingStill()) {
        const CoordStruct at = object->WhatAmI() == AbstractType::Building
            ? CoordStruct{MapCoords.X * 256 + 128, MapCoords.Y * 256 + 128, 0}
            : object->Location;
        object->MarkAllOccupationBits(at);
    }
}

void CellClass::RemoveContent(ObjectClass* object, bool bridge) {
    if (!object) return;
    auto** link = bridge ? &AltObject : &FirstObject;
    while (*link && *link != object) link = &(*link)->NextObject;
    if (*link) {
        *link = object->NextObject;
        object->NextObject = nullptr;
    }
    // Original clears non-infantry bits even when the pointer was not found.
    if (object->WhatAmI() != AbstractType::Infantry && object->IsStandingStill()) {
        const CoordStruct at = object->WhatAmI() == AbstractType::Building
            ? CoordStruct{MapCoords.X * 256 + 128, MapCoords.Y * 256 + 128, 0}
            : object->Location;
        object->UnmarkAllOccupationBits(at);
    }
}

unsigned char YRPP_FASTCALL CellClass::InfantrySubpositionIndex(const CoordStruct& coord) {
    const int x = static_cast<unsigned char>(coord.X);
    const int y = static_cast<unsigned char>(coord.Y);
    const int dx = x - 128, dy = y - 128;
    if (static_cast<int>(Math::sqrt(double(dx * dx + dy * dy))) < 60) return 0;
    const unsigned quadrant = unsigned(x > 128) | (unsigned(y > 128) << 1);
    return quadrant ? quadrant + 1 : 0;
}

CoordStruct* CellClass::FindInfantrySubposition(CoordStruct* out,
    const CoordStruct& input, bool ignore, bool bridge, bool useCellCoords) {
    if (!out) return nullptr;
    const auto coord = input; // out may alias input
    unsigned spot = InfantrySubpositionIndex(coord);
    const auto base = useCellCoords ? GetCoords() : coord;
    const auto flags = bridge ? AltOccupationFlags : OccupationFlags;
    if (!ignore) {
        if (flags & 0x20u) { *out = CoordStruct::Empty; return out; }
        if (OccupationFlags & 0x40u) {
            auto* gate = static_cast<BuildingClass*>(FindObjectOfType(AbstractType::Building, false));
            if (!gate || !gate->Type || !gate->Type->Gate ||
                gate->CurrentMission != Mission::Open || !gate->UnloadTimer.AreStates01()) {
                *out = CoordStruct::Empty; return out;
            }
        }
        constexpr unsigned char order[5][4] {
            {1,2,3,4}, {0,2,3,4}, {0,1,4,3}, {0,1,4,2}, {0,2,3,1}};
        constexpr unsigned char alternate[4][4] {
            {1,2,3,4}, {2,3,4,1}, {3,4,1,2}, {4,1,2,3}};
        const unsigned char* candidates = nullptr;
        if (spot == 0) {
            if (!ScenarioClass::Instance) std::abort();
            candidates = alternate[ScenarioClass::Instance->Random.RandomRanged(0,3)];
        } else if (spot == 1 || (flags & (1u << spot))) {
            candidates = order[spot];
        }
        if (candidates) {
            bool found = false;
            for (unsigned i = 0; i < 4; ++i) {
                const auto next = candidates[i];
                if (next >= 2 && !(flags & (1u << next))) {
                    spot = next; found = true; break;
                }
            }
            if (!found) { *out = CoordStruct::Empty; return out; }
        }
    }
    constexpr Point2D offsets[] {{128,128},{64,64},{192,64},{64,192},{192,192}};
    *out = {base.X - static_cast<unsigned char>(base.X) + offsets[spot].X,
            base.Y - static_cast<unsigned char>(base.Y) + offsets[spot].Y,
            MapClass::Instance.GetCellFloorHeight(coord) + (bridge ? BridgeHeight : 0)};
    return out;
}

// OpenTS Incoming; YR 0x481670 snapshots the chain before callbacks move it.
void CellClass::ScatterContent(const CoordStruct& from,bool force,bool urgent,bool alt) {
 bool elite=false;
 if(!urgent)for(auto* object=alt?AltObject:FirstObject;object;object=object->NextObject)
  if((object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None&&static_cast<TechnoClass*>(object)->Veterancy.IsElite()){elite=true;break;}
 DynamicVectorClass<ObjectClass*> objects;
 for(auto* object=alt?AltObject:FirstObject;object;object=object->NextObject)objects.AddItem(object);
 for(auto* object:objects){
  auto* techno=(object->AbstractFlags&AbstractFlags::Techno)!=AbstractFlags::None?static_cast<TechnoClass*>(object):nullptr;
  if(elite||urgent||RulesClass::Instance->PlayerScatter||(techno&&(techno->HasAbility(Ability::Scatter)||techno->Owner->IQLevel2>=RulesClass::Instance->Scatter)))
   object->Scatter(from,force,urgent);
 }
}
