// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 partsys.cpp::Draw_It.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x62E280; EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/SpotlightClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>

void ParticleSystemClass::DrawIt(Point2D*,RectangleStruct*) const {
 if(Type&&Type->OneFrameLight&&Type->LightSize>0&&Particles.Count>0){
  const double fullness=std::clamp(double(Particles.Count)/Type->ParticleCap,0.4,1.0);
  SpotlightClass light(Location,rule_integer(fullness*Type->LightSize));
  light.MovementRadius=SpotlightRadius;light.Draw();
 }
}
