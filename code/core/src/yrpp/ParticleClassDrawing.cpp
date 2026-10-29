#include "yrpp/ParticleClass.h"
#include <cstdlib>
#include "particle_shape.hpp"
int ParticleClass::vt_entry_1E8() { return game::particle_shape_number(*this); }
void ParticleClass::DrawIt(Point2D*, RectangleStruct*) const { std::abort(); }
