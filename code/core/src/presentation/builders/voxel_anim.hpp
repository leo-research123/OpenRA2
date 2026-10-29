#pragma once
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"
class VoxelAnimClass;
namespace game { void build_voxel_anim(PresentationContext&, const VoxelAnimClass&, Point2D*, RectangleStruct*); }
