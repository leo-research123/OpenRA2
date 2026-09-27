// YR aircraft spatial index, 0x4129C0–0x413A70. This module has no
// corresponding tracker in OpenTS 44fac744; calibrated directly against YR.
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/MapClass.h"
#include <algorithm>

AircraftTrackerClass& AircraftTrackerClass::Instance=[]() -> AircraftTrackerClass& {
    static AircraftTrackerClass tracker;
    return tracker;
}();

int AircraftTrackerClass::GetVectorIndex(CellStruct pos) {
    const auto& bounds=MapClass::Instance.MapCoordBounds;
    // YR divides by the absolute map bounds, not MapRect.Width/Height.
    const int x=std::min(19,std::max(0,int(pos.X))/(bounds.Right/20));
    const int y=std::min(19,std::max(0,int(pos.Y))/(bounds.Bottom/20));
    return x+20*y;
}
void AircraftTrackerClass::FillCurrentVector(CellClass* cell,int range) {
    range=std::max(range,1);
    const auto at=CellClass::Coord2Cell(cell->GetCoords());
    const int origin=GetVectorIndex(at);
    const auto append=[&](int index) {
        auto& source=TrackerVectors[index/20][index%20];
        const int count=source.Count;
        for(int i=0;i<count;++i)CurrentVector.AddItem(source[i]);
    };
    const auto step=[](int index,int x,int y) {
        return std::clamp(index%20+x,0,19)+20*std::clamp(index/20+y,0,19);
    };
    append(origin);
    // Exact YR insertion order: center, E/N/S/W rays (outside in), then
    // NW/SW/NE/SE diagonals and the two additional cells per quadrant.
    const auto ray=[&](CellStruct end,int dx,int dy) {
        int count=0;
        for(int i=GetVectorIndex(end);i!=origin;i=step(i,dx,dy)){append(i);++count;}
        return count;
    };
    const int east=ray({short(at.X+range),at.Y},-1,0);
    const int north=ray({at.X,short(at.Y-range)},0,1);
    const int south=ray({at.X,short(at.Y+range)},0,-1);
    const int west=ray({short(at.X-range),at.Y},1,0);
    const auto corner=[&](int horizontal,int vertical,int dx,int dy) {
        int index=origin;
        for(int i=0;i<std::min(horizontal,vertical);++i){index=step(index,dx,dy);append(index);}
        if(horizontal>=2 && vertical>=2){append(step(origin,2*dx,dy));append(step(origin,dx,2*dy));}
    };
    corner(west,north,-1,-1);corner(west,south,-1,1);
    corner(east,north,1,-1);corner(east,south,1,1);
}
TechnoClass* AircraftTrackerClass::Get() {
    if(!CurrentVector.Count)return nullptr;
    auto* result=CurrentVector[0];CurrentVector.Remove(result);return result;
}
void AircraftTrackerClass::Add(TechnoClass* entry) {
    const auto at=entry->GetMapCoords();entry->SetLastFlightMapCoords(at);
    const int index=GetVectorIndex(at);TrackerVectors[index/20][index%20].AddItem(entry);
}
void AircraftTrackerClass::Remove(TechnoClass* entry) {
    const auto at=entry->GetLastFlightMapCoords();entry->SetLastFlightMapCoords(CellStruct::Empty);
    const int index=GetVectorIndex(at);TrackerVectors[index/20][index%20].Remove(entry);
}
void AircraftTrackerClass::Update(TechnoClass* entry,CellStruct before,CellStruct after) {
    entry->SetLastFlightMapCoords(after);
    const int from=GetVectorIndex(before),to=GetVectorIndex(after);
    if(from==to)return;
    TrackerVectors[from/20][from%20].Remove(entry);
    TrackerVectors[to/20][to%20].AddItem(entry);
}
bool AircraftTrackerClass::Clear() {
    const auto reset=[](auto& vector) {
        const int capacity=vector.Capacity;vector.Clear();return vector.SetCapacity(capacity);
    };
    for(auto& row:TrackerVectors)for(auto& vector:row)reset(vector);
    return reset(CurrentVector);
}
bool AircraftTrackerClass::IsJumpjet(TechnoClass* entry) {
    const auto* occupant=entry->GetCell()->Jumpjet;
    return occupant && static_cast<const void*>(occupant)!=static_cast<const void*>(entry);
}
