// Original Execute timer cases, 0x006DE428 / 0x006DE5DA / 0x006DE67B.
#include "yrpp/TActionClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/StringTable.h"
#include <bit>
#include <cstring>
#include <cstdlib>
namespace {
wchar_t empty_timer_text[] = L"";
}
bool TActionClass::MissionTimerStart(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    auto& timer = ScenarioClass::Instance->MissionTimer;
    if (!timer.IsTicking()) {
        timer.Resume();
        TabClass::Instance.RefreshMissionTimer();
    }
    return true;
}
bool TActionClass::MissionTimerStartWithText(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    auto& scenario = *ScenarioClass::Instance;
    scenario.MissionTimer.Start(std::bit_cast<int>(15u * unsigned(Value)));
    scenario.MissionTimer.Resume();
    scenario.MissionTimerTextCSF = empty_timer_text;
    std::memset(scenario.MissionTimerText, 0, sizeof(scenario.MissionTimerText));
    TabClass::Instance.RefreshMissionTimer();
    return true;
}
bool TActionClass::MissionTimerSetText(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    try {
        auto& scenario = *ScenarioClass::Instance;
        scenario.MissionTimerTextCSF =
            Text[0] ? const_cast<wchar_t*>(StringTable::LoadString(Text, nullptr, "D:\\ra2mdpost\\TAction.CPP", 2242))
                    : empty_timer_text;
        std::strncpy(scenario.MissionTimerText, Text, sizeof(scenario.MissionTimerText) - 1);
        scenario.MissionTimerText[sizeof(scenario.MissionTimerText) - 1] = 0;
        TabClass::Instance.RefreshMissionTimer();
        return true;
    } catch (...) {
        std::abort();
    }
}
