// Existing YRpp methods, calibrated against gamemd 1.001 at 0x00447AC0 and
// 0x00459EF0. The sprite render origin is distinct from the foundation center.
#include "yrpp/BuildingClass.h"
#include "yrpp/TacticalClass.h"
#include <bit>
#include <cstdint>
#include <cstring>

// OpenTS 44fac744 building.cpp Get_Render_Rect; YR 0x455C20.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
RectangleStruct* BuildingClass::GetRenderDimensions(RectangleStruct* output) {
    const auto render=GetRenderCoords();const auto camera=TacticalClass::Instance->TacticalPos;
    if(render!=unknown_coord_64C || camera.X!=unknown_int_658 || unsigned(camera.Y)!=unknown_65C){
        Point2D point;
        TacticalClass::CoordsToClient(render,camera,TacticalClass::ViewBounds,point);
        RectangleStruct bounds;Type->GetDrawRect(&bounds);
        bounds.X=std::bit_cast<int>(unsigned(bounds.X)+unsigned(point.X));
        bounds.Y=std::bit_cast<int>(unsigned(bounds.Y)+unsigned(point.Y));
        unknown_coord_64C=render;unknown_int_658=camera.X;unknown_65C=unsigned(camera.Y);
        unknown_rect_63C=bounds;
    }
    *output=unknown_rect_63C;return output;
}

// OpenTS Target_Coord returns its base; YR 0x4500A0 instead adds the
// building type's three-dimensional target offset to virtual GetCoords.
CoordStruct* BuildingClass::GetTargetCoords(CoordStruct* output) const {
    const auto at=GetCoords();const auto offset=Type->TargetCoordOffset;
    *output={std::bit_cast<int>(unsigned(at.X)+unsigned(offset.X)),
        std::bit_cast<int>(unsigned(at.Y)+unsigned(offset.Y)),
        std::bit_cast<int>(unsigned(at.Z)+unsigned(offset.Z))};
    return output;
}

int BuildingClass::GetYSort() const {
    // 0x00449410: TurretAnimIsVoxel (+0x16C5), Gate (+0x16B7).
    return std::bit_cast<std::int32_t>(std::uint32_t(ObjectClass::GetYSort())
        + (Type && Type->TurretAnimIsVoxel ? 32u : 0u)
        - (Type && Type->Gate ? 16u : 0u));
}

int BuildingClass::GetOccupantCount() const { return Occupants.Count; }
bool BuildingClass::IsTraversable() const {
    return !Type->Gate || (GetCurrentMission() == Mission::Open && !UnloadTimer.State1 && UnloadTimer.State2);
}
bool BuildingClass::CanOccupyFire() const {
    return Type&&Type->CanBeOccupied&&Type->CanOccupyFire&&GetOccupantCount()>0;
}

CoordStruct* BuildingClass::GetRenderCoords(CoordStruct* output) const {
    if (!output) return nullptr;
    *output = {
        std::bit_cast<std::int32_t>(std::uint32_t(Location.X) - 128u),
        std::bit_cast<std::int32_t>(std::uint32_t(Location.Y) - 128u),
        Location.Z};
    return output;
}

// Building vtable 0x7E3EBC + 0x48. The inherited +0x58 GetCenterCoords
// forwards to this virtual entry (0x410540), as do range/approach queries.
CoordStruct* BuildingClass::GetCoords(CoordStruct* output) const {
    if (!output || !Type) return nullptr;
    *output = {
        std::bit_cast<std::int32_t>(std::uint32_t(Location.X) +
            std::uint32_t(Type->GetFoundationWidth()) * 128u - 128u),
        std::bit_cast<std::int32_t>(std::uint32_t(Location.Y) +
            std::uint32_t(Type->GetFoundationHeight(false)) * 128u - 128u),
        Location.Z};
    return output;
}

// OpenTS Open_Gate; YR 0x452540.
bool BuildingClass::MakeTraversable() {
 if(!Type->Gate)return true;
 if(GetCurrentMission()!=Mission::Open||UnloadTimer.AreStates10()||UnloadTimer.AreStates00()){
  ForceMission(Mission::None);QueueMission(Mission::Open,false);NextMission();
 }else if(GetCurrentMission()==Mission::Open&&UnloadTimer.AreStates01())return true;
 return false;
}

// OpenTS 44fac744 building.cpp Aim_Direction; YR 0x43ED40.
DirStruct BuildingClass::FireAngleTo(AbstractClass* target) const {
    auto origin=GetCoords();
    const auto pixels=Type->PrimaryFirePixelOffset!=Point2D{0xFFFF,0xFFFF}
        ?Type->PrimaryFirePixelOffset:Type->BuildingAnim[9].Position;
    const auto offset=TacticalClass::Instance->ApplyMatrix_Pixel(pixels);
    origin.X=std::bit_cast<int>(unsigned(origin.X)+unsigned(offset.X));
    origin.Y=std::bit_cast<int>(unsigned(origin.Y)+unsigned(offset.Y));
    const auto destination=target->GetCoords();
    const double angle=Math::atan2(double(origin.Y)-destination.Y,double(destination.X)-origin.X);
    const auto direction=static_cast<unsigned short>(int((angle-1.5707963267948966)*-10430.060040584269));
    const std::uint32_t packed=(static_cast<std::uint32_t>(destination.X)&0xFFFF0000u)|direction;
    DirStruct result;std::memcpy(&result,&packed,sizeof(packed));return result;
}
