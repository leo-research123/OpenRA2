// YRpp 9402d7da; constructor 006CE5B0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/SuperWeaponTypeClass.h"
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

namespace { DynamicVectorClass<SuperWeaponTypeClass*> types; }
DynamicVectorClass<SuperWeaponTypeClass*>& SuperWeaponTypeClass::Array = types;
SuperWeaponTypeClass* YRPP_FASTCALL SuperWeaponTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL SuperWeaponTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

SuperWeaponTypeClass::SuperWeaponTypeClass(const char* pID)
    : AbstractTypeClass(pID),
      ArrayIndex{},
      WeaponType{},
      RechargeVoice{},
      ChargingVoice{},
      ImpatientVoice{},
      SuspendVoice{},
      RechargeTime{},
      Type{},
      SidebarImage{},
      Action{},
      SpecialSound{},
      StartSound{},
      AuxBuilding{},
      SidebarImageFile{},
      zero_E4{},
      UseChargeDrain{},
      IsPowered{},
      DisableableFromShell{},
      FlashSidebarTabFrames{},
      AIDefendAgainst{},
      PreClick{},
      PostClick{},
      PreDependent{},
      ShowTimer{},
      ManualControl{},
      Range{},
      LineMultiplier{} {
    WeaponType = nullptr;
    RechargeVoice = -1;
    ChargingVoice = -1;
    ImpatientVoice = -1;
    SuspendVoice = -1;
    RechargeTime = 4500;
    Type = static_cast<::SuperWeaponType>(0xffffffffu);
    SidebarImage = nullptr;
    Action = static_cast<::Action>(0x0u);
    SpecialSound = -1;
    StartSound = -1;
    AuxBuilding = nullptr;
    SidebarImageFile[0] = static_cast<char>(0);
    UseChargeDrain = false;
    IsPowered = true;
    DisableableFromShell = false;
    FlashSidebarTabFrames = -1;
    AIDefendAgainst = false;
    PreClick = false;
    PostClick = false;
    PreDependent = -1;
    ShowTimer = false;
    ManualControl = false;
    Range = 0.0f;
    LineMultiplier = 0;
    std::strncpy(SidebarImageFile, ID, sizeof(SidebarImageFile));
    zero_E4 = 0;
    Create_ID();
    ArrayIndex = Array.Count;
    Array.AddItem(this);
}

SuperWeaponTypeClass::~SuperWeaponTypeClass() {
    Array.Remove(this);
}
int SuperWeaponTypeClass::GetArrayIndex() const { return ArrayIndex; }
