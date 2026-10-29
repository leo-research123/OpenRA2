#pragma once
#include "yrpp/RadBeam.h"
namespace game {
// The working record follows the original beam state machine. A null sink performs the same state
// transition without submitting pixels. Presentation runs it on a local copy of the original record.
using RadBeamSegmentSink = void (*)(void*, const RadBeam&);
bool advance_radbeam(RadBeam&, RadBeamSegmentSink = nullptr, void* = nullptr);
void radbeam_before_draw();
void radbeam_after_draw();
}
