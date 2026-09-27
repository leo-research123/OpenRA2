// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 code/anim.cpp:
// Flaming_Guy_AI, Next_Flaming_Guy_Coord, Is_Valid_Flaming_Guy_Cell.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR 0x00425670 / 0x00425D10 / 0x004260F0; YR also expires on beach.
#include "yrpp/AnimClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "RulesClassReaders.hpp"

namespace {
bool bridge_at(const CellStruct& cell) {
    auto* at=MapClass::Instance.TryGetCellAt(cell);
    return at && at->ContainsBridgeEx();
}
bool bridge_near(const CellStruct& cell) {
    return bridge_at(cell) || bridge_at({short(cell.X-1),cell.Y}) || bridge_at({cell.X,short(cell.Y-1)});
}
unsigned short direction(const CoordStruct& from,const CoordStruct& to) {
    return static_cast<unsigned short>(rule_integer(
        (Math::atan2(double(from.Y)-to.Y,double(to.X)-from.X)-1.5707963267948966)*-10430.060040584269));
}
}

bool AnimClass::IsValidFlamingGuyCell(const CellStruct& cell) noexcept {
    auto* at=MapClass::Instance.TryGetCellAt(cell);
    if(!at)return false;
    const int floor=int(static_cast<signed char>(at->Level))*Unsorted::LevelHeight;
    if(at->ContainsBridgeEx() && Location.Z-floor>2*Unsorted::LevelHeight)
        return (at->AltOccupationFlags&0xE0u)==0;
    if(at->LandType==LandType::Rock || at->LandType==LandType::Tunnel || at->LandType==LandType::Wall)return false;
    if(at->OverlayTypeIndex!=-1 && OverlayTypeClass::Array[at->OverlayTypeIndex]->Wall)return false;
    return (at->OccupationFlags&0xE0u)==0 && floor-Location.Z<=2*Unsorted::LevelHeight;
}

CoordStruct* AnimClass::NextFlamingGuyCoords(CoordStruct* result) noexcept {
    auto& map=MapClass::Instance;
    const auto cell=CellClass::Coord2Cell(Location);
    CellStruct nearest=CellStruct::Empty;int best=10000;
    // Preserve the original asymmetric scan bounds and southeast distance bias.
    for(int x=cell.X-5;x<cell.X+5;++x)for(int y=cell.Y-5;y<cell.Y+5;++y) {
        const CellStruct candidate{short(x),short(y)};
        if(!map.IsWithinUsableArea(candidate,true))continue;
        auto* at=map.TryGetCellAt(candidate);
        if(!at || at->LandType!=LandType::Water || bridge_near(candidate))continue;
        const double dx=x-cell.X,dy=y-cell.Y;
        int distance=rule_integer(Math::sqrt(dx*dx+dy*dy));
        if(x>cell.X || y>cell.Y)distance-=3;
        if(distance<best){best=distance;nearest=candidate;}
    }
    const auto select=[&](const CellStruct& target) {
        if(!IsValidFlamingGuyCell(target))return false;
        *result={int(target.X)*256+128,int(target.Y)*256+128,0};return true;
    };
    if(nearest!=CellStruct::Empty) {
        const int dx=(nearest.X>cell.X)-(nearest.X<cell.X),dy=(nearest.Y>cell.Y)-(nearest.Y<cell.Y);
        if(select({short(cell.X+dx),short(cell.Y+dy)}) || select({short(cell.X+dx),cell.Y})
            || select({cell.X,short(cell.Y+dy)}))return result;
    }
    // The EXE tests the global CELL_NONE against (0,0), not the scan result.
    if(CellStruct::Empty==CellStruct{0,0} && ScenarioClass::Instance) {
        const int first=ScenarioClass::Instance->Random.RandomRanged(0,7);
        for(int i=0;i<8;++i) {
            const auto delta=Unsorted::AdjacentCell[(first+i)&7];
            if(select({short(cell.X+delta.X),short(cell.Y+delta.Y)}))return result;
        }
    }
    *result=CoordStruct::Empty;return result;
}

void AnimClass::FlamingGuyAI() noexcept {
    // Invalid custom art cannot supply the divisor required by the original.
    if(Type->RunningFrames<=0){TimeToDie=true;return;}
    if(!FlamingGuyExpire)Animation.Start(0);
    const auto finish=[&] {
        FlamingGuyCoords=CoordStruct::Empty;FlamingGuyExpire=true;
        Animation.Start(1);Animation.Value=Type->RunningFrames*8+1;
    };
    auto& map=MapClass::Instance;
    const auto move=[&](CoordStruct at) {
        const int floor=map.GetCellFloorHeight(at);
        at.Z=floor+(Location.Z>floor+2*Unsorted::LevelHeight?CellClass::BridgeHeight:0);
        SetLocation(at);
    };
    if(FlamingGuyCoords!=CoordStruct::Empty && !FlamingGuyExpire) {
        const double dx=double(Location.X)-FlamingGuyCoords.X,dy=double(Location.Y)-FlamingGuyCoords.Y;
        if(rule_integer(Math::sqrt(dx*dx+dy*dy))<=18) {
            if(!IsFallingDown) {
                CoordStruct next;NextFlamingGuyCoords(&next);
                const auto* at=map.TryGetCellAt(FlamingGuyCoords);
                const bool sink=at && (at->LandType==LandType::Water || at->LandType==LandType::Beach)
                    && GetHeight()<=Unsorted::LevelHeight;
                if(next==CoordStruct::Empty || FlamingGuyRetries>=7 || sink){finish();return;}
                ++FlamingGuyRetries;move(FlamingGuyCoords);FlamingGuyCoords=next;
            }
        }else {
            const double angle=(int(static_cast<short>(direction(GetCoords(),FlamingGuyCoords)))-0x3FFF)*-0.00009587672516830327;
            move({rule_integer(Location.X+Math::cos(angle)*18.0),rule_integer(Location.Y-Math::sin(angle)*18.0),Location.Z});
        }
        if(!IsFallingDown && Location.Z>=map.GetCellFloorHeight(Location)+CellClass::BridgeHeight
            && !bridge_near(CellClass::Coord2Cell(Location)))IsFallingDown=true;
    }else if(!IsFallingDown && !FlamingGuyExpire) {
        CoordStruct next;NextFlamingGuyCoords(&next);
        if(next==CoordStruct::Empty)finish();else FlamingGuyCoords=next;
    }
    if(!FlamingGuyExpire) {
        unsigned facing=0;
        if(FlamingGuyCoords!=CoordStruct::Empty)facing=(7u-(((unsigned(direction(GetCoords(),FlamingGuyCoords))+0x1000u)>>13)&7u))&7u;
        Animation.Value=int(facing)*Type->RunningFrames+Unsorted::CurrentFrame/3%Type->RunningFrames;
    }else {
        auto* image=Type->GetImage();if(image)image=image->GetData();
        if(image && Animation.Value>=image->Frames/2-1)TimeToDie=true;
    }
}
