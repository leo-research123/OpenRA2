// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp Sight_From; YR 0x005673A0 / 0x005678E0.
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MPGameModeClass.h"
#include "yrpp/SessionClass.h"
#include "scenario_runtime.hpp"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif
namespace {
// 309 entries match original initializer 0x005638D0 byte-for-byte.
constexpr CellStruct occlusion[]{
{0,0},{-1,1},{0,1},{1,1},{1,0},{-1,0},{1,-1},{0,-1},{-1,-1},{1,1},{0,1},{-1,1},
{1,1},{-1,1},{1,0},{-1,0},{1,-1},{-1,-1},{1,-1},{0,-1},{-1,-1},{0,1},{0,1},{0,1},
{1,1},{-1,1},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,-1},{-1,-1},{0,-1},{0,-1},
{0,-1},{0,1},{0,1},{0,1},{1,1},{1,1},{-1,1},{-1,1},{1,1},{-1,1},{1,0},{-1,0},
{1,0},{-1,0},{1,0},{-1,0},{1,-1},{-1,-1},{1,-1},{1,-1},{-1,-1},{-1,-1},{0,-1},{0,-1},
{0,-1},{0,1},{0,1},{0,1},{1,1},{1,1},{-1,1},{-1,1},{1,1},{-1,1},{1,1},{-1,1},
{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,-1},{-1,-1},{1,-1},{-1,-1},{1,-1},{1,-1},
{-1,-1},{-1,-1},{0,-1},{0,-1},{0,-1},{0,1},{0,1},{0,1},{1,1},{0,1},{0,1},{-1,1},
{1,1},{-1,1},{1,1},{-1,1},{1,0},{1,0},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},
{1,0},{-1,0},{1,-1},{-1,-1},{1,-1},{-1,-1},{1,-1},{0,-1},{0,-1},{-1,-1},{0,-1},{0,-1},
{0,-1},{0,1},{0,1},{0,1},{1,1},{0,1},{0,1},{-1,1},{1,1},{1,1},{-1,1},{-1,1},
{1,1},{-1,1},{1,1},{-1,1},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},
{1,0},{-1,0},{1,-1},{-1,-1},{1,-1},{-1,-1},{1,-1},{1,-1},{-1,-1},{-1,-1},{1,-1},{0,-1},
{0,-1},{-1,-1},{0,-1},{0,-1},{0,-1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},
{1,1},{1,1},{-1,1},{-1,1},{1,1},{-1,1},{1,1},{-1,1},{1,0},{-1,0},{1,0},{-1,0},
{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,-1},{-1,-1},
{1,-1},{-1,-1},{1,-1},{1,-1},{-1,-1},{-1,-1},{0,-1},{0,-1},{0,-1},{0,-1},{0,-1},{0,-1},
{0,-1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{1,1},{1,1},{-1,1},{-1,1},
{1,1},{-1,1},{1,1},{-1,1},{1,1},{-1,1},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},
{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,-1},{-1,-1},{1,-1},{-1,-1},
{1,-1},{-1,-1},{1,-1},{1,-1},{-1,-1},{-1,-1},{0,-1},{0,-1},{0,-1},{0,-1},{0,-1},{0,-1},
{0,-1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{1,1},{1,1},{-1,1},{-1,1},
{1,1},{1,1},{-1,1},{-1,1},{1,1},{-1,1},{1,1},{-1,1},{1,1},{-1,1},{1,0},{-1,0},
{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},{1,0},{-1,0},
{1,-1},{-1,-1},{1,-1},{-1,-1},{1,-1},{-1,-1},{1,-1},{1,-1},{-1,-1},{-1,-1},{1,-1},{1,-1},
{-1,-1},{-1,-1},{0,-1},{0,-1},{0,-1},{0,-1},{0,-1},{0,-1},{0,-1},
};
const SessionClass* session() {
#if defined(RA2_YRPP_GAME)
    return &SessionClass::Instance;
#else
    const auto* houses=game::scenario_runtime().houses;
    return houses?houses->session:nullptr;
#endif
}
bool special_multiplayer() {
    const auto* current=session();
    return current && (current->GameMode==GameMode::LAN || current->GameMode==GameMode::Internet) &&
        current->MPGameMode && !current->MPGameMode->vt_entry_04();
}
bool in_map(const MapClass& map,CellStruct at) {
    const int sum=at.X+at.Y,width=map.MapRect.Width;
    return sum>width && at.X-at.Y<width && at.Y-at.X<width && sum<=width+2*map.MapRect.Height;
}
bool reserved_cell(const MapClass& map,const CellClass& cell) {
    const int x=cell.MapCoords.X,y=cell.MapCoords.Y,width=map.MapRect.Width,height=map.MapRect.Height;
    return (x==7 && y==width+5) || (x==13 && y==width+11) || (x==height+13 && y==width+height-15);
}
HouseClass* sight_owner(HouseClass* house) {
    auto* current=HouseClass::CurrentPlayer;
    if(house && current && house!=current) {
        if(house->RadarVisibleTo.data&(1u<<(unsigned(current->Type->ArrayIndex2)&31)))house=current;
        if(house!=current && house->IsAlliedWith(current) && RulesClass::Instance->AllyReveal)house=current;
    }
    return house;
}
// The paired entry updates a reference count even on already mapped cells;
// the one-shot entry tests existing visibility and finishes with RevealArea3.
template<bool Paired>
void reveal(MapClass& map,CoordStruct* position,int radius,HouseClass* house,
            BYTE incremental,BYTE dontMap,BYTE unfog,BYTE byHeight,BYTE last) {
    const int level=position->Z/Unsorted::LevelHeight;
    auto projected=*position;
    const auto project=[&](int coordinate) {
        const int shift=TacticalClass::AdjustForZ(projected.Z)/-30;
        return std::bit_cast<int>(unsigned(coordinate)+unsigned(shift)*256u);
    };
    projected.X=project(projected.X);projected.Y=project(projected.Y);
    const CellStruct center{short(projected.X/256),short(projected.Y/256)};
    const CellStruct heightOffset{short(center.X-position->X/256-2),short(center.Y-position->Y/256-2)};
    bool special=false;
    if constexpr(Paired)special=special_multiplayer();
    if(!in_map(map,center) || !radius)return;
    radius=std::min(radius,10);
    const auto first=!RulesClass::Instance->RevealByHeight && incremental && radius>2
        ?CellSpread::NumCells(unsigned(radius-3)):0;
    const auto end=CellSpread::NumCells(unsigned(radius));
    house=sight_owner(house);
    if(house!=HouseClass::CurrentPlayer)return;
    if constexpr(!Paired)special=special_multiplayer();
    for(auto i=first;i<end;++i) {
        const auto offset=CellSpread::GetCell(i);
        CellStruct at{short(center.X+offset.X),short(center.Y+offset.Y)};
        if(!in_map(map,at) || std::abs(int(at.X)-center.X)>radius)continue;
        const int dx=short(at.X-center.X),dy=short(at.Y-center.Y);
        if(short(game::x87_integer(Math::sqrt(double(dx)*dx+double(dy)*dy)))>radius)continue;
        auto* cell=map.GetCellAt(at);
        if(byHeight && RulesClass::Instance->RevealByHeight) {
            const CellStruct raised{short(at.X-heightOffset.X+occlusion[i].X),short(at.Y-heightOffset.Y+occlusion[i].Y)};
            if(MapClass::Instance.GetCellAt(raised)->Level>level+3)continue;
        }
        cell->Flags&=~CellFlags::IsPlot;
        if constexpr(!Paired)if(special && reserved_cell(map,*cell))continue;
        if(unfog) {
            if((cell->Flags&CellFlags::Revealed)!=CellFlags::Revealed &&
               (cell->AltFlags&AltCellFlags::Mapped)!=AltCellFlags{})
                RadarClass::Instance.DisplayClass::MapCellFoggedness(&at,house);
        }else if constexpr(Paired) {
            if(!special || !reserved_cell(map,*cell))RadarClass::Instance.RevealFogShroud(&at,house,last!=0);
        }else if((cell->AltFlags&AltCellFlags::Clear)!=AltCellFlags::Clear ||
                 (cell->Flags&CellFlags::Revealed)!=CellFlags::Revealed) {
            if(!dontMap) {
                if(last)RadarClass::Instance.RevealFogShroud(&at,house,false);
                else cell->Unshroud();
            }
        }
    }
    if constexpr(!Paired)MapClass::Instance.RevealArea3(&projected,0,radius+3,false);
}
}
void MapClass::RevealArea1(CoordStruct* position,int radius,HouseClass* house,
                         BYTE incremental,BYTE dontMap,BYTE unfog,BYTE byHeight,BYTE last) {
    reveal<false>(*this,position,radius,house,incremental,dontMap,unfog,byHeight,last);
}
void MapClass::RevealArea2(CoordStruct* position,int radius,HouseClass* house,
                         BYTE incremental,int,BYTE unfog,BYTE byHeight,BYTE last) {
    reveal<true>(*this,position,radius,house,incremental,0,unfog,byHeight,last);
}
