// YRpp 9402d7da; constructor 0041C8B0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/AircraftTypeClass.h"
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

namespace { DynamicVectorClass<AircraftTypeClass*> types; }
DynamicVectorClass<AircraftTypeClass*>& AircraftTypeClass::Array = types;
AircraftTypeClass* YRPP_FASTCALL AircraftTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL AircraftTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

AircraftTypeClass::AircraftTypeClass(const char* pID)
    : TechnoTypeClass(pID, static_cast<::SpeedType>(4)),
      ArrayIndex{},
      Carryall{},
      Trailer{},
      SpawnDelay{},
      Rotors{},
      CustomRotor{},
      Landable{},
      FlyBy{},
      FlyBack{},
      AirportBound{},
      Fighter{} {
    TechnoTypeClass::RotCount = 32;
    TechnoTypeClass::MoveToShroud = false;
    TechnoTypeClass::Bunkerable = false;
    TechnoTypeClass::ImmuneToPsionics = false;
    TechnoTypeClass::ImmuneToPsionicWeapons = false;
    TechnoTypeClass::Parasiteable = true;
    TechnoTypeClass::ImmuneToPoison = false;
    TechnoTypeClass::ConsideredAircraft = true;
    TechnoTypeClass::Organic = false;
    ArrayIndex = -1;
    Carryall = false;
    Trailer = nullptr;
    SpawnDelay = 3;
    Rotors = false;
    CustomRotor = false;
    Landable = false;
    FlyBy = false;
    FlyBack = false;
    AirportBound = false;
    Fighter = false;
    Create_ID();
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
}

AircraftTypeClass::~AircraftTypeClass() {
    NotifyTypeExpired();
    Array.Remove(this);
}
int AircraftTypeClass::GetArrayIndex() const { return ArrayIndex; }
