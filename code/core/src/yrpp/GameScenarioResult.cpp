#include "yrpp/Unsorted.h"
#include "yrpp/ScenarioClass.h"

int Game::TakeScenarioResult() {
    const int result = PlayerWins ? 1 : PlayerLoses ? 2 : 0;
    if (!result) return 0;
    PlayerWins = PlayerLoses = false;
    if (ScenarioClass::Instance) ScenarioClass::Instance->ElapsedTimer.Pause();
    Unsorted::ScenarioStarted = false;
    Unsorted::TacticalActive = false;
    return result;
}
