// Original Detail module in FPSCounter.h. 0x0055AF40 publishes the minimum
// effects frame rate used by radar movie entry/exit and post-load repair.
#include "yrpp/FPSCounter.h"
#if !defined(RA2_YRPP_GAME)
namespace {unsigned minimum_frame_rate=15;}
unsigned& Detail::MinFrameRate=minimum_frame_rate;
#endif
void Detail::SetMinFrameRate(unsigned value) noexcept {MinFrameRate=value;}
