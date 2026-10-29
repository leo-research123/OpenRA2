#include "yrpp/SuperClass.h"
namespace {
bool lightning_active = false;
DynamicVectorClass<AnimClass*> clouds, manifesting, bolts;
CellStruct lightning_coords{0, 0};
HouseClass* lightning_owner = nullptr;
int lightning_deferment = 0, lightning_duration = -1, lightning_start = -1;
bool lightning_end = false;
PsychicDominatorStatus dominator = PsychicDominatorStatus::Inactive;
NukeFlashStatus nuke = NukeFlashStatus::Inactive;
CellStruct dominator_coords{-1,-1};
HouseClass* dominator_owner{};
AnimClass* dominator_anim{};
int nuke_start = -1, nuke_duration = -1;
int chrono = 0;
int chrono_duration = 0;
}
bool& LightningStorm::Active = lightning_active;
PsychicDominatorStatus& PsyDom::Status = dominator;
NukeFlashStatus& NukeFlash::Status = nuke;
int& ChronoScreenEffect::Status = chrono;

int& ChronoScreenEffect::Duration = chrono_duration;

CellStruct& LightningStorm::Coords = lightning_coords;
HouseClass*& LightningStorm::Owner = lightning_owner;
int& LightningStorm::Deferment = lightning_deferment;
int& LightningStorm::Duration = lightning_duration;
int& LightningStorm::StartTime = lightning_start;
bool& LightningStorm::TimeToEnd = lightning_end;

DynamicVectorClass<AnimClass*>& LightningStorm::CloudsPresent = clouds;
DynamicVectorClass<AnimClass*>& LightningStorm::CloudsManifesting = manifesting;
DynamicVectorClass<AnimClass*>& LightningStorm::BoltsPresent = bolts;

CellStruct& PsyDom::Coords = dominator_coords;
HouseClass*& PsyDom::Owner = dominator_owner;
AnimClass*& PsyDom::Anim = dominator_anim;
int& NukeFlash::StartTime = nuke_start;
int& NukeFlash::Duration = nuke_duration;
