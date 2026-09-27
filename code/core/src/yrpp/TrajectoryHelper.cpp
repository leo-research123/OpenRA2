// SPDX-License-Identifier: GPL-3.0-or-later
// YR 0x4CC100/0x4CC310/0x4CC360/0x4CC680/0x4CC6D0, used by the
// OpenTS-derived Techno::In_Range. These YR wall/cliff helpers have no
// equivalent OpenTS object model to replace; retain MapClass.h's original API.
#include "yrpp/MapClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/YRMath.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <cstdlib>

bool YRPP_FASTCALL TrajectoryHelper::IsCliffHit(const CellClass* source,const CellClass* before,const CellClass* after){
    return after->GetLevel()-before->GetLevel()>=4&&after->GetLevel()>source->GetLevel();
}
bool YRPP_FASTCALL TrajectoryHelper::IsWallHit(const CellClass* source,const CellClass* check,const CellClass* target,const HouseClass* owner){
    auto* overlay=OverlayTypeClass::Array.GetItemOrDefault(check->OverlayTypeIndex);
    if(check==target||!overlay||!overlay->Wall||static_cast<signed char>(source->Level)>static_cast<signed char>(target->Level))return false;
    auto* wallOwner=HouseClass::Array.GetItemOrDefault(check->WallOwnerIndex);
    return !RulesClass::Instance->AlliedWallTransparency||!wallOwner||!wallOwner->IsAlliedWith(owner);
}
CellClass* YRPP_FASTCALL TrajectoryHelper::GetObstacle(const CellClass* source,const CellClass* target,
        const CellClass* before,CoordStruct at,const BulletTypeClass* type,const HouseClass* owner){
    auto* cell=MapClass::Instance.GetCellAt(at);
    const auto blocked=[&]{return (type->SubjectToCliffs&&IsCliffHit(source,before,cell))
        ||(type->SubjectToWalls&&IsWallHit(source,cell,target,owner));};
    if(!blocked())return nullptr;
    const auto center=cell->GetCoords();
    const int dx=at.X-center.X,dy=at.Y-center.Y,dz=at.Z-center.Z;
    if(rule_integer(Math::sqrt(double(dx)*dx+double(dy)*dy+double(dz)*dz))<=85)return cell;
    const auto from=source->GetCoords(),to=target->GetCoords();
    const int x=std::abs(from.X-to.X),y=std::abs(from.Y-to.Y);
    if((y<x&&std::abs(dy)>std::abs(dx))||(y>x&&std::abs(dx)>std::abs(dy)))
        return blocked()?cell:nullptr;
    return cell;
}
CellClass* YRPP_FASTCALL TrajectoryHelper::FindFirstObstacle(const CoordStruct& from,const CoordStruct& to,
        const BulletTypeClass* type,const HouseClass* owner){
    if(!type->SubjectToCliffs&&!type->SubjectToWalls)return nullptr;
    const CellStruct source{short(from.X/256),short(from.Y/256)},target{short(to.X/256),short(to.Y/256)};
    const int steps=std::max(std::abs(int(source.X)-target.X),std::abs(int(source.Y)-target.Y));
    if(steps<=0)return nullptr;
    const CoordStruct increment{(to.X-from.X)/steps,(to.Y-from.Y)/steps,(to.Z-from.Z)/steps};
    auto* sourceCell=MapClass::Instance.GetCellAt(source);
    auto* targetCell=MapClass::Instance.GetCellAt(target);
    auto* before=sourceCell;auto at=from;
    // Original samples the source, excludes the end point, and truncates
    // the step once. Do not replace this with a host ray/Bresenham traversal.
    for(int i=0;i<steps;++i){
        if(auto* blocked=GetObstacle(sourceCell,targetCell,before,at,type,owner))return blocked;
        before=MapClass::Instance.GetCellAt(at);
        at.X+=increment.X;at.Y+=increment.Y;at.Z+=increment.Z;
    }
    return nullptr;
}
CellClass* YRPP_FASTCALL TrajectoryHelper::FindFirstImpenetrableObstacle(const CoordStruct& from,const CoordStruct& to,
        const WeaponTypeClass* weapon,const HouseClass* owner){
    auto* obstacle=FindFirstObstacle(from,to,weapon->Projectile,owner);
    if(obstacle&&obstacle->ConnectsToOverlay(-1,-1)&&weapon->Warhead->Wall)return nullptr;
    return obstacle;
}
