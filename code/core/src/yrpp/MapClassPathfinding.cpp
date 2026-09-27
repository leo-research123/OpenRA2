// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp zone/bridge queries, Nearby_Location and Region_Threat,
// calibrated to YR.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include <algorithm>
#include <bit>
#include <cstdlib>
namespace {
int movement_adjust[13][8]{
 {1,2,2,2,2,2,2,3},{1,1,2,2,2,2,2,3},{1,1,1,2,2,2,2,3},
 {1,1,1,1,1,1,2,3},{1,1,2,1,1,2,2,3},{1,2,2,1,1,2,2,3},
 {1,1,1,2,2,2,1,3},{1,2,2,2,2,1,2,3},{1,1,1,2,2,1,2,3},
 {1,1,1,1,1,1,1,3},{2,2,2,2,1,2,2,3},{2,2,2,1,1,2,2,3},{1,1,1,2,2,2,2,3}};
bool under_bridge(const CellClass* cell){return (static_cast<unsigned>(cell->Flags)&0x100u)!=0;}
bool intact_end(const CellClass* cell){return !under_bridge(cell) && (cell->Tile_Is_Bridge() || cell->Tile_Is_WoodBridge()) && cell->LandType!=LandType::Rock;}
CellStruct add(CellStruct a,CellStruct b){return {short(a.X+b.X),short(a.Y+b.Y)};}
short distance(CellStruct a,CellStruct b){const int x=short(a.X-b.X),y=short(a.Y-b.Y);return short(int(Math::sqrt(double(x)*x+double(y)*y)));}
int average(int a,int b){return std::bit_cast<int>(static_cast<unsigned>(a)+static_cast<unsigned>(b))>>1;}
}
int (&MapClass::MovementAdjustArray)[13][8]=movement_adjust;

// OpenTS Closest_Passable_Cell; YR 0x586FC0 scans the owning TMP's
// populated subtiles in row-major order, retaining the first equal distance.
CellStruct* MapClass::ClosestPassableCell(CellStruct* output,const CellStruct& target,const CellStruct& reference) {
    const auto* cell=GetCellAt(target);
    if(cell->IsoTileTypeIndex<0){*output=target;return output;}
    auto* type=IsometricTileTypeClass::Array[cell->IsoTileTypeIndex];
    const auto* tmp=reinterpret_cast<const TMPStruct*>(type->GetImage());
    // Original requires the corresponding tile resource to have loaded.
    if(!tmp || !type->unk_2E4)std::abort();
    const int width=static_cast<int>(type->unk_2E4),height=static_cast<int>(type->unk_2E8);
    // Existing YRpp name Height is the unsigned subtile index at 0x11A.
    const int subTile=static_cast<unsigned char>(cell->Height);
    const CellStruct origin{short(target.X-subTile%width),short(target.Y-subTile/width)};
    *output=CellStruct::Empty;double best=10000.0;
    for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
        const TMPImage* image=nullptr;
        if(!tmp->GetSubTile(x+y*width,image) || !image)continue;
        const CellStruct candidate{short(origin.X+x),short(origin.Y+y)};
        if(!IsWithinUsableArea(candidate,true))continue;
        const int dx=candidate.X-reference.X,dy=candidate.Y-reference.Y;
        const double distance=Math::sqrt(double(dx*dx+dy*dy));
        if(distance<best){best=distance;*output=candidate;}
    }
    return output;
}

// OpenTS Nearby_Location / Is_Clear_To_Move; YR 0x56DC20 / 0x56E7C0.
// Keep ring order, duplicate radius-zero entries and the 24-candidate limit:
// they affect both Frame-based selection and equal-distance ties.
CellStruct* MapClass::NearByLocation(CellStruct& output,const CellStruct& position,
    SpeedType speed,int zone,MovementZone movement,bool checkBridge,
    int width,int height,bool disallowOverlay,bool checkHeight,
    bool requireBurrowable,bool allowBridge,const CellStruct& closeTo,
    bool southeastOnly,bool buildable) {
    const CellStruct origin=position;
    if(zone==0xFFFF)zone=-1;
    const auto* start=GetCellAt(origin);
    const int level=static_cast<signed char>(start->Level)+(checkBridge && under_bridge(start)?4:0);
    const int maxRadius=std::min(MapRect.Width+MapRect.Height,32);
    CellStruct candidates[24],visible[24],hidden[24];
    int count=0,visibleCount=0,hiddenCount=0;
    bool found=false;
    const auto uncovered=[](CellStruct cell){
        const CoordStruct at{cell.X*256+128,cell.Y*256+128,0};
        CellStruct adjusted;
        return *TacticalClass::AdjustCellForHeight(&adjusted,&at)==cell;
    };
    const auto consider=[&](CellStruct cell){
        auto* ground=GetCellAt(cell);
        if(!IsWithinUsableArea(ground,true))return;
        // 0x56E7C0 tests the whole requested rectangle, not just its anchor.
        for(int x=0;x<width;++x)for(int y=0;y<height;++y){
            auto* part=GetCellAt(CellStruct{short(cell.X+x),short(cell.Y+y)});
            if((disallowOverlay && part->OverlayTypeIndex!=-1)
                || !part->IsClearToMove(speed,false,false,zone,movement,-1,checkBridge))return;
        }
        if(checkHeight && std::abs(level-(under_bridge(ground)?4:0)-static_cast<signed char>(ground->Level))>=2)return;
        if(requireBurrowable && !ground->CanBurrowHere())return;
        if(!allowBridge && under_bridge(ground))return;
        RectangleStruct area{cell.X,cell.Y,width,height};
        if(buildable && !IsAreaFree(&area,-1))return;
        candidates[count++]=cell;
        if(checkBridge || uncovered(cell))found=true;
    };
    for(int radius=0;radius<maxRadius && count<24 && !found;++radius){
        for(int x=-radius;x<=radius && count<24;++x){
            if(!southeastOnly)consider({short(origin.X+x),short(origin.Y-radius)});
            if(count==24)break;
            if(!southeastOnly || x>-radius)consider({short(origin.X+x),short(origin.Y+radius)});
        }
        for(int y=1-radius;y<radius && count<24;++y){
            if(!southeastOnly)consider({short(origin.X-radius),short(origin.Y+y)});
            if(count==24)break;
            consider({short(origin.X+radius),short(origin.Y+y)});
        }
    }
    for(int i=0;i<count;++i){
        if(uncovered(candidates[i]))visible[visibleCount++]=candidates[i];
        else hidden[hiddenCount++]=candidates[i];
    }
    const auto* choices=visibleCount?visible:hidden;
    const int total=visibleCount?visibleCount:hiddenCount;
    // 0x5618B0 initializes target CELL_NONE (0xABD480) to {0,0} at startup.
    CellStruct result=CellStruct::Empty;
    if(total && closeTo==CellStruct::Empty)result=choices[Unsorted::CurrentFrame%total];
    else {
        double nearest=100000.0;
        for(int i=0;i<total;++i){
            const int x=choices[i].X-closeTo.X,y=choices[i].Y-closeTo.Y;
            const auto squared=std::bit_cast<int>(static_cast<unsigned>(x)*static_cast<unsigned>(x)
                +static_cast<unsigned>(y)*static_cast<unsigned>(y));
            const double distance=Math::sqrt(double(squared));
            if(distance<nearest){nearest=distance;result=choices[i];}
        }
    }
    output=result;return &output;
}

// OpenTS Is_Area_Available; YR 0x586780 additionally rejects terrain,
// overlays, non-clear passability, ramps and buildings, even for house -1.
bool MapClass::IsAreaFree(RectangleStruct* rect,int houseID) {
    const unsigned mask=houseID==-1?0u:1u<<(static_cast<unsigned>(houseID)&31u);
    for(int x=rect->X;x<rect->X+rect->Width;++x)for(int y=rect->Y;y<rect->Y+rect->Height;++y){
        auto* cell=GetCellAt(CellStruct{short(x),short(y)});
        // Original typed lookups scan only FirstObject while GameActive;
        // the resource helpers' broader native foundation fallback is not used.
        if(Game::IsActive)for(auto* object=cell->FirstObject;object;object=object->NextObject)
            if(object->WhatAmI()==AbstractType::Terrain || object->WhatAmI()==AbstractType::Building)return false;
        if((cell->BaseSpacerOfHouses&mask) || cell->OverlayTypeIndex!=-1
            || static_cast<int>(cell->Passability)!=0 || cell->SlopeIndex)return false;
    }
    return InLocalRadar(rect,true);
}

// OpenTS In_Local_Radar(Rect); YR 0x578390 keeps four-corner short-circuit order.
bool MapClass::InLocalRadar(RectangleStruct* rect,bool checkLevel) {
    return IsWithinUsableArea(CellStruct{short(rect->X),short(rect->Y)},checkLevel)
        && IsWithinUsableArea(CellStruct{short(rect->X+rect->Width-1),short(rect->Y)},checkLevel)
        && IsWithinUsableArea(CellStruct{short(rect->X),short(rect->Y+rect->Height-1)},checkLevel)
        && IsWithinUsableArea(CellStruct{short(rect->X+rect->Width-1),short(rect->Y+rect->Height-1)},checkLevel);
}

bool YRPP_STDCALL MapClass::IsSameCellZone(const CellStruct& from,const CellStruct& to,
    MovementZone movement,bool fromBridge,bool toBridge,bool allowLeavingMap) {
    if(movement==MovementZone::None)return true;
    auto& map=Instance;
    const auto in_map=[&](CellStruct c){return c.X+c.Y>map.MapRect.Width && c.X-c.Y<map.MapRect.Width
        && c.Y-c.X<map.MapRect.Width && c.X+c.Y<=map.MapRect.Width+2*map.MapRect.Height;};
    const bool from_usable=map.IsWithinUsableArea(from,true);
    if(!from_usable && in_map(from))return true;
    const bool to_usable=map.IsWithinUsableArea(to,true);
    if(allowLeavingMap && from_usable && !to_usable && in_map(to))return true;
    return map.GetMovementZoneType(from,movement,fromBridge)==map.GetMovementZoneType(to,movement,toBridge);
}

int MapClass::GetCellZoneIndex(const CellStruct& cell) const {
    const int index=cell.X+cell.Y*(MapRect.Width+MapRect.Height+1);
    return index<0?0:index>=ValidMapCellCount?ValidMapCellCount-1:index;
}
int MapClass::ZoneConnectionIndex(const CellStruct& cell,int maximum,int start) const {
    for(int i=start;i<ZoneConnections.Count;++i){const auto& c=ZoneConnections[i];if(c.ConnectionType)continue;
        const auto from=c.FromMapCoords,to=c.ToMapCoords;
        if(from.X==to.X){if(cell.Y>=from.Y && cell.Y<=to.Y && std::abs(cell.X-from.X)<=maximum)return i;}
        else if(cell.X>=from.X && cell.X<=to.X && std::abs(cell.Y-from.Y)<=maximum)return i;
    }
    return -1;
}
int MapClass::GetMovementZoneType(const CellStruct& cell,MovementZone movement,bool bridge) {
    CellStruct at=cell;
    if(bridge && under_bridge(GetCellAt(cell))){
        const int index=ZoneConnectionIndex(cell,1,0);if(index==-1)return -1;
        const auto& connection=ZoneConnections[index];at=connection.FromMapCoords;
        if(!connection.IsPassable){auto* edge=GetCellAt(cell);
            const auto facing=connection.FromMapCoords.X==connection.ToMapCoords.X?FacingType::South:FacingType::East;
            while(under_bridge(edge))edge=edge->GetNeighbourCell(facing);
            if(intact_end(edge))at=connection.ToMapCoords;
        }
    }
    const auto* zones=static_cast<const unsigned short*>(MovementZones[static_cast<int>(movement)]);
    if(!zones)std::abort(); // Zone lifecycle must be initialized, not silently treated as one region.
    return zones[LevelAndPassability[GetCellZoneIndex(at)].ZoneArrayIndex];
}
CellStruct* YRPP_STDCALL MapClass::FindBridgeSpanEndCell(CellStruct* output,const CellStruct& cell,const CellStruct& reference) {
    auto& map=Instance;auto* forward=map.GetCellAt(cell);auto* backward=forward;
    if(!under_bridge(forward)){*output=forward->MapCoords;return output;}
    const auto direction=(static_cast<unsigned>(forward->Flags)&0x800u)?FacingType::East:FacingType::South;
    const auto reverse=static_cast<FacingType>((static_cast<unsigned>(direction)-4u)&7u);
    CellStruct from{0,0},to{0,0};
    while(under_bridge(forward) || under_bridge(backward)) {
        if(under_bridge(forward)){forward=forward->GetNeighbourCell(direction);if(intact_end(forward))from=forward->MapCoords;}
        if(under_bridge(backward)){backward=backward->GetNeighbourCell(reverse);if(intact_end(backward))to=backward->MapCoords;}
    }
    *output=map.IsWithinUsableArea(from,true)?from:CellStruct{0,0};
    if(map.IsWithinUsableArea(to,true) && (*output==CellStruct{0,0} || distance(to,reference)<distance(from,reference)))*output=to;
    return output;
}
CellStruct* YRPP_STDCALL MapClass::GetBridgeZoneConnectionCell(CellStruct* output,CellClass* cell,bool bridge) {
    auto& map=Instance;
    if(!bridge || !under_bridge(cell)){*output=cell->MapCoords;return output;}
    const int index=map.ZoneConnectionIndex(cell->MapCoords,2,0);
    if(index==-1){FindBridgeSpanEndCell(output,cell->MapCoords,cell->MapCoords);if(*output!=CellStruct{0,0})return output;
        std::abort(); // Original would dereference connection -1: malformed/unbuilt bridge graph.
    }
    const auto& c=map.ZoneConnections[index];
    const CellStruct offset=(static_cast<unsigned>(cell->Flags)&0x800u)?CellStruct{0,short(cell->MapCoords.Y-c.FromMapCoords.Y)}:CellStruct{short(cell->MapCoords.X-c.FromMapCoords.X),0};
    const auto from=add(c.FromMapCoords,offset),to=add(c.ToMapCoords,offset);
    if(c.IsPassable)*output=distance(from,cell->MapCoords)<distance(to,cell->MapCoords)?from:to;
    else {const auto direction=c.FromMapCoords.X==c.ToMapCoords.X?FacingType::South:FacingType::East;
        while(under_bridge(cell))cell=cell->GetNeighbourCell(direction);
        *output=intact_end(cell)?to:from;
    }
    return output;
}
CellStruct* MapClass::FindBridgeEndCellForSubzone(CellStruct* output,const CellStruct& cell,int level,int subzone) {
    auto* forward=GetCellAt(cell);auto* backward=forward;
    if(!under_bridge(forward)){*output=forward->MapCoords;return output;}
    const int direction=(static_cast<unsigned>(forward->Flags)&0x800u)?2:4;
    CellStruct from{0,0},to{0,0};
    while(under_bridge(forward) || under_bridge(backward)) {
        if(under_bridge(forward)){forward=forward->GetNeighbourCell(static_cast<FacingType>(direction));if(intact_end(forward))from=forward->MapCoords;}
        if(under_bridge(backward)){backward=backward->GetNeighbourCell(static_cast<FacingType>((direction-4)&7));if(intact_end(backward))to=backward->MapCoords;}
    }
    const CellStruct side=CellSpread::GetNeighbourOffset((direction+2)&7),other=CellSpread::GetNeighbourOffset((direction-2)&7);
    const CellStruct candidates[]{from,add(from,side),add(from,other),to,add(to,side),add(to,other)};
    for(const auto& candidate:candidates)
        if(IsWithinUsableArea(candidate,true) && LevelAndPassabilityStruct2pointer_70[GetCellZoneIndex(candidate)].SubzoneIDs[level]==subzone){*output=candidate;return output;}
    *output={0,0};return output;
}
int YRPP_STDCALL MapClass::RegionThreat(HouseClass* house,int level,int fromSubzone,int toSubzone) {
    auto& map=Instance;
    const auto raw_threat=[&](int region){return std::bit_cast<int>(house->ThreatPosedEstimates[region/130][region%130]);};
    if(level==1)return raw_threat(static_cast<int>(map.SubzoneTracking[1][toSubzone].ThreatRegion));
    if(level!=2)return 0;
    const auto region_cell=[](unsigned region){const int value=std::bit_cast<int>(region),column=(value-1)%130;
        return CellStruct{short(4*column),short(4*((value-column)/130-1))};};
    const auto from=region_cell(map.SubzoneTracking[2][fromSubzone].ThreatRegion),to=region_cell(map.SubzoneTracking[2][toSubzone].ThreatRegion);
    const CellStruct from_block{short(from.X-(from.X/4%2!=0)),short(from.Y-(from.Y/4%2!=0))};
    const CellStruct to_block{short(to.X-(to.X/4%2!=0)),short(to.Y-(to.Y/4%2!=0))};
    const auto cell_threat=[&](CellStruct cell){return raw_threat(cell.X/4+1+130*(cell.Y/4+1));};
    if(from_block==to_block)return average(cell_threat(from),cell_threat(to));
    // 0x5860F0 JGE handles equal X separately; preserve the cardinal branch
    // even though the decompiler can merge away that flag-dependent edge.
    const int direction=to_block.X<from_block.X?(to_block.Y<=from_block.Y?(to_block.Y<from_block.Y?7:6):5)
        :to_block.X>from_block.X?(to_block.Y<=from_block.Y?(to_block.Y>=from_block.Y?2:1):3)
        :to_block.Y>from_block.Y?4:0;
    constexpr CellStruct offsets[]{{0,0},{4,0},{0,4},{4,4}};
    constexpr int from_quadrants[8][2]{{0,1},{1,1},{1,3},{3,3},{2,3},{2,2},{0,2},{0,0}};
    constexpr int to_quadrants[8][2]{{2,3},{2,2},{0,2},{0,0},{0,1},{1,1},{1,3},{3,3}};
    const int a=std::min(cell_threat(add(from_block,offsets[from_quadrants[direction][0]])),cell_threat(add(from_block,offsets[from_quadrants[direction][1]])));
    const int b=std::min(cell_threat(add(to_block,offsets[to_quadrants[direction][0]])),cell_threat(add(to_block,offsets[to_quadrants[direction][1]])));
    return average(a,b);
}

bool MapClass::BuildReachableSubzones(CellClass* start,int level,DynamicVectorClass<unsigned short>& unreachable,const FootClass* foot) {
    const int dimension=1<<(level+1),mask=dimension-1;
    CellClass* stack[64]{};bool visited[8][8]{};DynamicVectorClass<unsigned short> reached;
    const auto subzone_at=[&](const CellStruct& cell){return static_cast<unsigned short>(LevelAndPassabilityStruct2pointer_70[GetCellZoneIndex(cell)].SubzoneIDs[level]);};
    const auto start_subzone=subzone_at(start->MapCoords);unsigned short previous=0;
    stack[0]=start;int count=1;visited[start->MapCoords.X&mask][start->MapCoords.Y&mask]=true;
    const auto* passability=MovementAdjustArray[static_cast<int>(foot->GetTechnoType()->MovementZone)];
    while(count>0) {
        auto* current=stack[--count];
        for(int direction=0;direction<8;++direction) {
            auto* next=current->GetNeighbourCell(static_cast<FacingType>(direction));
            const auto subzone=subzone_at(next->MapCoords);const int x=next->MapCoords.X&mask,y=next->MapCoords.Y&mask;
            if(foot->IsCellOccupied(next,static_cast<FacingType>(direction),static_cast<signed char>(next->Level),nullptr,true)==Move::OK
                || passability[static_cast<int>(next->Passability)]!=1) {
                if(subzone==start_subzone){if(!visited[x][y]){visited[x][y]=true;if(count>=64)std::abort();stack[count++]=next;}}
                else if(subzone && subzone!=previous){previous=subzone;if(reached.FindItemIndex(subzone)==-1)reached.AddItem(subzone);}
            }
        }
    }
    // The target uses this same coordinate for every visited-grid element;
    // row/column are NOT added here (also preserved by fixed OpenTS).
    const CellStruct probe{short(start->MapCoords.X+(start->MapCoords.X&mask)),short(start->MapCoords.Y+(start->MapCoords.Y&mask))};
    if(IsWithinUsableArea(probe,true) && subzone_at(probe)==start_subzone)
        for(int row=0;row<dimension;++row)for(int col=0;col<dimension;++col)if(!visited[row][col])return true;
    const auto& connections=SubzoneTracking[level][start_subzone].SubzoneConnections;
    for(int i=connections.Count-1;i>=0;--i){const auto subzone=static_cast<unsigned short>(connections[i].SubzoneID);
        if(reached.FindItemIndex(subzone)==-1)unreachable.AddItem(subzone);}
    return false;
}
