#pragma once
// Internal ABI handoffs only. There is deliberately no standalone provider.
// Core-owned methods and algorithms never have target-specific definitions here.
#ifdef RA2_IMAGE_GAME
struct TintStruct;
namespace game {
// Scenario state and the shared zero row are still owned/read by unported EXE code.
bool OriginalScenarioTint(TintStruct&);
unsigned char* OriginalZeroZAdjust();
}
#endif
