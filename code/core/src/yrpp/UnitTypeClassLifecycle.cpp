// YRpp 9402d7da; constructor 007470D0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/UnitTypeClass.h"
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

namespace { DynamicVectorClass<UnitTypeClass*> types; }
DynamicVectorClass<UnitTypeClass*>& UnitTypeClass::Array = types;
UnitTypeClass* YRPP_FASTCALL UnitTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL UnitTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

UnitTypeClass::UnitTypeClass(const char* pID)
    : TechnoTypeClass(pID, static_cast<::SpeedType>(-1)),
      ArrayIndex{},
      MovementRestrictedTo{},
      HalfDamageSmokeLocation{},
      Passive{},
      CrateGoodie{},
      Harvester{},
      Weeder{},
      unknown_E10{},
      HasTurret{},
      DeployToFire{},
      IsSimpleDeployer{},
      IsTilter{},
      UseTurretShadow{},
      TooBigToFitUnderBridge{},
      CanBeach{},
      SmallVisceroid{},
      LargeVisceroid{},
      CarriesCrate{},
      NonVehicle{},
      StandingFrames{},
      DeathFrames{},
      DeathFrameRate{},
      StartStandFrame{},
      StartWalkFrame{},
      StartFiringFrame{},
      StartDeathFrame{},
      MaxDeathCounter{},
      Facings{},
      FiringSyncFrame0{},
      FiringSyncFrame1{},
      BurstDelay0{},
      BurstDelay1{},
      BurstDelay2{},
      BurstDelay3{},
      AltImage{},
      WalkFrames{},
      FiringFrames{},
      AltImageFile{} {
    TechnoTypeClass::RotCount = 32;
    TechnoTypeClass::Bunkerable = true;
    TechnoTypeClass::ImmuneToPsionics = false;
    TechnoTypeClass::ImmuneToPsionicWeapons = false;
    TechnoTypeClass::Parasiteable = true;
    TechnoTypeClass::ImmuneToPoison = false;
    TechnoTypeClass::ConsideredAircraft = false;
    TechnoTypeClass::Organic = false;
    ArrayIndex = -1;
    MovementRestrictedTo = static_cast<::LandType>(0xffffffffu);
    HalfDamageSmokeLocation.X = 0;
    HalfDamageSmokeLocation.Y = 0;
    HalfDamageSmokeLocation.Z = 0;
    Passive = false;
    CrateGoodie = false;
    Harvester = false;
    Weeder = false;
    unknown_E10 = false;
    HasTurret = false;
    DeployToFire = false;
    IsSimpleDeployer = false;
    IsTilter = true;
    UseTurretShadow = false;
    TooBigToFitUnderBridge = false;
    CanBeach = false;
    SmallVisceroid = false;
    LargeVisceroid = false;
    CarriesCrate = false;
    NonVehicle = false;
    StandingFrames = 0;
    DeathFrames = 0;
    DeathFrameRate = 1;
    StartStandFrame = -1;
    StartWalkFrame = -1;
    StartFiringFrame = -1;
    StartDeathFrame = -1;
    MaxDeathCounter = -1;
    Facings = 8;
    FiringSyncFrame0 = -1;
    FiringSyncFrame1 = -1;
    BurstDelay0 = -1;
    BurstDelay1 = -1;
    BurstDelay2 = -1;
    BurstDelay3 = -1;
    AltImage = nullptr;
    WalkFrames = static_cast<char>(12);
    FiringFrames = static_cast<char>(0);
    AltImageFile[0] = static_cast<char>(0);
    Create_ID();
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
}

UnitTypeClass::~UnitTypeClass() {
    NotifyTypeExpired();
    Array.Remove(this);
}
int UnitTypeClass::GetArrayIndex() const { return ArrayIndex; }
