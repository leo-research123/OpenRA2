// Existing YRpp RadarClass hash/object branch, calibrated to fixed YR
// 655560, 655740, 655C50 and 656750. No substitute object registry.
#include "yrpp/RadarClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/RulesClass.h"
#include "scenario_runtime.hpp"
#include <algorithm>

namespace {
using Bucket=DynamicVectorClass<HashObject<RadarTrackingStruct,TechnoClass*>>;
Bucket* bucket(const RadarClass& radar,int x,int y) noexcept {
    auto* table=radar.unknown_1258;
    if (!table || !table->Buckets || table->BucketCount!=256) return nullptr;
    return &table->Buckets[(unsigned(x)+251u*unsigned(y))&255u];
}
HouseClass* player() {
    const auto* houses=game::scenario_runtime().houses;
    return houses && houses->current_player ? *houses->current_player : nullptr;
}
bool controlled(const HouseClass* house,const HouseClass* local) {
    if (!house) return false;
    const auto& runtime=game::scenario_runtime();
    return runtime.session_mode && runtime.session_mode(runtime.context)==int(GameMode::Campaign)
        ? house->IsHumanPlayer || house->IsInPlayerControl : house==local;
}
bool neutral_color(WORD& output) {
    const auto* render=game::scenario_runtime().render;
    const auto* schemes=render ? render->color_schemes : nullptr;
    if (!schemes) return false;
    for (auto* scheme : *schemes) {
        if (scheme && scheme->ID && scheme->ShadeCount==1 && !_strcmpi(scheme->ID,"LightGrey")) {
            output=WORD(scheme->MainShadeIndex);return true;
        }
    }
    return false;
}
}

bool RadarClass::TrackObject(TechnoClass* object,int x,int y) noexcept {
    const auto& bounds=unknown_rect_149C;
    if (!object || bounds.Width<=0 || bounds.Height<=0) return false;
    try {
        if (x<0 || y<0 || x>=bounds.Width || y>=bounds.Height) {
            if (object->WhatAmI()==AbstractType::Building) return false;
            x=std::clamp(x,0,bounds.Width-1);y=std::clamp(y,0,bounds.Height-1);
            object->RadarPosition={x,y};
        }
        auto* entries=bucket(*this,x,y);
        if (!entries) return false;
        const RadarTrackingStruct key{object,x,y};
        for (const auto& entry : *entries) if (entry.Key==key) return true;
        const int previous=entries->Count;
        if (!entries->AddItem({key,object})) return false;
        // The first matching record supplies color; the last supplies picking.
        // Preserve original local-owner prepend and non-local append order.
        if (object->Owner==player()) {
            for (int i=previous;i>0;--i) entries->Items[i]=entries->Items[i-1];
            entries->Items[0]={key,object};
        }
        Point2D point{x,y};RefreshCrd(&point);unknown_bool_14D9=true;
        return true;
    } catch (...) { return false; }
}
bool RadarClass::UntrackObject(TechnoClass* object,int x,int y) noexcept {
    if (!object) return false;
    auto* entries=bucket(*this,x,y);
    if (!entries) return false;
    const HashObject<RadarTrackingStruct,TechnoClass*> entry{{object,x,y},object};
    if (!entries->Remove(entry)) return false;
    Point2D point{x,y};RefreshCrd(&point);unknown_bool_14D9=true;
    return true;
}
bool RadarClass::RadarToCell(const Point2D& point,CellStruct& cell,TechnoClass*& object) const noexcept {
    object=nullptr;
    const int x=point.X-unknown_rect_149C.X,y=point.Y-unknown_rect_149C.Y;
    if (x<0 || y<0 || x>=unknown_rect_149C.Width || y>=unknown_rect_149C.Height) return false;
    try {
        if (auto* entries=bucket(*this,x,y)) {
            for (int i=entries->Count-1;i>=0;--i) {
                const auto& key=entries->Items[i].Key;
                if (key.X==x && key.Y==y && key.Object) {
                    CoordStruct location;
                    const auto* result=key.Object->GetDestination(&location,nullptr);
                    if (!result) return false;
                    cell={short(result->X/256),short(result->Y/256)};
                    object=key.Object;return true;
                }
            }
        }
        return RadarToTerrainCell(point,cell);
    } catch (...) { object=nullptr;return false; }
}
bool RadarClass::ApplyTrackedObjects(WORD* pixels,unsigned count) const noexcept {
    const auto& rect=unknown_rect_149C;
    if (!pixels || rect.Width<1 || rect.Height<1 || rect.Width>140 || rect.Height>108 || count<unsigned(rect.Width*rect.Height)) return false;
    try {
        auto* local=player();
        if (!local) return true; // Terrain-only sessions have no synthetic House.
        for (int y=0;y<rect.Height;++y) for (int x=0;x<rect.Width;++x) {
            auto* entries=bucket(*this,x,y);
            if (!entries || !entries->Count) continue;
            CellStruct cell;
            if (!RadarToTerrainCell({rect.X+x,rect.Y+y},cell)) return false;
            auto* ground=MapClass::Instance.GetCellAt(cell);
            CoordStruct world=CellClass::Cell2Coord(cell,ground->GetFloorHeight({128,128}));
            const bool shrouded=MapClass::Instance.IsLocationShrouded(world);
            TechnoClass* chosen=nullptr;
            for (const auto& entry : *entries) {
                const auto& key=entry.Key;auto* candidate=key.Object;
                if (key.X!=x || key.Y!=y || !candidate) continue;
                auto* type=candidate->GetType();
                if (!type || (shrouded && !controlled(candidate->Owner,local)) ||
                    (type->RadarInvisible && !local->IsAlliedWith(candidate))) continue;
                if (type->Insignificant) {
                    const auto* techno_type=static_cast<const TechnoTypeClass*>(type);
                    if (!techno_type->RadarVisible &&
                        (!candidate->Owner || !candidate->Owner->Type || candidate->Owner->Type->MultiplayPassive)) continue;
                }
                chosen=candidate;break;
            }
            if (!chosen) continue;
            auto* owner=chosen->IsDisguised() ? chosen->GetDisguiseHouse(false) : chosen->Owner;
            WORD color=0;
            if (owner) color=WORD((unsigned(owner->Color.R)>>3)<<11 | (unsigned(owner->Color.G)>>2)<<5 | (unsigned(owner->Color.B)>>3));
            else if (!neutral_color(color)) return false;
            const int remaining=chosen->RadarFlashTimer.GetTimeLeft();
            const auto* rules=RulesClass::Instance;
            if (chosen->Owner==local && remaining>0 && rules && rules->FlashFrameTime>0 &&
                (remaining-1)/rules->FlashFrameTime%2==1) color=WORD(~color);
            pixels[y*rect.Width+x]=color;
        }
        return true;
    } catch (...) { return false; }
}
