#include "yrpp/MissionClass.h"
namespace {
MissionControlClass missions[32];
const char* const names[32] = {"Sleep", "Attack", "Move", "QMove", "Retreat", "Guard", "Sticky",
    "Enter", "Capture", "Eaten", "Harvest", "Area Guard", "Return", "Stop", "Ambush", "Hunt",
    "Unload", "Sabotage", "Construction", "Selling", "Repair", "Rescue", "Missile", "Harmless",
    "Open", "Patrol", "Paradrop Approach", "Paradrop Overfly", "Wait", "Attack Move",
    "Spyplane Approach", "Spyplane Overfly"};
}
MissionControlClass (&MissionControlClass::Array)[32] = missions;
const char* const (&MissionControlClass::Names)[32] = names;
