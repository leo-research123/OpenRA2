// Campaign selection portion of Scenario victory, YR 0x0068599E..0x00685AB0.
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/EvadeClass.h"
#include <cstring>

#if !defined(RA2_YRPP_GAME)
const char* ScenarioClass::SetFileName(const char* filename) {
    if (!filename) return nullptr;
    std::strncpy(FileName, filename, sizeof(FileName));
    FileName[sizeof(FileName) - 1] = 0;
    return FileName;
}
#endif
bool ScenarioClass::PrepareNextScenario() {
    if (SessionClass::Instance.GameMode != GameMode::Campaign || OneTimeOnly || EndOfGame) return false;
    if (SkipMapSelect)
        return MapSelectClass::SelectScenario(this, GlobalVariables[1].Value ? AltNextScenario : NextScenario) != nullptr;
    return MapSelectClass::SelectNextScenario(this) != nullptr;
}
