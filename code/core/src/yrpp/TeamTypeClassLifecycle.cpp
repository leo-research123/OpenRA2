// YRpp 9402d7da; constructor 006F06E0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/TeamTypeClass.h"
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

namespace { DynamicVectorClass<TeamTypeClass*> types; }
DynamicVectorClass<TeamTypeClass*>& TeamTypeClass::Array = types;
TeamTypeClass* YRPP_FASTCALL TeamTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL TeamTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

TeamTypeClass::TeamTypeClass(const char* pID)
    : AbstractTypeClass(pID),
      ArrayIndex{},
      Group{},
      VeteranLevel{},
      Loadable{},
      Full{},
      Annoyance{},
      GuardSlower{},
      Recruiter{},
      Autocreate{},
      Prebuild{},
      Reinforce{},
      Whiner{},
      Aggressive{},
      LooseRecruit{},
      Suicide{},
      Droppod{},
      UseTransportOrigin{},
      DropshipLoadout{},
      OnTransOnly{},
      Priority{},
      Max{},
      field_BC{},
      MindControlDecision{},
      Owner{},
      idxHouse{},
      TechLevel{},
      Tag{},
      Waypoint{},
      TransportWaypoint{},
      cntInstances{},
      ScriptType{},
      TaskForce{},
      IsGlobal{},
      field_EC{},
      field_F0{},
      field_F1{},
      AvoidThreats{},
      IonImmune{},
      TransportsReturnOnUnload{},
      AreTeamMembersRecruitable{},
      IsBaseDefense{},
      OnlyTargetHouseEnemy{} {
    ArrayIndex = -1;
    Group = -1;
    VeteranLevel = 1;
    Loadable = false;
    Full = false;
    Annoyance = false;
    GuardSlower = false;
    Recruiter = false;
    Autocreate = false;
    Prebuild = false;
    Reinforce = false;
    Whiner = false;
    Aggressive = false;
    LooseRecruit = false;
    Suicide = false;
    Droppod = false;
    UseTransportOrigin = false;
    DropshipLoadout = false;
    OnTransOnly = false;
    Priority = 7;
    Max = -1;
    field_BC = 0;
    MindControlDecision = 0;
    Owner = nullptr;
    idxHouse = -1;
    TechLevel = 0;
    Tag = nullptr;
    Waypoint = -1;
    TransportWaypoint = -1;
    cntInstances = 0;
    ScriptType = nullptr;
    TaskForce = nullptr;
    IsGlobal = 0;
    field_EC = 9;
    field_F0 = true;
    field_F1 = false;
    AvoidThreats = false;
    IonImmune = false;
    TransportsReturnOnUnload = false;
    AreTeamMembersRecruitable = true;
    IsBaseDefense = false;
    OnlyTargetHouseEnemy = false;
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
    TypeExpirationListeners.AddItem(this);
}

TeamTypeClass::~TeamTypeClass() {
    NotifyTypeExpired();
    Array.Remove(this);
    TypeExpirationListeners.Remove(this);
}
int TeamTypeClass::GetArrayIndex() const { return ArrayIndex; }
void TeamTypeClass::PointerExpired(AbstractClass* object, bool removed) {
    (void)removed;
    if (reinterpret_cast<AbstractClass*>(Tag) == object) Tag = nullptr;
    if (reinterpret_cast<AbstractClass*>(Owner) == object) Owner = nullptr;
    if (TaskForce == object) TaskForce = nullptr;
    if (ScriptType == object) ScriptType = nullptr;
    if (ScriptType) ScriptType->PointerExpired(object, removed);
}
