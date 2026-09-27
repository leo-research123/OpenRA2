// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 display.cpp Map_Cell / Fog_Map_Cell; YR 0x004A9890,
// 0x004A9CA0 and 0x004A9DD0 add the reference-counted reveal variant.
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
#include "yrpp/DisplayClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/CellSpread.h"
namespace {
bool has(CellFlags value,CellFlags mask) {return (value&mask)!=CellFlags{};}
bool has(AltCellFlags value,AltCellFlags mask) {return (value&mask)!=AltCellFlags{};}
void mark(CellClass* cell) {TacticalClass::Instance->RegisterCellAsVisible(cell);}
bool refresh_frame(CellClass& cell,const CellStruct& at,bool fog) {
    const char frame=TacticalClass::Instance->GetOcclusion(at,fog);
    auto& old=fog?cell.Foggedness:cell.Visibility;
    const bool changed=old!=frame;old=frame;
    if(frame==-1) {
        if(fog)cell.Flags|=CellFlags::CenterRevealed;
        else cell.AltFlags|=AltCellFlags::NoFog;
    }
    return changed;
}
void finish(CellClass* cell,HouseClass* house,bool changed,bool newlyMapped,bool wasFogged) {
    if(changed)MapClass::RevealCheck(cell,house,newlyMapped);
    // The deprecated FoggedObject snapshot path retains its original boundary.
    // Native shipped maps all disable this Scenario flag.
    if(has(cell->Flags,CellFlags::EdgeRevealed) && wasFogged && ScenarioClass::Instance->SpecialFlags.FogOfWar)
        cell->CleanFog();
}
CellStruct neighbour(const CellStruct& at,int index) {
    const auto offset=CellSpread::GetNeighbourOffset(unsigned(index));
    return {short(at.X+offset.X),short(at.Y+offset.Y)};
}
// Both originals repair impossible edge-frame combinations recursively.
void repair_neighbours(DisplayClass& display,const CellStruct& source,HouseClass* house,bool fogOnly) {
    for(int i=0;i<8;++i) {
        auto at=neighbour(source,i);auto* cell=display.GetCellAt(at);
        if(at==source)continue;
        for(int layer=fogOnly?1:0;layer<2;++layer) {
            const bool fog=layer!=0;
            const bool visible=fog?has(cell->Flags,CellFlags::CenterRevealed):has(cell->AltFlags,AltCellFlags::NoFog);
            if(visible)continue;
            const int frame=TacticalClass::Instance->GetOcclusion(at,fog);
            const bool mapped=fog?has(cell->Flags,CellFlags::EdgeRevealed):has(cell->AltFlags,AltCellFlags::Mapped);
            if(frame!=-2 && !mapped) {
                if(fogOnly)display.MapCellFoggedness(&at,house);else display.MapCell(&at,house);
            }else if(frame==-1) {
                if(fog)cell->Flags|=CellFlags::CenterRevealed;else cell->AltFlags|=AltCellFlags::NoFog;
                mark(cell);
                for(int j=0;j<8;++j) {
                    auto adjacent=neighbour(at,j);auto* other=display.GetCellAt(adjacent);
                    const char next=TacticalClass::Instance->GetOcclusion(adjacent,fog);
                    auto& old=fog?other->Foggedness:other->Visibility;
                    if(old!=next){old=next;mark(other);}
                }
            }else if(frame>=0) {
                auto& old=fog?cell->Foggedness:cell->Visibility;
                if(old!=frame){old=char(frame);mark(cell);}
            }
        }
    }
}
}
bool DisplayClass::MapCell(CellStruct* at,HouseClass* house) {
    auto* cell=GetCellAt(*at);
    const bool wasFogged=!has(cell->Flags,CellFlags::EdgeRevealed);
    const bool newlyMapped=wasFogged || !has(cell->AltFlags,AltCellFlags::Mapped);
    cell->Flags=(cell->Flags&~CellFlags(0x42))|CellFlags::EdgeRevealed;
    cell->AltFlags|=AltCellFlags::Mapped;
    bool changed=newlyMapped;
    changed=refresh_frame(*cell,*at,false)||changed;
    changed=refresh_frame(*cell,*at,true)||changed;
    if(changed)mark(cell);
    repair_neighbours(*this,*at,house,false);
    finish(cell,house,changed,newlyMapped,wasFogged);
    return changed;
}
bool DisplayClass::RevealFogShroud(CellStruct* at,HouseClass* house,bool increase) {
    auto* cell=GetCellAt(*at);
    const bool wasFogged=!has(cell->Flags,CellFlags::EdgeRevealed);
    const bool newlyMapped=wasFogged || !has(cell->AltFlags,AltCellFlags::Mapped);
    cell->Flags=(cell->Flags&~CellFlags(0x42))|CellFlags::EdgeRevealed;
    if(increase)cell->IncreaseShroudCounter();else cell->ReduceShroudCounter();
    bool changed=newlyMapped;
    changed=refresh_frame(*cell,*at,false)||changed;
    changed=refresh_frame(*cell,*at,true)||changed;
    if(changed)mark(cell);
    finish(cell,house,changed,newlyMapped,wasFogged);
    return changed;
}
bool DisplayClass::MapCellFoggedness(CellStruct* at,HouseClass* house) {
    auto* cell=GetCellAt(*at);
    const bool wasFogged=!has(cell->Flags,CellFlags::EdgeRevealed);
    cell->Flags=(cell->Flags&~CellFlags(0x42))|CellFlags::EdgeRevealed;
    const bool changed=refresh_frame(*cell,*at,true)||wasFogged;
    if(changed)mark(cell);
    repair_neighbours(*this,*at,house,true);
    finish(cell,house,changed,wasFogged,wasFogged);
    return changed;
}
