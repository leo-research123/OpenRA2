#include "yrpp/Fundamentals.h"
#include "yrpp/Unsorted.h"
namespace { int current_frame = 0; int scenario_init = 0; bool scenario_started = false; }
int& Unsorted::CurrentFrame = current_frame;

int& Unsorted::ScenarioInit = scenario_init;
bool& Unsorted::ScenarioStarted = scenario_started;
