#include "yrpp/SuperClass.h"
namespace {
bool lightning_active = false;
PsychicDominatorStatus dominator = PsychicDominatorStatus::Inactive;
NukeFlashStatus nuke = NukeFlashStatus::Inactive;
int chrono = 0;
}
bool& LightningStorm::Active = lightning_active;
PsychicDominatorStatus& PsyDom::Status = dominator;
NukeFlashStatus& NukeFlash::Status = nuke;
int& ChronoScreenEffect::Status = chrono;
