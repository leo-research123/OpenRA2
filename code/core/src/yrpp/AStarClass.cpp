// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 astar.cpp regular search; YR 0x429A90..0x42AC00.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AStarClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include <algorithm>
#include <bit>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <new>

struct AStarClass_PathNode {CellClass** CellSlot;int Level;AStarClass_PathNode* Parent;};
struct AStarClass_PathQueueNode {
    AStarClass_PathNode* Node;float Cost;float Score;int Length;
    bool operator<(const AStarClass_PathQueueNode& other)const{return Score<other.Score;}
};
struct AStarClass_PathNodeBuffer {AStarClass_PathNode Nodes[131072];int Count;};
struct AStarClass_PathQueueBuffer {AStarClass_PathQueueNode Nodes[65536];int Count;};
struct AStarClass_HierarchicalNode {
    int ParentIndex;int SubzoneID;float Score;int Depth;
    bool operator<(const AStarClass_HierarchicalNode& other)const{return Score<other.Score;}
};
struct AStarClass_HierarchicalBuffer {AStarClass_HierarchicalNode Nodes[10000];};
#if UINTPTR_MAX == UINT32_MAX
static_assert(sizeof(AStarClass_PathNode)==0xC && sizeof(AStarClass_PathQueueNode)==0x10);
static_assert(sizeof(AStarClass_PathNodeBuffer)==0x180004 && sizeof(AStarClass_PathQueueBuffer)==0x100004);
static_assert(sizeof(PriorityQueueClass_PathQueueNode)==0x14 && sizeof(PriorityQueueClass_HierarchicalNode)==0x14);
#endif
static_assert(sizeof(AStarClass_HierarchicalNode)==0x10 && sizeof(AStarClass_HierarchicalBuffer)==0x27100);
namespace {
int map_stride=0;
int path_levels[2001]{};
PathFinderData final_path{};
template<class T> T* allocate(int count=1) {
    auto* result=static_cast<T*>(YRMemory::Allocate(sizeof(T)*static_cast<std::size_t>(count)));
    if(!result)std::abort();
    // Construct directly in the pool: a value-initialized multi-megabyte
    // temporary would exceed the original Windows thread's stack.
    for(int i=0;i<count;++i)::new(static_cast<void*>(result+i)) T{};
    return result;
}
int height(const CellClass* c){return static_cast<signed char>(c->Level);}
bool bridge(const CellClass* c){return (static_cast<unsigned>(c->Flags)&0x100u)!=0;}
float store_float(double value) {
    const float rounded=static_cast<float>(value);
    return std::abs(double(rounded))>std::abs(value)?std::bit_cast<float>(std::bit_cast<unsigned>(rounded)-1u):rounded;
}
template<class T> PriorityQueueClass<T>* allocate_queue(int capacity) {
    void* storage=YRMemory::Allocate(sizeof(PriorityQueueClass<T>));
    if(!storage)std::abort();
    auto* queue=::new(storage) PriorityQueueClass<T>(capacity);
    queue->RMost=reinterpret_cast<T*>(~std::uintptr_t{});
    return queue;
}
AStarClass astar;
unsigned edge_key(int a,int b){const auto first=static_cast<unsigned short>(a),second=static_cast<unsigned short>(b);return (unsigned(std::min(first,second))<<16)|std::max(first,second);}
void ban_edge(AStarClass& finder,int from,int to,int level){if(from!=to)finder.ZoneIndices[level].AddItem(edge_key(from,to));}
}
AStarClass& AStarClass::Instance=astar;

AStarClass::AStarClass() noexcept
 : unknown_byte_0{},FindBridgeDir{},unknown_byte_2{},CanFindPath{true},PathCostFactor{1},IsAlt{true},
 PathNodeBuffer{allocate<AStarClass_PathNodeBuffer>()},PathQueueBuffer{allocate<AStarClass_PathQueueBuffer>()},
 PathQueue{allocate_queue<AStarClass_PathQueueNode>(65536)},VisitCounts{},AltVisitCounts{},AltDistances{},Distances{},
 SearchID{-1},FinderSpeedType{SpeedType::None},StartLevel{},EndLevel{},IsSearching{true},FindMode{},
 LevelVisitedMarkers{},OpenSetMarkers{},GCostArray{},HierarchyBuffer{allocate<AStarClass_HierarchicalBuffer>()},
 HierarchyQueue{allocate_queue<AStarClass_HierarchicalNode>(10000)},PathLength{-1},CellStructBuffer{},ZoneIndices{},PassabilityData{},PassabilityCounts{} {}
AStarClass::~AStarClass() {
    PathQueue->~PriorityQueueClass();YRMemory::Deallocate(PathQueue);
    HierarchyQueue->~PriorityQueueClass();YRMemory::Deallocate(HierarchyQueue);
    YRMemory::Deallocate(PathNodeBuffer);YRMemory::Deallocate(PathQueueBuffer);YRMemory::Deallocate(HierarchyBuffer);
    YRMemory::Deallocate(VisitCounts);YRMemory::Deallocate(AltVisitCounts);YRMemory::Deallocate(Distances);YRMemory::Deallocate(AltDistances);
    for(int i=0;i<3;++i){YRMemory::Deallocate(LevelVisitedMarkers[i]);YRMemory::Deallocate(OpenSetMarkers[i]);YRMemory::Deallocate(GCostArray[i]);}
}
void AStarClass::UpdateMapDimensions(const RectangleStruct& dimensions) {
    YRMemory::Deallocate(VisitCounts);YRMemory::Deallocate(AltVisitCounts);YRMemory::Deallocate(Distances);YRMemory::Deallocate(AltDistances);
    map_stride=dimensions.Width+dimensions.Height+1;
    if(map_stride<=0 || map_stride>1024)std::abort();
    const int count=map_stride*map_stride;
    // OpenTS initializes stamps; raw allocator contents are not valid visits.
    VisitCounts=allocate<int>(count);AltVisitCounts=allocate<int>(count);
    Distances=allocate<float>(count);AltDistances=allocate<float>(count);
}
void AStarClass::Reset() {
    for(int i=0;i<3;++i) {
        YRMemory::Deallocate(LevelVisitedMarkers[i]);YRMemory::Deallocate(OpenSetMarkers[i]);YRMemory::Deallocate(GCostArray[i]);
        const int count=MapClass::Instance.SubzoneTrackingCounts[i];
        LevelVisitedMarkers[i]=count?allocate<int>(count):nullptr;
        OpenSetMarkers[i]=count?allocate<int>(count):nullptr;
        GCostArray[i]=count?allocate<float>(count):nullptr;
    }
}
void AStarClass::Clear() {
    PathNodeBuffer->Count=PathQueueBuffer->Count=0;
    PathQueue->Clear();HierarchyQueue->Clear();
    SearchID=std::bit_cast<int>(static_cast<unsigned>(SearchID)+1u);
    if(!SearchID) {
        if(VisitCounts)std::fill_n(VisitCounts,map_stride*map_stride,0);
        if(AltVisitCounts)std::fill_n(AltVisitCounts,map_stride*map_stride,0);
        for(int i=0;i<3;++i){const int count=MapClass::Instance.SubzoneTrackingCounts[i];
            if(count){std::fill_n(LevelVisitedMarkers[i],count,0);std::fill_n(OpenSetMarkers[i],count,0);std::fill_n(GCostArray[i],count,0.0f);}}
        ++SearchID;
    }
}
AStarClass_PathQueueNode* AStarClass::CreatePathNode(const AStarClass_PathQueueNode* parent,CellClass** slot,const CellStruct& destination,float cost) {
    auto* node=&PathNodeBuffer->Nodes[PathNodeBuffer->Count++];
    auto* open=&PathQueueBuffer->Nodes[PathQueueBuffer->Count++];
    node->CellSlot=slot;node->Parent=parent?parent->Node:nullptr;node->Level=StartLevel;
    if(parent){auto* cell=*slot;auto* from=*parent->Node->CellSlot;node->Level=height(cell);
        if(bridge(cell) && ((bridge(from) && parent->Node->Level==height(from)+4)
            || (!bridge(from) && std::abs(height(cell)-parent->Node->Level+3)<=1)))node->Level+=4;}
    open->Node=node;open->Cost=parent?store_float(double(parent->Cost)+cost):0;open->Length=parent?parent->Length+1:1;
    const double dx=(*slot)->MapCoords.X-destination.X,dy=(*slot)->MapCoords.Y-destination.Y;
    open->Score=store_float(open->Cost+Math::sqrt(dx*dx+dy*dy));return open;
}
PathFinderData* YRPP_STDCALL AStarClass::BuildFinalPath(const AStarClass_PathQueueNode* final,int* directions) {
    final_path.TotalDistance=static_cast<int>(final->Score);if(!final_path.TotalDistance)final_path.TotalDistance=1;
    final_path.PathLength=final->Length;final_path.Directions=directions;final_path.Levels=path_levels;
    final_path.unknown_int_10=0;final_path.unknown_cellstruct_18={0,0};
    auto* current=final->Node;
    constexpr int facing_table[3][3]{{3,4,5},{2,-1,6},{1,0,7}};
    for(int i=final->Length-2;i>=0;--i) {
        auto* parent=current->Parent;path_levels[i]=parent->Level;
        const int dx=(*parent->CellSlot)->MapCoords.X-(*current->CellSlot)->MapCoords.X;
        const int dy=(*parent->CellSlot)->MapCoords.Y-(*current->CellSlot)->MapCoords.Y;
        directions[i]=std::abs(dx)>1 || std::abs(dy)>1?8:facing_table[dy+1][dx+1];current=parent;
    }
    directions[final->Length-1]=-1;final_path.StartCell=(*current->CellSlot)->MapCoords;return &final_path;
}
PathFinderData* AStarClass::FindPathRegular(const CellStruct& start,const CellStruct& end,FootClass* foot,int* directions,int maxSteps,bool hierarchical) {
    auto& map=MapClass::Instance;
    if(!VisitCounts || !AltVisitCounts || !directions)std::abort(); // Dimensions must be set before search.
    const int start_index=MapClass::GetCellIndex(start),end_index=MapClass::GetCellIndex(end);
    if(start_index<0 || end_index<0 || start_index>=map.Cells.Capacity || end_index>=map.Cells.Capacity)return nullptr;
    auto** from=map.Cells.Items+start_index;auto** to=map.Cells.Items+end_index;
    if(!*from || !*to)return nullptr;
    const bool aircraft=foot->WhatAmI()==AbstractType::Aircraft;
    EndLevel=height(*to)+(!aircraft && bridge(*to)?4:0);StartLevel=height(*from)+(!aircraft && foot->OnBridge?4:0);
    auto* type=foot->GetTechnoType();
    if(type->IsTrain && bridge(*from) && std::abs(foot->Location.Z/Unsorted::LevelHeight-StartLevel)>2)StartLevel+=4;
    FinderSpeedType=type->SpeedType;PathLength=0;CellStructBuffer=start;
    auto* working=CreatePathNode(nullptr,from,end,0);
    if(start==end && StartLevel==EndLevel)return nullptr;
    if(FindMode)ApplyPathCollisionAvoidance(foot);
    struct RestorePrediction {AStarClass* finder;FootClass* foot;~RestorePrediction(){if(finder->FindMode)finder->ApplyPathCollisionAvoidance(foot);}} restore{this,foot};
    const auto index_of=[&](const CellClass* cell){return cell->MapCoords.X+map_stride*cell->MapCoords.Y;};
    int index=index_of(*from);if(index<0 || index>=map_stride*map_stride)return nullptr;
    (StartLevel>height(*from)?AltVisitCounts:VisitCounts)[index]=SearchID;
    (StartLevel>height(*from)?AltDistances:Distances)[index]=0;
    constexpr int offsets[]{-512,-511,1,513,512,511,-1,-513};
    constexpr float facing_costs[]{0.001f,0.005f,0.002f,0.006f,0.003f,0.007f,0.004f,0.008f};
    if(type->IsTrain)for(int face=0;face<8;++face) {
        const int difference=std::abs(static_cast<int>(foot->PrimaryFacing.Current().GetFacing<8>())-face);
        if(difference>2 && difference<6){auto* cell=from[offsets[face]];if(!cell)continue;index=index_of(cell);
            if(index<0 || index>=map_stride*map_stride)continue;
            (StartLevel>height(cell)+1?AltVisitCounts:VisitCounts)[index]=SearchID;
            (StartLevel>height(cell)+1?AltDistances:Distances)[index]=0;}}
    const bool passive=foot->WhatAmI()==AbstractType::Unit && static_cast<UnitClass*>(foot)->Type->Passive;
    if(maxSteps<0)maxSteps=65527;
    int tries=0;bool reached_obstacle=false;
    while(working && tries<maxSteps) {
        auto** current=working->Node->CellSlot;
        if(current==to && working->Node->Level==EndLevel)break;
        AStarClass_PathQueueNode* candidate=nullptr;
        for(int face=0;face<=8;++face) {
            CellClass** neighbour;
            if(face==8){const auto* tube=(*current)->GetTunnel();if(!tube)continue;
                const int tunnel_index=MapClass::GetCellIndex(tube->ExitCell);
                if(tunnel_index<0 || tunnel_index>=map.Cells.Capacity)continue;
                neighbour=map.Cells.Items+tunnel_index;
            } else {const auto flat=(current-map.Cells.Items)+offsets[face];if(flat<0 || flat>=map.Cells.Capacity)continue;neighbour=map.Cells.Items+flat;}
            auto* cell=*neighbour;if(!cell)continue;
            index=index_of(cell);if(index<0 || index>=map_stride*map_stride)continue;
            const bool ground=!bridge(cell) || std::abs(StartLevel-height(cell))<=1;
            int subzone=0;
            if(map.LevelAndPassabilityStruct2pointer_70 && map.ValidMapCellCount>0)
                subzone=map.LevelAndPassabilityStruct2pointer_70[std::clamp(index,0,map.ValidMapCellCount-1)].SubzoneIDs[0];
            if(hierarchical && ground && !cell->BlockedNeighbours
                && (!LevelVisitedMarkers[0] || LevelVisitedMarkers[0][subzone]!=SearchID))continue;
            auto* visited=ground?VisitCounts:AltVisitCounts;auto* costs=ground?Distances:AltDistances;
            if(visited[index]==SearchID && double(working->Cost)+1.009>costs[index])continue;
            Move move=foot->IsCellOccupied(cell,static_cast<FacingType>(face),StartLevel,*current,IsAlt);
            if(type->IsTrain && move<Move::No)move=Move::OK;
            float cost;
            if(face==8)cost=static_cast<float>(std::max(std::abs((*current)->MapCoords.X-cell->MapCoords.X),std::abs((*current)->MapCoords.Y-cell->MapCoords.Y)));
            else cost=store_float(GetMovementCost(current,neighbour,!ground,move,foot)*PathCostFactor+facing_costs[face]);
            if(move<Move::No) {
                if(visited[index]==SearchID)continue;
                // Preserve OpenTS's memory guards; exhaustion is failure to expand,
                // never a write beyond the original fixed pools or caller path storage.
                if(PathNodeBuffer->Count>=131072 || PathQueueBuffer->Count>=65536 || working->Length>=2000)continue;
                auto* node=CreatePathNode(working,neighbour,end,cost);
                // YR's INC overwrites the score-comparison flags before JE. The
                // last generated neighbour stays the candidate, unlike OpenTS.
                if(candidate)PathQueue->Insert(candidate);
                candidate=node;visited[index]=SearchID;costs[index]=node->Cost;
                if(PathLength+1<500 && subzone==PassabilityData[0].Indices[PathLength+1]){++PathLength;CellStructBuffer=cell->MapCoords;}
            } else if(neighbour==to && !passive && std::abs(StartLevel-EndLevel)<=1){reached_obstacle=true;break;}
        }
        if(reached_obstacle)break;
        working=candidate?PathQueue->Replace_Root(candidate):PathQueue->Extract_Min();if(working)StartLevel=working->Node->Level;++tries;
    }
    if(tries==10000 || !working || tries==maxSteps || working->Length<2)return nullptr;
    auto* result=BuildFinalPath(working,directions);CutCorners(result,foot);OptimizeMoves(result,foot);return result;
}

bool AStarClass::FindPathHierarchical(const CellStruct& start,const CellStruct& end,MovementZone movement,const FootClass* foot) {
    auto& map=MapClass::Instance;const auto* pass=MapClass::MovementAdjustArray[static_cast<int>(movement)];
    const double avoidance=foot?foot->ThreatAvoidanceValue():0;const bool avoid=avoidance>0.00001;
    constexpr float passability_cost[]{1,0,0,1,1,0,1,1};
    for(int level=2;level>=0;--level) {
        HierarchyQueue->Clear();
        const int from=map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex(start)].SubzoneIDs[level];
        const int to=map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex(end)].SubzoneIDs[level];
        auto* final=LevelVisitedMarkers[level];auto* opened=OpenSetMarkers[level];auto* costs=GCostArray[level];
        if(!final || !opened || !costs)std::abort(); // Map graph and Reset are required, no fake single-zone fallback.
        final[from]=final[to]=SearchID;
        if(from==to){if(!level){HierarchyBuffer->Nodes[0].Depth=0;HierarchyBuffer->Nodes[0].SubzoneID=from;}
            PassabilityData[level].Indices[0]=static_cast<unsigned short>(from);PassabilityCounts[level]=1;continue;}
        auto* nodes=HierarchyBuffer->Nodes;nodes[0]={-1,from,0,0};HierarchyQueue->Insert(nodes);int count=1;
        opened[from]=SearchID;costs[from]=0;
        auto* best=HierarchyQueue->Extract_Min();
        const bool no_banned=ZoneIndices[level].Count==0;
        while(best && best->SubzoneID!=to) {
            const auto& connections=map.SubzoneTracking[level][best->SubzoneID].SubzoneConnections;
            for(int i=0;i<connections.Count;++i) {
                const auto& connection=connections[i];const int next=static_cast<int>(connection.SubzoneID);
                const auto& record=map.SubzoneTracking[level][next];const int terrain=static_cast<int>(record.Passability);
                const int threat=avoid?static_cast<int>(MapClass::RegionThreat(foot->Owner,level,best->SubzoneID,next)*avoidance):0;
                const float score=store_float(double(passability_cost[terrain])+best->Score+threat+(connection.IsCrossBlock?0.001:0));
                if((opened[next]!=SearchID || costs[next]>score)
                    && (level==2 || LevelVisitedMarkers[level+1][record.ParentSubzoneID]==SearchID || terrain==1)
                    && pass[terrain]==1 && (no_banned || ZoneIndices[level].FindItemIndex(edge_key(best->SubzoneID,next))==-1)) {
                    if(count>=10000)return false; // Fixed original pool, preserve OpenTS overflow protection.
                    auto* node=nodes+count++;*node={static_cast<int>(best-nodes),next,score,best->Depth+1};
                    HierarchyQueue->Insert(node);opened[next]=SearchID;costs[next]=score;
                }
            }
            best=HierarchyQueue->Extract_Min();
        }
        if(!best || best->Depth+1>500)return false;
        for(auto* node=best;node->ParentIndex!=-1;node=nodes+node->ParentIndex)final[node->SubzoneID]=SearchID;
        PassabilityCounts[level]=best->Depth+1;
        auto* node=best;
        for(int i=best->Depth;i>0;--i){PassabilityData[level].Indices[i]=static_cast<unsigned short>(node->SubzoneID);node=nodes+node->ParentIndex;}
        PassabilityData[level].Indices[0]=static_cast<unsigned short>(node->SubzoneID);
    }
    return true;
}

void AStarClass::BanNeighbourhoodSubzoneEdges(int subzone,int level) {
    const int count=PassabilityCounts[level];if(count<=1){IsSearching=false;return;}
    int index=0;while(index<count && PassabilityData[level].Indices[index]!=subzone)++index;
    if(index==count){IsSearching=false;return;}
    const auto* path=PassabilityData[level].Indices;
    const int node=index==count-1?path[index]:path[index+1],neighbour=index==count-1?path[index-1]:path[index];
    ban_edge(*this,node,neighbour,level);
    const auto& a=MapClass::Instance.SubzoneTracking[level][node].SubzoneConnections;
    const auto& b=MapClass::Instance.SubzoneTracking[level][neighbour].SubzoneConnections;
    for(int j=a.Count-1;j>=0;--j){const auto common=static_cast<unsigned short>(a[j].SubzoneID);if(common==neighbour)continue;
        for(int i=b.Count-1;i>=0;--i)if(b[i].SubzoneID==common)ban_edge(*this,neighbour,common,level);}
}
void AStarClass::BanBlockedSubzoneEdges(const FootClass* foot) {
    auto& map=MapClass::Instance;const auto& subzones=map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex(CellStructBuffer)];
    for(int level=0;level<3;++level){const int from=static_cast<unsigned short>(subzones.SubzoneIDs[level]);DynamicVectorClass<unsigned short> unreachable;
        if(map.BuildReachableSubzones(map.GetCellAt(CellStructBuffer),level,unreachable,foot))BanNeighbourhoodSubzoneEdges(from,level);
        else for(int i=unreachable.Count-1;i>=0;--i)ban_edge(*this,from,unreachable[i],level);}
}

PathFinderData* AStarClass::FindPath(CellStruct* start,CellStruct* end,FootClass* foot,int* directions,int maximum,MovementZone movement,int mode) {
    IsSearching=true;Clear();for(auto& edges:ZoneIndices)edges.Clear();FindMode=mode;
    auto& map=MapClass::Instance;auto* from=map.GetCellAt(*start);auto* to=map.GetCellAt(*end);auto* type=foot->GetTechnoType();
    if(movement==MovementZone::None)movement=type->MovementZone;
    const int from_zone=map.GetMovementZoneType(*start,movement,foot->OnBridge),to_zone=map.GetMovementZoneType(*end,movement,bridge(to));
    CellStruct from_bridge,to_bridge;MapClass::GetBridgeZoneConnectionCell(&from_bridge,from,foot->OnBridge);MapClass::GetBridgeZoneConnectionCell(&to_bridge,to,bridge(to));
    if(foot->WhatAmI()==AbstractType::Infantry && static_cast<InfantryClass*>(foot)->Type->JumpJet) {
        movement=MovementZone::Infantry;ILocomotion* driver=foot->Locomotor;if(!driver)std::abort();
        constexpr GUID iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};IPersist* persist=nullptr;
        try {if(driver->QueryInterface(iid,reinterpret_cast<void**>(&persist))<0 || !persist)std::abort();
            CLSID id{};persist->GetClassID(&id);persist->Release();}
        catch(...){std::abort();} // Original COM failure is fatal, not an exception across the core ABI.
    }
    bool hierarchical=!type->IsTrain && foot->IsInPlayfield && !foot->vt_entry_320()
        && map.IsWithinUsableArea(from_bridge,true) && map.IsWithinUsableArea(to_bridge,true);
    if(from_zone!=to_zone && hierarchical)return nullptr;
    if(hierarchical && !FindPathHierarchical(from_bridge,to_bridge,movement,foot))hierarchical=false;
    const int retries=maximum==-1?5:1;
    for(int attempt=0;;) {
        auto* result=FindPathRegular(*start,*end,foot,directions,maximum,hierarchical);
        if(result || !hierarchical)return result;
        ++attempt;BanBlockedSubzoneEdges(foot);Clear();hierarchical=IsSearching;
        if(attempt>=retries || (hierarchical && !FindPathHierarchical(from_bridge,to_bridge,movement,foot)))return nullptr;
    }
}

int AStarClass::AttemptPath(CellStruct* start,CellStruct* end,FootClass* foot,bool fromAlt,bool toAlt,MovementZone movement) {
    IsSearching=true;Clear();for(auto& edges:ZoneIndices)edges.Clear();
    auto& map=MapClass::Instance;CellStruct from,to;
    MapClass::GetBridgeZoneConnectionCell(&from,map.GetCellAt(*start),fromAlt);
    MapClass::GetBridgeZoneConnectionCell(&to,map.GetCellAt(*end),toAlt);
    if(movement==MovementZone::None)movement=foot?foot->GetTechnoType()->MovementZone:MovementZone::Normal;
    if(!FindPathHierarchical(from,to,movement,foot))return 0x7FFFFFFF;
    const auto distance=[](const CellStruct& a,const CellStruct& b){return std::max(std::abs(a.X-b.X),std::abs(a.Y-b.Y));};
    const int direct=distance(*start,*end),count=PassabilityCounts[0];int cost=2*count-2;
    if(toAlt) {
        if(fromAlt){const int connection=map.ZoneConnectionIndex(*end,3,0);if(connection!=-1 && connection==map.ZoneConnectionIndex(*start,3,0))return direct;}
        CellStruct candidate{0,0};
        if(count>=4)map.FindBridgeEndCellForSubzone(&candidate,*end,0,PassabilityData[0].Indices[count-2]);
        if(candidate==CellStruct{0,0})map.FindBridgeEndCellForSubzone(&candidate,*end,0,PassabilityData[0].Indices[count-1]);
        if(candidate==CellStruct{0,0})candidate=to;
        if(candidate!=CellStruct{0,0})cost+=distance(*end,candidate);
    }
    if(fromAlt) {
        CellStruct candidate{0,0};
        if(count>=4)map.FindBridgeEndCellForSubzone(&candidate,*start,0,PassabilityData[0].Indices[1]);
        if(candidate==CellStruct{0,0})map.FindBridgeEndCellForSubzone(&candidate,*start,0,PassabilityData[0].Indices[0]);
        if(candidate==CellStruct{0,0})candidate=from;
        if(candidate!=CellStruct{0,0})cost+=distance(*start,candidate);
    }
    return std::max(direct,cost);
}
