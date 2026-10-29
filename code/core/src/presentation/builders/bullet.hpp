#pragma once
#include "presentation/context.hpp"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"
class BulletClass;
namespace game { void build_bullet(PresentationContext&, const BulletClass&, Point2D*, RectangleStruct*); }
