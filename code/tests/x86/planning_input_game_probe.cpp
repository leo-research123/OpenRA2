// Exercise the outgoing original-game by-value wrappers against the EXE.
#include "yrpp/EventClass.h"
#include "yrpp/Unsorted.h"
extern "C" {
__declspec(dllexport) bool __fastcall OriginalEnqueue(const EventClass* event){return EventClass::AddEvent(EventClass(*event));}
__declspec(dllexport) BYTE __fastcall OriginalSubmit(const EventClass* event){return Game::PlanningManager_Submit(EventClass(*event));}
__declspec(dllexport) bool __fastcall OriginalRingAdd(const EventClass* event){return EventClass::OutList.Add(*event);}
__declspec(dllexport) void __fastcall OriginalReject(const EventClass* event){Game::PlanningManager_RejectEvent(event);}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
}
