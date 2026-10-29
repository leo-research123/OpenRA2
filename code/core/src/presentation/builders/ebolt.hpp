#pragma once
#include "presentation/context.hpp"
class EBolt;
namespace game {
// The original EBolt::DrawAll (YR 0x004C2830) with DrawSingle (0x004C2790) and Draw (0x004C1F20).
void build_ebolts(PresentationContext&);
}
