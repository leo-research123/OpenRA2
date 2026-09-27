// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp Place_Down/Pick_Up; YR 0x5683C0/0x5687F0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <bit>
#include <cstdlib>
namespace {
constexpr CellStruct end{0x7FFF,0x7FFF};
void footprint_copy(CellStruct (&out)[32],const CellStruct* source){
    // List_Copy 0x48DEE0 reserves the last entry for REFRESH_EOL.
    for(int i=0;i<32;++i){out[i]=source[i];if(out[i]==end)return;}
    out[31]=end;
}
CellClass* valid_cell(MapClass& map,CellStruct at){
    const int index=MapClass::GetCellIndex(at);
    return index>=0 && index<map.Cells.Capacity?map.Cells[index]:nullptr;
}
void adjust_height(CellClass& cell,bool down){
    if(down)cell.OccupyHeightsCoveringMe=std::bit_cast<int>(static_cast<unsigned>(cell.OccupyHeightsCoveringMe)+1u);
    else if(cell.OccupyHeightsCoveringMe)cell.OccupyHeightsCoveringMe=std::bit_cast<int>(static_cast<unsigned>(cell.OccupyHeightsCoveringMe)-1u);
}
void content(MapClass& map,CellStruct anchor,ObjectClass* object,bool down){
    if(!object || !object->GetType()->AllowCellContent || object->InWhichLayer()!=Layer::Ground)return;
    CellStruct footprint[32]{};footprint_copy(footprint,object->GetFoundationData(false));
    for(const auto* entry=footprint;*entry!=end;++entry){
        const CellStruct at{short(anchor.X+entry->X),short(anchor.Y+entry->Y)};
        if(auto* cell=valid_cell(map,at)){
            if(down)cell->AddContent(object,object->OnBridge);else cell->RemoveContent(object,object->OnBridge);
            // Deliberately re-fetch: occupancy callbacks can mutate the map.
            map.GetCellAt(at)->RecalcAttributes(-1);
        }
    }
    if(object->WhatAmI()!=AbstractType::Building)return;
    const auto* type=static_cast<BuildingClass*>(object)->Type;
    if(!type->CanHideThings)return;
    const int height=std::max(type->OccupyHeight-1,1);
    bool covered[2048]{};
    for(const auto* entry=footprint;*entry!=end;++entry){
        int x=entry->X,y=entry->Y;CellStruct at{short(anchor.X+x),short(anchor.Y+y)};
        for(int i=0;i<height;++i){
            if(at.X<0 || at.Y<0)continue;
            if(x<0)x+=16;if(y<0)y+=16;
            const int index=y*16+x;
            if(index<0 || index>=2048)std::abort(); // Invalid footprint would overrun the target's stack array.
            if(!covered[index]){covered[index]=true;if(auto* cell=valid_cell(map,at))adjust_height(*cell,down);}
            --x;--y;--at.X;--at.Y;
        }
    }
    for(int i=0;i<8;++i){
        // Keep YR's order and asymmetry, including the legacy field names.
        const auto a=type->RemoveOccupy[i];
        if(a.X!=0xFFFF || a.Y!=0xFFFF){
            if(auto* cell=valid_cell(map,{short(anchor.X+a.X),short(anchor.Y+a.Y)}))adjust_height(*cell,down);
        }
        if(down){const auto b=type->AddOccupy[i];
            if(b.X!=0xFFFF || b.Y!=0xFFFF)
                if(auto* cell=valid_cell(map,{short(anchor.X+b.X),short(anchor.Y+b.Y)}))adjust_height(*cell,false);
        }
    }
}
}
void MapClass::AddContentAt(CellStruct* at,ObjectClass* object){if(object)content(*this,*at,object,true);}
void MapClass::RemoveContentAt(CellStruct* at,ObjectClass* object){if(object)content(*this,*at,object,false);}
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(offsetof(BuildingTypeClass,OccupyHeight)==0xEF8);
static_assert(offsetof(BuildingTypeClass,RemoveOccupy)==0x1624);
static_assert(offsetof(BuildingTypeClass,AddOccupy)==0x1664);
static_assert(offsetof(BuildingTypeClass,CanHideThings)==0x1766);
#endif

// OpenTS Try_Open_Gate; YR 0x578AD0 uses the ground content chain.
bool MapClass::MakeTraversable(const ObjectClass* visitor,const CellStruct& at) const {
 for(auto* object=GetCellAt(at)->FirstObject;object;object=object->NextObject){
  if(object==visitor||object->WhatAmI()!=AbstractType::Building)continue;
  auto* building=static_cast<BuildingClass*>(object);
  if(!building->Type->Gate)continue;
  if(building->Owner->IsAlliedWith(visitor))return building->MakeTraversable();
  if(building->IsTraversable())return true;
 }
 return true;
}

// OpenTS Closest_Free_Spot; YR 0x4ACA10 resolves the bridge layer before
// forwarding to the original cell's infantry subposition allocator.
CoordStruct* YRPP_STDCALL MapClass::PickInfantrySublocation(CoordStruct& output,const CoordStruct& coords,bool ignoreContents){
    auto* cell=Instance.GetCellAt(coords);
    const bool bridge=(unsigned(cell->Flags)&0x100u)&&coords.Z>=Unsorted::LevelHeight*(cell->Level+4);
    output=cell->FindInfantrySubposition(coords,ignoreContents,bridge,false);
    return &output;
}
