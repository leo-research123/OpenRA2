// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp Zone_Reset/Zone_Span and subzone graph construction.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md. Calibrated to YR gamemd.exe.
#include "yrpp/MapClass.h"
#include "yrpp/AStarClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/TubeClass.h"
#include "map_hash.hpp"
#include <algorithm>
#include <cstdlib>
#include <vector>

namespace {
int last_adjacent_zone;
DWORD YRPP_FASTCALL zone_hash(const DWORD& key){return (key&0xFu)|((key>>12)&0xF0u);}
template<class Value> void prepare(HashTable<DWORD,Value>*& hash) {
    if(!hash)hash=game::create_map_hash<DWORD,Value>(&zone_hash,16);
    if(!hash)std::abort();
    for(int i=0;i<hash->BucketCount;++i)hash->Buckets[i].Clear();
}
template<class Value> void stage(HashTable<DWORD,Value>* hash,DWORD key,const Value& value) {
    if(!hash)std::abort();
    auto& bucket=hash->Buckets[zone_hash(key)];
    for(int i=0;i<bucket.Count;++i)if(bucket[i].Key==key)return; // First staged value wins, including its cross-block flag.
    try{if(!bucket.AddItem({key,value}))std::abort();}catch(...){std::abort();}
}
void stage_subzone(HashTable<DWORD,SubzoneConnectionStruct>* hash,int from,int to,bool cross) {
    const DWORD key=(static_cast<DWORD>(from)<<16)|static_cast<unsigned short>(to);
    stage(hash,key,SubzoneConnectionStruct{key,static_cast<BYTE>(cross)});
}
int height(char value){return static_cast<unsigned char>(value);}
CellStruct adjacent(CellStruct cell,int facing){const auto& delta=CellSpread::GetNeighbourOffset(facing&7);return {short(cell.X+delta.X),short(cell.Y+delta.Y)};}

void update_cell_zone(MapClass& map,const CellStruct& cell,bool removed){
 auto& changed=map.LevelAndPassability[map.GetCellZoneIndex(cell)];
 if(changed.CellPassability==7)return;
 const int stride=map.MapRect.Width+map.MapRect.Height+1;
 const int offsets[]{-stride,1-stride,1,stride+1,stride,stride-1,-1,-stride-1};
 const auto index=&changed-map.LevelAndPassability;
 CellLevelPassabilityStruct outside{};outside.CellPassability=7;
 const auto neighbour=[&](int offset)->CellLevelPassabilityStruct&{
  const auto at=index+offset;return at>=0&&at<map.ValidMapCellCount?map.LevelAndPassability[at]:outside;
 };
 CellLevelPassabilityStruct* baseline=nullptr;
 for(int offset:offsets){auto& other=neighbour(offset);if(other.CellPassability==(removed?0:changed.CellPassability)){baseline=&other;break;}}
 if(baseline&&map.MovementZones[0]){
  const auto* movement=static_cast<const unsigned short*>(map.MovementZones[0]);
  int transitions=0,previous=0;
  for(int offset:offsets){auto& other=neighbour(offset);
   if(movement[other.ZoneArrayIndex]!=movement[previous]&&other.CellPassability!=7){previous=other.ZoneArrayIndex;++transitions;}
  }
  if(transitions<=3&&(!removed||changed.CellPassability==0)){changed.ZoneArrayIndex=baseline->ZoneArrayIndex;return;}
 }
 map.ResetAllZones();
}
}

// OpenTS Update_Cell_Zone / Update_Cell_Zone_Constructively;
// original YR 0x0056D460 / 0x0056D5A0.
void MapClass::ResetZones(const CellStruct& cell){update_cell_zone(*this,cell,true);}
void MapClass::RecalculateZones(const CellStruct& cell){update_cell_zone(*this,cell,false);}

int MapClass::ZoneSpan(CellLevelPassabilityStruct* seed,int zone,int& skip) {
    const auto pass=seed->CellPassability;const bool impassable=pass==6;
    auto* begin=seed;auto* end=seed;int previous_height=height(seed->CellLevel);
    const auto boundary=[&](const CellLevelPassabilityStruct* cell,int neighbour_height){
        const int other=cell->ZoneArrayIndex;
        if(other && other!=zone && other!=last_adjacent_zone && (std::abs(height(cell->CellLevel)-neighbour_height)<2 || impassable)) {
            const DWORD key=(static_cast<DWORD>(other)<<16)|static_cast<unsigned short>(zone);
            stage(unknown_pointer_14,key,key);last_adjacent_zone=other;
        }
    };
    while(begin->CellPassability==pass && std::abs(height(begin->CellLevel)-previous_height)<2){begin->ZoneArrayIndex=static_cast<unsigned short>(zone);previous_height=height(begin->CellLevel);--begin;}
    boundary(begin,previous_height);
    // YR retains the left scan's final height and permits a rightward step < 4.
    while(end->CellPassability==pass && std::abs(height(end->CellLevel)-previous_height)<4){end->ZoneArrayIndex=static_cast<unsigned short>(zone);previous_height=height(end->CellLevel);++end;}
    boundary(end,previous_height);
    int filled=static_cast<int>(end-begin-1);skip=static_cast<int>(end-seed-1);++begin;--end;
    const int stride=MapRect.Width+MapRect.Height+1;
    for(int side:{-1,1}) {
        auto* shadow=begin+side*stride-1;auto* last=end+side*stride+1;
        while(shadow<=last) {
            auto* neighbour=shadow-side*stride+(shadow<last-1?1:shadow==last-1?0:-1);
            if(!shadow->ZoneArrayIndex && shadow->CellPassability==pass && std::abs(height(shadow->CellLevel)-height(neighbour->CellLevel))<2){int advance=0;filled+=ZoneSpan(shadow,zone,advance);shadow+=advance;}
            else{if(shadow->ZoneArrayIndex)boundary(shadow,height(neighbour->CellLevel));++shadow;}
        }
    }
    return filled;
}

int MapClass::ResetAllZones() {
    try {
        prepare(unknown_pointer_14);
        for(auto& zones:MovementZones){YRMemory::Deallocate(zones);zones=nullptr;}
        for(int i=0;i<ValidMapCellCount;++i)LevelAndPassability[i].ZoneArrayIndex=0;
        std::vector<unsigned char> passability{7};int best=-1,best_span=-1,zone=1;
        for(int i=0;i<ValidMapCellCount;) {
            auto* cell=LevelAndPassability+i;
            if(cell->CellPassability==7 || cell->ZoneArrayIndex){++i;continue;}
            if(zone>=0x10000)std::abort();
            const auto pass=static_cast<unsigned char>(cell->CellPassability);int skip=0;last_adjacent_zone=0;
            const int span=ZoneSpan(cell,zone,skip);if(span>best_span){best=zone;best_span=span;}
            passability.push_back(pass);++zone;i+=skip;
        }
        somecount_4C=static_cast<unsigned short>(zone);
        if(zone>=0x10000)std::abort();
        for(int i=ZoneConnections.Count-1;i>=0;--i)if(const auto& c=ZoneConnections[i];c.IsPassable){
            int from=LevelAndPassability[GetCellZoneIndex(c.FromMapCoords)].ZoneArrayIndex,to=LevelAndPassability[GetCellZoneIndex(c.ToMapCoords)].ZoneArrayIndex;
            if(from!=to){if(to<from)std::swap(from,to);const DWORD key=(static_cast<DWORD>(from)<<16)|static_cast<unsigned>(to);stage(unknown_pointer_14,key,key);}
        }
        std::vector<std::vector<unsigned short>> neighbours(zone);
        for(int b=0;b<256;++b)for(int i=0;i<unknown_pointer_14->Buckets[b].Count;++i){const auto key=unknown_pointer_14->Buckets[b][i].Value;
            neighbours[key&0xFFFFu].push_back(static_cast<unsigned short>(key>>16));neighbours[key>>16].push_back(static_cast<unsigned short>(key));}
        std::vector<unsigned short> stack(zone);
        for(int movement=0;movement<13;++movement) {
            auto* zones=static_cast<unsigned short*>(YRMemory::Allocate(sizeof(unsigned short)*zone));if(!zones)std::abort();MovementZones[movement]=zones;
            const auto* table=MovementAdjustArray[movement];for(int i=0;i<zone;++i)zones[i]=table[passability[i]]!=1;
            unsigned short next=2;
            for(int i=0;i<zone;++i)if(!zones[i]){
                int count=1;stack[0]=static_cast<unsigned short>(i);zones[i]=next;const int pass=table[passability[i]];
                while(count){const auto current=stack[--count];const auto& links=neighbours[current];
                    for(int j=static_cast<int>(links.size())-1;j>=0;--j){const int n=links[j];if(table[passability[n]]==pass && !zones[n]){stack[count++]=static_cast<unsigned short>(n);zones[n]=next;}}
                }
                ++next;
            }
            zones[0]=0xFFFF;
        }
        return best; // YR returns raw best zone ID, unlike OpenTS's movement-zone lookup.
    }catch(...){std::abort();} // Original allocation failures are fatal; no exception crosses the class boundary.
}

int MapClass::SubzoneSpan(LevelAndPassabilityStruct2* seed,int level,int subzone,const RectangleStruct& bounds,const CellStruct& cell) {
    const int zone=seed->ZoneID,y=cell.Y,xmin=bounds.X,xmax=bounds.X+bounds.Width-1;
    auto* begin=seed;auto* end=seed;int x=cell.X,end_x=x,last=-1,previous_height=height(seed->CellLevel);
    while(x>=xmin && std::abs(height(begin->CellLevel)-previous_height)<2){begin->SubzoneIDs[level]=static_cast<short>(subzone);previous_height=height(begin->CellLevel);--begin;--x;if(begin->ZoneID!=zone)break;}
    const int begin_x=x;
    const auto boundary=[&](const LevelAndPassabilityStruct2* shadow,const LevelAndPassabilityStruct2* neighbour,CellStruct shadow_cell,CellStruct span_cell,bool cross){
        const int id=shadow->SubzoneIDs[level];
        if(id && id!=subzone && id!=last && std::abs(height(shadow->CellLevel)-height(neighbour->CellLevel))<2
            && IsWithinUsableArea(span_cell,true) && IsWithinUsableArea(shadow_cell,true)) {stage_subzone(unknown_80[level],id,subzone,cross);last=id;}
    };
    // The left/right boundary test uses the last accepted cell height.
    LevelAndPassabilityStruct2 neighbour{};neighbour.CellLevel=static_cast<char>(previous_height);
    boundary(begin,&neighbour,{short(x),short(y)},{short(x+1),short(y)},false);
    previous_height=height(seed->CellLevel);
    while(end->ZoneID==zone && end_x<=xmax && std::abs(height(end->CellLevel)-previous_height)<2){end->SubzoneIDs[level]=static_cast<short>(subzone);previous_height=height(end->CellLevel);++end;++end_x;}
    neighbour.CellLevel=static_cast<char>(previous_height);
    boundary(end,&neighbour,{short(end_x),short(y)},{short(end_x-1),short(y)},false);
    const int skip=static_cast<int>(end-seed-1),stride=MapRect.Width+MapRect.Height+1;
    for(int side:{-1,1}) {
        auto* shadow=begin+side*stride;x=begin_x;
        while(x<=end_x) {
            const int dx=x==begin_x?1:x<end_x?0:-1;auto* span=shadow-side*stride+dx;
            const CellStruct shadow_cell{short(x),short(y+side)},span_cell{short(x+dx),short(y)};
            const bool outside_y=side<0?y<=bounds.Y:y>=bounds.Y+bounds.Height-1;
            if(shadow->SubzoneIDs[level] || outside_y || x<xmin || x>xmax){boundary(shadow,span,shadow_cell,span_cell,x<xmin || x>xmax);++shadow;++x;}
            else if(shadow->ZoneID==zone && std::abs(height(shadow->CellLevel)-height(span->CellLevel))<2)SubzoneSpan(shadow,level,subzone,bounds,shadow_cell);
            else{++shadow;++x;}
        }
    }
    return skip;
}

void MapClass::RegisterZoneConnectionEntries(const ZoneConnectionClass& connection,int level) {
    const auto from=connection.FromMapCoords,to=connection.ToMapCoords;auto* cell=GetCellAt(from);
    CellStruct enter1,enter2,end1,end2;
    if(cell->Tile_Is_Bridge() || cell->Tile_Is_WoodBridge()){
        constexpr int sides[]{0,0,-1,2,2,-1,0,0,0,0,0,2,2,2,2,2}; // YR 0x82A944.
        const int base=cell->Tile_Is_Bridge()?IsometricTileTypeClass::BridgeSet:IsometricTileTypeClass::WoodBridgeSet;
        const int side=sides[cell->IsoTileTypeIndex-base];
        enter1=adjacent(from,side);enter2=adjacent(from,side-4);end1=adjacent(to,side);end2=adjacent(to,side-4);
    }else{
        const auto* tunnel=cell->GetTunnel();if(!tunnel)std::abort();
        enter1=adjacent(from,tunnel->ExitFace+2);enter2=adjacent(from,tunnel->ExitFace-2);
        const auto* a=GetCellAt(enter1)->GetTunnel();const auto* b=GetCellAt(enter2)->GetTunnel();if(!a || !b)return;
        AStarClass::FollowPath(&end1,&enter1,a->FaceCount,a->Faces);AStarClass::FollowPath(&end2,&enter2,b->FaceCount,b->Faces);
    }
    const auto link=[&](CellStruct a,CellStruct b){stage_subzone(unknown_80[level],LevelAndPassabilityStruct2pointer_70[GetCellZoneIndex(a)].SubzoneIDs[level],LevelAndPassabilityStruct2pointer_70[GetCellZoneIndex(b)].SubzoneIDs[level],false);};
    link(from,to);link(enter1,end1);link(enter2,end2);
}
void MapClass::RegisterSubzoneZoneConnections(int level){for(int i=0;i<ZoneConnections.Count;++i)if(ZoneConnections[i].IsPassable)RegisterZoneConnectionEntries(ZoneConnections[i],level);}

void MapClass::ResetSubzone(int level) {
    try {
        prepare(unknown_80[level]);auto& tracking=SubzoneTracking[level];
        for(int i=0;i<ValidMapCellCount;++i){auto& sub=LevelAndPassabilityStruct2pointer_70[i];const auto& zone=LevelAndPassability[i];sub.SubzoneIDs[level]=0;sub.ZoneID=static_cast<short>(zone.ZoneArrayIndex);sub.CellLevel=zone.CellLevel;}
        if(!tracking.AddItem(SubzoneTrackingStruct{}))std::abort();tracking[0].ParentSubzoneID=0;tracking[0].Passability=7;
        int count=1,x=0,y=0;const int dimension=1<<(level+1),mask=dimension-1,stride=MapRect.Width+MapRect.Height+1;RectangleStruct bounds{0,0,dimension,dimension};
        for(int i=0;i<ValidMapCellCount;) {
            auto& sub=LevelAndPassabilityStruct2pointer_70[i];const int pass=static_cast<unsigned char>(LevelAndPassability[i].CellPassability);
            if(pass==7 || sub.SubzoneIDs[level]){++i;++x;}
            else{
                if(count>=0x8000)std::abort(); // IDs are read as signed short by the original search.
                const int parent=level<2?sub.SubzoneIDs[level+1]:0;
                const int skip=SubzoneSpan(&sub,level,count,bounds,{short(x),short(y)});
                if(!tracking.AddItem(SubzoneTrackingStruct{}))std::abort();auto& entry=tracking[count++];
                entry.ParentSubzoneID=static_cast<unsigned short>(parent);entry.Passability=pass;entry.SubzoneConnections.CapacityIncrement=16;entry.ThreatRegion=x/4+130*(y/4)+131;
                i+=skip;x+=skip;
            }
            if(x==stride){x=0;++y;bounds.X=0;bounds.Y=y-(y&mask);}else if(!(x&mask))bounds.X=x;
        }
        SubzoneTrackingCounts[level]=count;RegisterSubzoneZoneConnections(level);
        for(int b=0;b<256;++b)for(int i=0;i<unknown_80[level]->Buckets[b].Count;++i){const auto& link=unknown_80[level]->Buckets[b][i].Value;const int a=link.SubzoneID>>16,b=link.SubzoneID&0xFFFFu;
            if(!tracking[b].SubzoneConnections.AddItem({static_cast<unsigned>(a),link.IsCrossBlock}) || !tracking[a].SubzoneConnections.AddItem({static_cast<unsigned>(b),link.IsCrossBlock}))std::abort();}
    }catch(...){std::abort();}
}
void MapClass::ResetAllSubzones(){for(int level=2;level>=0;--level){SubzoneTracking[level].Clear();ResetSubzone(level);}AStarClass::Instance.Reset();}

// OpenTS Update_Cell_Subzones; YR 0x00584550. Rebuild only the affected
// 8/4/2-cell blocks, remove reciprocal old links and reset the path search.
void MapClass::RecalculateSubZones(const CellStruct& cell){
 if(!IsWithinUsableArea(cell,true))return;
 try{
  const int stride=MapRect.Width+MapRect.Height+1;
  for(int level=2;level>=0;--level){
   const int size=1<<(level+1);
   const RectangleStruct bounds{cell.X-cell.X%size,cell.Y-cell.Y%size,size,size};
   const int endX=std::min(bounds.X+size,stride),endY=std::min(bounds.Y+size,stride);
   prepare(unknown_80[level]);auto& tracking=SubzoneTracking[level];
   std::vector<int> collected;
   for(int y=bounds.Y;y<endY;++y)for(int x=bounds.X;x<endX;++x){
    const int index=GetCellZoneIndex({short(x),short(y)});auto& sub=LevelAndPassabilityStruct2pointer_70[index];
    const int id=sub.SubzoneIDs[level];
    if(id&&std::find(collected.begin(),collected.end(),id)==collected.end())collected.push_back(id);
    sub.SubzoneIDs[level]=0;sub.ZoneID=static_cast<short>(LevelAndPassability[index].ZoneArrayIndex);
   }
   for(auto it=collected.rbegin();it!=collected.rend();++it){
    auto& entry=tracking[*it];
    for(int i=entry.SubzoneConnections.Count-1;i>=0;--i){
     auto& reverse=tracking[entry.SubzoneConnections[i].SubzoneID].SubzoneConnections;
     for(int j=reverse.Count-1;j>=0;--j)if(reverse[j].SubzoneID==unsigned(*it)){reverse.RemoveItem(j);break;}
    }
    entry.SubzoneConnections.Clear();
   }
   int count=SubzoneTrackingCounts[level];
   for(int y=bounds.Y;y<endY;++y)for(int x=bounds.X;x<endX;++x){
    const CellStruct at{short(x),short(y)};if(!IsWithinUsableArea(at,true))continue;
    const int index=GetCellZoneIndex(at);auto& sub=LevelAndPassabilityStruct2pointer_70[index];
    const int pass=static_cast<unsigned char>(LevelAndPassability[index].CellPassability);
    if(pass==7||sub.SubzoneIDs[level])continue;
    // Native search reads signed IDs; compact before its representable range
    // is exhausted, using the same full-rebuild fallback as the original.
    if(count>=0x8000){ResetAllSubzones();return;}
    SubzoneSpan(&sub,level,count,bounds,at);
    if(!tracking.AddItem(SubzoneTrackingStruct{}))std::abort();auto& entry=tracking[count++];
    entry.ParentSubzoneID=level<2?sub.SubzoneIDs[level+1]:0;
    entry.Passability=pass;entry.SubzoneConnections.CapacityIncrement=16;
    entry.ThreatRegion=x/4+130*(y/4)+131;
   }
   SubzoneTrackingCounts[level]=count;
   const auto inside=[&](CellStruct at){return at.X>=bounds.X&&at.X<bounds.X+size&&at.Y>=bounds.Y&&at.Y<bounds.Y+size;};
   for(int i=ZoneConnections.Count-1;i>=0;--i){const auto& connection=ZoneConnections[i];
    if(connection.IsPassable&&(inside(connection.FromMapCoords)||inside(connection.ToMapCoords)))RegisterZoneConnectionEntries(connection,level);
   }
   for(int bucket=0;bucket<unknown_80[level]->BucketCount;++bucket)for(const auto& item:unknown_80[level]->Buckets[bucket]){
    const auto& link=item.Value;const int a=link.SubzoneID>>16,b=link.SubzoneID&0xFFFFu;
    if(!tracking[b].SubzoneConnections.AddItem({static_cast<unsigned>(a),link.IsCrossBlock})
      ||!tracking[a].SubzoneConnections.AddItem({static_cast<unsigned>(b),link.IsCrossBlock}))std::abort();
   }
  }
  const int x0=cell.X-cell.X%8,y0=cell.Y-cell.Y%8;
  for(int y=y0;y<std::min(y0+8,stride);++y)for(int x=x0;x<std::min(x0+8,stride);++x){
   const CellStruct at{short(x),short(y)};if(!IsWithinUsableArea(at,true))continue;
   const auto& sub=LevelAndPassabilityStruct2pointer_70[GetCellZoneIndex(at)];
   for(int level=0;level<2;++level)SubzoneTracking[level][sub.SubzoneIDs[level]].ParentSubzoneID=sub.SubzoneIDs[level+1];
  }
  AStarClass::Instance.Reset();
 }catch(...){std::abort();} // Original allocation failure semantics; no exception crosses the boundary.
}

void MapClass::ComputeZoneConnections() {
    constexpr int starts[]{7,7,-1,7,7,-1,4,4,4,4,4,2,2,2,2,2};
    constexpr int directions[]{2,2,-1,4,4,-1,2,2,2,2,2,4,4,4,4,4};
    constexpr int ends[]{-1,-1,4,-1,-1,2,4,4,4,4,4,2,2,2,2,2};
    const auto append=[&](const ZoneConnectionClass& connection){try{if(!ZoneConnections.AddItem(connection))std::abort();}catch(...){std::abort();}};
    ZoneConnections.Clear();CellIteratorReset();
    while(auto* cell=CellIteratorNext()) {
        if(!cell->Tile_Is_Bridge() && !cell->Tile_Is_WoodBridge()) {
            if(cell->GetTunnel() && ((cell->GetNeighbourCell(FacingType::East)->GetTunnel() && cell->GetNeighbourCell(FacingType::West)->GetTunnel())
                || (cell->GetNeighbourCell(FacingType::South)->GetTunnel() && cell->GetNeighbourCell(FacingType::North)->GetTunnel()))) {
                const auto exit=cell->GetTunnel()->ExitCell;
                if(cell->MapCoords.X+512*cell->MapCoords.Y<exit.X+512*exit.Y)append({cell->MapCoords,exit,true,1});
            }
            continue;
        }
        const int variant=cell->IsoTileTypeIndex-(cell->Tile_Is_Bridge()?IsometricTileTypeClass::BridgeSet:IsometricTileTypeClass::WoodBridgeSet);
        if(starts[variant]!=static_cast<unsigned char>(cell->Height))continue;
        const auto facing=static_cast<FacingType>(directions[variant]);bool crossed=false,passable=true;auto* cursor=cell;
        for(;;) {
            cursor=cursor->GetNeighbourCell(facing);const auto at=cursor->MapCoords;
            const bool within=at.X+at.Y>MapRect.Width && at.X-at.Y<MapRect.Width && at.Y-at.X<MapRect.Width && at.X+at.Y<=MapRect.Width+2*MapRect.Height;
            if(!within && !crossed)break;
            if(crossed){append({cell->MapCoords,cursor->GetNeighbourCell(static_cast<FacingType>((directions[variant]-4)&7))->MapCoords,passable,0});break;}
            if(cursor->Tile_Is_Bridge() || cursor->Tile_Is_WoodBridge()) {
                const int index=cursor->IsoTileTypeIndex-(cursor->Tile_Is_Bridge()?IsometricTileTypeClass::BridgeSet:IsometricTileTypeClass::WoodBridgeSet);
                if(ends[index]==static_cast<unsigned char>(cursor->Height))crossed=true;
            }else if(!(static_cast<unsigned>(cursor->Flags)&0x100u))passable=false;
        }
    }
}
