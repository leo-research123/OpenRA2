#pragma once
// BuildingClass::GetVoxelBarrelOffsetMatrix (YR 0x00458810) only reads the building, but the original declaration is not
// const. The computation lives here so that the simulation method and the presentation (the voxel barrel draw) share it
// without a const_cast.
class BuildingClass;
struct Matrix3D;
namespace game {
Matrix3D* building_barrel_offset_matrix(const BuildingClass&, Matrix3D& output);
}
