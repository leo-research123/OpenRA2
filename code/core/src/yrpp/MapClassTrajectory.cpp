// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp Firestorm_On_Path; YR 0x5880A0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "RulesClassReaders.hpp"
#include <bit>

// OpenTS coord.cpp Coord_Scatter; YR uses the low random byte, signed
// direction angle and map-array bounds rather than the playable diamond.
CoordStruct* YRPP_FASTCALL MapClass::GetRandomCoordsNear(CoordStruct& out,const CoordStruct& from,int distance,bool center) {
    const auto direction=static_cast<unsigned char>(ScenarioClass::Instance->Random.Random());
    const auto facing=std::bit_cast<short>(static_cast<unsigned short>(unsigned(direction)<<8));
    const double angle=(int(facing)-0x3FFF)*-0.00009587672516830327;
    const int y=rule_integer(double(from.Y)-Math::sin(angle)*distance);
    const int x=rule_integer(Math::cos(angle)*distance+double(from.X));
    CoordStruct result{x,y,from.Z};
    if(unsigned(x/256)>=512u || unsigned(y/256)>=512u)result=from;
    if(center){result.X=result.X-static_cast<unsigned char>(result.X)+128;result.Y=result.Y-static_cast<unsigned char>(result.Y)+128;}
    out=result;return &out;
}

CoordStruct* MapClass::FindFirstFirestorm(CoordStruct* out,const CoordStruct& from,
        const CoordStruct& to,const HouseClass* owner) const {
    const auto blocks=[&](const CellStruct& cell) {
        auto* building=GetCellAt(cell)->GetBuilding();
        return building && building->Type->FirestormWall && building->Owner->FirestormActive && building->Owner!=owner;
    };
    if(from!=to) {
        auto cell=CellClass::Coord2Cell(from);
        const auto target=CellClass::Coord2Cell(to);
        if(cell.X==target.X) {
            // Original compares the sign-extended source with zero-extended
            // destination Y here (unlike the X-only arm).
            const int step=int(cell.Y)<static_cast<unsigned short>(target.Y)?1:-1;
            while(cell!=target) {
                if(blocks(cell)){*out=CellClass::Cell2Coord(cell);return out;}
                cell.Y=static_cast<short>(cell.Y+step);
            }
        }else if(cell.Y==target.Y) {
            const int step=cell.X<target.X?1:-1;
            while(cell!=target) {
                if(blocks(cell)){*out=CellClass::Cell2Coord(cell);return out;}
                cell.X=static_cast<short>(cell.X+step);
            }
        }else {
            const int dx=from.X<to.X?256:-256,dy=from.Y<to.Y?256:-256;
            int x=from.X-from.X%256+(dx>0?256:0),y=from.Y-from.Y%256+(dy>0?256:0);
            const double spanX=double(to.X)-from.X,spanY=double(to.Y)-from.Y;
            double progressX=(double(x)-from.X)/spanX,progressY=(double(y)-from.Y)/spanY;
            const double inverseY=1.0/spanY;
            while((progressX<=1.0 || progressY<=1.0) && progressX>=0.0 && progressY>=0.0) {
                if(blocks({short(x/256),short(y/256)})){*out={x,y,0};return out;}
                if(progressX<progressY){x+=dx;progressX=(double(x)-from.X)*(1.0/spanX);}
                else {y+=dy;progressY=(double(y)-from.Y)*inverseY;}
            }
        }
    }
    *out=CoordStruct::Empty;return out;
}
