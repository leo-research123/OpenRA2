// Fixed YR 586360 / 5864A0 / 5865E0. Height projects into shroud cells;
// odd half-levels consult both diagonal neighbours. No host visibility state.
#include "yrpp/MapClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/TacticalClass.h"
#include <algorithm>
#include <bit>
#include <cstdlib>

namespace {
CellClass* projected_cell(const MapClass& map,const CoordStruct& world,int& level) {
    level=world.Z/Unsorted::LevelHeight;
    const int shift=level/2+(level&1);
    return map.GetCellAt(CellStruct{
        std::bit_cast<short>(static_cast<unsigned short>(world.X/256-shift)),
        std::bit_cast<short>(static_cast<unsigned short>(world.Y/256-shift))});
}
}
bool MapClass::IsLocationShrouded(const CoordStruct& world) const {
    int level; auto* cell=projected_cell(*this,world,level);
    if (level&1) {
        if ((cell->AltFlags&AltCellFlags::Mapped)!=AltCellFlags{}) return false;
        cell=cell->GetNeighbourCell(static_cast<FacingType>(3));
    }
    return (cell->AltFlags&AltCellFlags::Mapped)==AltCellFlags{};
}
bool MapClass::IsLocationGapped(const CoordStruct& world) const {
    int level; auto* cell=projected_cell(*this,world,level);
    if (level&1) {
        if (std::bit_cast<int>(cell->unknown_13C)<=0) return false;
        cell=cell->GetNeighbourCell(static_cast<FacingType>(3));
    }
    return std::bit_cast<int>(cell->unknown_13C)>0;
}
// This exact target's 5865E0 is xor al,al; ret 4. Gap dimming is 5864A0,
// not this disabled fog query, despite the old header's overlapping names.
bool MapClass::IsLocationFogged(const CoordStruct&) { return false; }

// YR 0x567DA0: recompute shroud-edge frames in the original radius scan order.
// Uses CellSpread's calibrated OpenTS RadiusOffset/RadiusCount table, not a
// square or Euclidean disk. This updates visibility classification, not sight.
void MapClass::RevealArea3(CoordStruct* coords,int startRadius,int radius,bool skipReveal) {
    radius=std::clamp(radius,3,11);startRadius=std::clamp(startRadius,0,8);
    const auto first=startRadius?CellSpread::NumCells(static_cast<unsigned>(startRadius-1)):0;
    const auto end=CellSpread::NumCells(static_cast<unsigned>(radius));
    if(skipReveal)return;
    if(first>end || !TacticalClass::Instance)std::abort();
    const auto project=[](int coordinate,int height) {
        const int delta=TacticalClass::AdjustForZ(height)/-30;
        return std::bit_cast<int>(static_cast<unsigned>(coordinate)+static_cast<unsigned>(delta)*256u)/256;
    };
    const int x=project(coords->X,coords->Z),y=project(coords->Y,coords->Z);
    for(auto i=first;i<end;++i) {
        const auto offset=CellSpread::GetCell(i);
        const CellStruct at{short(x+offset.X),short(y+offset.Y)};
        auto* cell=GetCellAt(at);
        const char visibility=TacticalClass::Instance->GetOcclusion(at,false);
        if(visibility!=cell->Visibility) {
            cell->Visibility=visibility;cell->VisibilityChanged=true;
            TacticalClass::Instance->RegisterCellAsVisible(cell);
        }
    }
}
