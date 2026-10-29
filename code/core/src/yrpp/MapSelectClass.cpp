// MapSel control flow, YR 0x005AE100 and 0x005ADF20.
// Native choices provide resource data; no original GUI object is constructed.
#include "yrpp/ScenarioClass.h"
#include "map_selection.hpp"
#include <cstring>

#if !defined(RA2_YRPP_GAME)
namespace {
struct Choices {
    game::MapChoices* value = nullptr;
    ~Choices() { game::close_map_choices(value); }
};
bool equal_filename(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a || *b) {
        const auto upper = [](unsigned char c) { return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c; };
        if (upper(*a++) != upper(*b++)) return false;
    }
    return true;
}
}
bool MapSelectClass::SetNextScenario(ScenarioClass* scenario) {
    Choices choices;
    if (!scenario || !game::open_map_choices(*scenario, choices.value)) return false;
    const auto& data = *choices.value;
    const int current = game::find_map_choice(data, scenario->FileName);
    if (current < 0) return false;
    ScenarioClass::WasGameSaved = false;
    // 0x7B47F0 compares MapVQ's internal pointer with nullptr, not its length.
    // MSChoice's 0x7B48D0 assignment allocates an empty string even when the
    // INI key is absent, so every successfully loaded node passes this gate.
    const int next = game::map_choice_link(data, current, 0);
    const char* name = game::map_choice_filename(data, next);
    if (!name || !*name) return false;  // Malformed original data is rejected by the host.
    scenario->SetFileName(name);
    scenario->Stage = static_cast<short>(next);
    return true;
}
const char* YRPP_FASTCALL MapSelectClass::SelectNextScenario(ScenarioClass* scenario) {
    MapSelectClass selection;
    return selection.SetNextScenario(scenario) && scenario->FileName[0] ? scenario->FileName : nullptr;
}
const char* YRPP_FASTCALL MapSelectClass::SelectScenario(ScenarioClass* scenario, const char* filename) {
    if (!scenario || !filename) return nullptr;
    Choices choices;
    if (!game::open_map_choices(*scenario, choices.value)) return nullptr;
    const auto& data = *choices.value;
    const unsigned stage = static_cast<unsigned short>(scenario->Stage);
    if (stage >= static_cast<unsigned>(game::map_choice_count(data))) return nullptr;
    scenario->SetFileName("");
    for (int i = 0; i < game::map_choice_link_count(data, stage); ++i) {
        const int next = game::map_choice_link(data, stage, i);
        const char* name = game::map_choice_filename(data, next);
        if (equal_filename(name, filename)) {
            scenario->SetFileName(name);
            scenario->Stage = static_cast<short>(next);
            break;
        }
    }
    return scenario->FileName[0] ? filename : nullptr;
}
#endif
