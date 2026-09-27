// YR PlanMgr input boundaries 0x00637DD0 / 0x00639FD0. The fixed OpenTS
// 44fac744 baseline has no planning module. Use the original event model,
// shared PlanMgr error flag and common out-list, not a second pending queue.
#include "yrpp/Unsorted.h"
#include "yrpp/EventClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/VocClass.h"
#if defined(RA2_YRPP_GAME)
__declspec(noinline) BYTE YRPP_STDCALL Game::PlanningManager_Submit(EventClass event) { JMP_STD(0x637DD0); }
#else
BYTE YRPP_STDCALL Game::PlanningManager_Submit(EventClass event) noexcept {
    if(event.Type!=EventType::MegaMission)return static_cast<BYTE>(event.Type);
    event.MegaMission.IsPlanningEvent=true;
    return EventClass::AddEvent(EventClass(event));
}
void YRPP_FASTCALL Game::PlanningManager_RejectEvent(const EventClass* event) noexcept {
    const char* label=nullptr;
    switch(event->Type) {
        case EventType::Deploy:label="MSG:PlanningModeNoDeploy";break;
        case EventType::Idle:label="MSG:PlanningModeNoStop";break;
        case EventType::Scatter:label="MSG:PlanningModeNoScatter";break;
        default:break;
    }
    if(label && !PlanningErrorReported) {
        PlanningErrorReported=true;
        ShowMessage(StringTable::LoadString(label,nullptr,"D:\\ra2mdpost\\PlanMgr.cpp",3233),480);
        VocClass::PlayGlobal(RulesClass::Instance->ScoldSound,0x2000,1.0f,nullptr);
    }
    // Original always plays this final sound, even after the first sound above
    // and even for unrelated event types. Do not deduplicate it as UI feedback.
    VocClass::PlayGlobal(RulesClass::Instance->ScoldSound,0x2000,1.0f,nullptr);
}
#endif
