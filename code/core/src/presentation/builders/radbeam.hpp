#pragma once
#include "presentation/context.hpp"
class RadBeam;
namespace game {
// `user` is the PresentationContext the segment is submitted through.
void draw_radbeam_segment(void* user, const RadBeam&);
void build_radbeams(PresentationContext&);
}
