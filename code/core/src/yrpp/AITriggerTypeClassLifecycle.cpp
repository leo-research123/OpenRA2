// YRpp 9402d7da; constructor 0041E350, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/AITriggerTypeClass.h"
#include "type_registry.hpp"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/TActionClass.h"
#include <cstring>
#include <new>

namespace { DynamicVectorClass<AITriggerTypeClass*> types; }
DynamicVectorClass<AITriggerTypeClass*>& AITriggerTypeClass::Array = types;
AITriggerTypeClass* YRPP_FASTCALL AITriggerTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL AITriggerTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

AITriggerTypeClass::AITriggerTypeClass(const char* pID)
    : AbstractTypeClass(pID),
      ConditionType{},
      IsGlobal{},
      OwnerHouseType{},
      IsEnabled{},
      HouseIndex{},
      SideIndex{},
      TechLevel{},
      unknown_B4{},
      Weight_Current{},
      Weight_Minimum{},
      Weight_Maximum{},
      IsForSkirmish{},
      IsForBaseDefense{},
      Enabled_Easy{},
      Enabled_Normal{},
      Enabled_Hard{},
      ConditionObject{},
      Team1{},
      Team2{},
      Conditions{},
      TimesExecuted{},
      TimesCompleted{},
      unknown_10C{} {
    ConditionType = static_cast<::AITriggerCondition>(0xffffffffu);
    IsGlobal = 0;
    OwnerHouseType = static_cast<::AITriggerHouseType>(0x0u);
    IsEnabled = false;
    HouseIndex = -1;
    SideIndex = 0;
    TechLevel = 0;
    Weight_Current = 1.0;
    Weight_Minimum = 1.0;
    Weight_Maximum = 1.0;
    IsForSkirmish = false;
    IsForBaseDefense = false;
    Enabled_Easy = true;
    Enabled_Normal = true;
    Enabled_Hard = true;
    ConditionObject = nullptr;
    Team1 = nullptr;
    Team2 = nullptr;
    TimesExecuted = 0;
    TimesCompleted = 0;
    Array.AddItem(this);
}

AITriggerTypeClass::~AITriggerTypeClass() {
    Array.Remove(this);
}
