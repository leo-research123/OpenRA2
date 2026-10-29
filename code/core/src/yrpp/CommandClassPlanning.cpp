// Original PlanningMode command. Native and original-game calls share metadata.
#include "yrpp/CommandClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/StringTable.h"
#include "yrpp/BeaconManagerClass.h"
const char* PlanningModeCommandClass::GetName() const { return "PlanningMode"; }
const wchar_t* PlanningModeCommandClass::GetUIName() const { return StringTable::LoadString("Cmnd:PlanningMode"); }
const wchar_t* PlanningModeCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_CONTROL"); }
const wchar_t* PlanningModeCommandClass::GetUIDescription() const {
    return StringTable::LoadString("Cmnd:PlanningModeDesc");
}
bool PlanningModeCommandClass::PreventCombinationOverride(WWKey input) const { return (int(input) & 0x700) != 0; }
bool PlanningModeCommandClass::ExtraTriggerCondition(WWKey) const { return true; }
bool PlanningModeCommandClass::CheckLoop55E020(WWKey input) const { return !(int(input) & 0x800); }
void PlanningModeCommandClass::Execute(WWKey input) const {
    if (int(input) & 0x800)
        Game::EndPlanning(false);
    else
        Game::BeginPlanning(false);
}

const char* DeleteCommandClass::GetName() const { return "Delete"; }
const wchar_t* DeleteCommandClass::GetUIName() const { return StringTable::LoadString("Cmnd:Delete"); }
const wchar_t* DeleteCommandClass::GetUICategory() const { return StringTable::LoadString("TXT_INTERFACE"); }
const wchar_t* DeleteCommandClass::GetUIDescription() const { return StringTable::LoadString("Cmnd:DeleteDesc"); }
void DeleteCommandClass::Execute(WWKey) const {
#if defined(RA2_YRPP_GAME)
    reinterpret_cast<void(YRPP_CDECL*)()>(0x731A10)();
#else
    if (Game::PlanningNodeAtAC4BF0)
        Game::PlanningManager_SubmitDelete(Game::PlanningNodeAtAC4BF0);
    else
        BeaconManagerClass::Instance.DeleteBeacon(-1, -1);
#endif
}
