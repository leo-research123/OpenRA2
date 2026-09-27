// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 building.cpp
// Fire_Coord / Turret_Coord / Voxel_Fire_Coord / Get_Barrel_Matrix.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA terms: third_party/opents/LICENSE.md. YR 0x00453840/0x00453A70/
// 0x00453BF0/0x00458810; keep branch priority and float-store boundaries.
#include "yrpp/BuildingClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "RulesClassReaders.hpp"
#include "Matrix3DArithmetic.hpp"

namespace {
int add_coordinate(int a,int b) noexcept {
 return std::bit_cast<int>(unsigned(a)+unsigned(b));
}
CoordStruct* add_pixel_offset(CoordStruct* output,const Point2D& point) {
 const auto offset=TacticalClass::Instance->ApplyMatrix_Pixel(point);
 output->X=add_coordinate(output->X,offset.X);output->Y=add_coordinate(output->Y,offset.Y);return output;
}
void translate_barrel(Matrix3D& matrix,const CoordStruct& offset) {
 // Original Translate_X/Y/Z store each axis separately under x87 0x0E7F.
 const float axes[]{matrix_store_float(offset.X),matrix_store_float(offset.Y),matrix_store_float(offset.Z)};
 for(int axis=0;axis<3;++axis)for(auto& row:matrix.row)
  row[3]=matrix_store_float(matrix_add(double(axes[axis])*row[axis],row[3]));
}
float barrel_angle(const FacingClass& facing) {
 return matrix_store_float((int(facing.Current().GetValue<5>())-8)*-0.1963495408493621);
}
}

Matrix3D* BuildingClass::GetVoxelBarrelOffsetMatrix(Matrix3D& output) {
 output.MakeIdentity();
 translate_barrel(output,Type->VoxelBarrelOffsetToBuildingPivotPoint);
 output.RotateZ(barrel_angle(PrimaryFacing));
 translate_barrel(output,Type->VoxelBarrelOffsetToRotatePivotPoint);
 output.RotateY(-barrel_angle(BarrelFacing));
 translate_barrel(output,Type->VoxelBarrelOffsetToPitchPivotPoint);
 const float scale=matrix_store_float(Type->VoxelBarrelScale);
 for(auto& row:output.row)for(int column=0;column<3;++column)
  row[column]=matrix_store_float(double(row[column])*scale);
 return &output;
}

CoordStruct* BuildingClass::GetVoxelFireCoords(CoordStruct* output,int weapon,bool justFired) const noexcept {
 // 0x00453BF0 uses the previous burst index for a just-fired muzzle,
 // wrapping to Weapon.Burst-1. Indices beyond the two barrels use zero Y.
 const auto* weaponType=GetWeapon(weapon)->WeaponType;
 const int burst=justFired?(CurrentBurstIndex?CurrentBurstIndex-1:weaponType->Burst-1):CurrentBurstIndex;
 auto end=Type->VoxelBarrelOffsetToBarrelEnd;
 end.Y=burst==0?end.Y:burst==1?std::bit_cast<int>(0u-unsigned(end.Y)):0;
 Matrix3D matrix;
 // YRpp's existing matrix entry is non-const; the original only reads state.
 const_cast<BuildingClass*>(this)->GetVoxelBarrelOffsetMatrix(matrix);
 const double x=matrix_store_float(end.X),y=matrix_store_float(end.Y),z=matrix_store_float(end.Z);
 // Matrix3D transform 0x005AFB80 retains the original row-specific sum order.
 const auto& r=matrix.row;
 const float dx=matrix_store_float(matrix_add(matrix_add(matrix_add(double(r[0][2])*z,double(r[0][1])*y),double(r[0][0])*x),r[0][3]));
 const float dy=matrix_store_float(matrix_add(matrix_add(matrix_add(double(r[1][0])*x,double(r[1][2])*z),double(r[1][1])*y),r[1][3]));
 const float dz=matrix_store_float(matrix_add(matrix_add(matrix_add(double(r[2][0])*x,double(r[2][2])*z),double(r[2][1])*y),r[2][3]));
 *output=GetRenderCoords();
 output->X=add_coordinate(output->X,rule_integer(dx));
 output->Y=add_coordinate(output->Y,rule_integer(-double(dy)));
 output->Z=add_coordinate(output->Z,rule_integer(dz));
 return add_pixel_offset(output,Type->BuildingAnim[int(BuildingAnimSlot::Turret)].Position);
}

CoordStruct* BuildingClass::vt_entry_300(CoordStruct* output,DWORD weapon) const {
 if(Type->CanBeOccupied&&GetOccupantCount()>0){
  *output=GetRenderCoords();return add_pixel_offset(output,Type->MuzzleFlash[FiringOccupantIndex]);
 }
 if(Type->PrimaryFirePixelOffset!=Point2D{0xFFFF,0xFFFF}){
  *output=GetRenderCoords();return add_pixel_offset(output,Type->PrimaryFirePixelOffset);
 }
 if(Type->BarrelAnimIsVoxel)return GetVoxelFireCoords(output,int(weapon),false);
 TechnoClass::vt_entry_300(output,weapon);
 // OpenTS Turret_Coord / YR 0x00453A70: the VXL turret is drawn at
 // BuildingAnim[Turret].Position, relative to the building render origin.
 // Convert this pixel offset once; rotating it with FLH moves the mount.
 if(Type->TurretAnimIsVoxel)add_pixel_offset(output,Type->BuildingAnim[int(BuildingAnimSlot::Turret)].Position);
 return output;
}
CoordStruct* BuildingClass::GetFLH(CoordStruct* output,int weapon,CoordStruct) const {
 if(Type->CanBeOccupied&&GetOccupantCount()>0)return vt_entry_300(output,weapon);
 // OpenTS Fire_Coord / YR 0x00453840: an explicit pixel
 // position replaces FLH, except PrimaryFireDualOffset adds both in YR.
 if(Type->PrimaryFirePixelOffset!=Point2D{0xFFFF,0xFFFF}){
  if(Type->PrimaryFireDualOffset)TechnoClass::GetFLH(output,weapon,CoordStruct::Empty);
  else *output=GetRenderCoords();
  return add_pixel_offset(output,Type->PrimaryFirePixelOffset);
 }
 if(Type->BarrelAnimIsVoxel)return GetVoxelFireCoords(output,weapon,true);
 TechnoClass::GetFLH(output,weapon,CoordStruct::Empty);
 // YR 0x00453840 also adds the VXL turret's fixed screen
 // placement to the rotated muzzle. GTGCAN uses (3,28); omitting this
 // leaves its shell and muzzle flash above/left of the displayed barrel.
 if(Type->TurretAnimIsVoxel)add_pixel_offset(output,Type->BuildingAnim[int(BuildingAnimSlot::Turret)].Position);
 return output;
}
