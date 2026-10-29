#pragma once
#include "yrpp/Matrix3D.h"
#include "yrpp/Drawing.h"
class DriveLocomotionClass; class ShipLocomotionClass; class HoverLocomotionClass; class ILocomotion; class TechnoTypeClass;
namespace game {
Matrix3D drive_drawing_matrix(DriveLocomotionClass&, VoxelIndexKey*, const TechnoTypeClass&);
Matrix3D ship_drawing_matrix(ShipLocomotionClass&, VoxelIndexKey*, const TechnoTypeClass&);
Matrix3D hover_drawing_matrix(HoverLocomotionClass&, VoxelIndexKey*, const TechnoTypeClass&);
Matrix3D locomotor_drawing_matrix(ILocomotion*, VoxelIndexKey*, const TechnoTypeClass&);
}
