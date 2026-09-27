// YRpp 9402d7da; constructor 005236A0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/InfantryTypeClass.h"
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

namespace { DynamicVectorClass<InfantryTypeClass*> types; }
DynamicVectorClass<InfantryTypeClass*>& InfantryTypeClass::Array = types;
InfantryTypeClass* YRPP_FASTCALL InfantryTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL InfantryTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }
InfantryTypeClass* YRPP_FASTCALL InfantryTypeClass::FindOrAllocate(const char* id) { return game::allocate_type<InfantryTypeClass>(id); }

InfantryTypeClass::InfantryTypeClass(const char* pID)
    : TechnoTypeClass(pID, static_cast<::SpeedType>(0)),
      ArrayIndex{},
      Pip{},
      OccupyPip{},
      OccupyWeapon{},
      EliteOccupyWeapon{},
      Sequence{},
      FireUp{},
      FireProne{},
      SecondaryFire{},
      SecondaryProne{},
      DeadBodies{},
      DeathAnims{},
      VoiceComment{},
      EnterWaterSound{},
      LeaveWaterSound{},
      Cyborg{},
      NotHuman{},
      Ivan{},
      DirectionDistance{},
      Occupier{},
      Assaulter{},
      HarvestRate{},
      Fearless{},
      Crawls{},
      Infiltrate{},
      Fraidycat{},
      TiberiumProof{},
      Civilian{},
      C4{},
      Engineer{},
      Agent{},
      Thief{},
      VehicleThief{},
      Doggie{},
      Deployer{},
      DeployedCrushable{},
      UseOwnName{},
      JumpJetTurn{},
      align_ECC{} {
    ObjectTypeClass::Crushable = true;
    TechnoTypeClass::RotCount = 8;
    TechnoTypeClass::RadarVisible = false;
    TechnoTypeClass::Repairable = false;
    TechnoTypeClass::Crewed = false;
    TechnoTypeClass::Bunkerable = false;
    TechnoTypeClass::ImmuneToPsionics = false;
    TechnoTypeClass::ImmuneToPsionicWeapons = false;
    TechnoTypeClass::Parasiteable = true;
    TechnoTypeClass::ImmuneToPoison = false;
    TechnoTypeClass::ConsideredAircraft = false;
    TechnoTypeClass::Organic = true;
    ArrayIndex = -1;
    Pip = static_cast<::PipIndex>(0x1u);
    OccupyPip = static_cast<::PipIndex>(0x7u);
    OccupyWeapon.WeaponType = nullptr;
    OccupyWeapon.FLH.X = 0;
    OccupyWeapon.FLH.Y = 0;
    OccupyWeapon.FLH.Z = 0;
    OccupyWeapon.BarrelLength = 0;
    OccupyWeapon.BarrelThickness = 0;
    OccupyWeapon.TurretLocked = false;
    EliteOccupyWeapon.WeaponType = nullptr;
    EliteOccupyWeapon.FLH.X = 0;
    EliteOccupyWeapon.FLH.Y = 0;
    EliteOccupyWeapon.FLH.Z = 0;
    EliteOccupyWeapon.BarrelLength = 0;
    EliteOccupyWeapon.BarrelThickness = 0;
    EliteOccupyWeapon.TurretLocked = false;
    Sequence = nullptr;
    FireUp = 0;
    FireProne = 0;
    SecondaryFire = 0;
    SecondaryProne = 0;
    EnterWaterSound = -1;
    LeaveWaterSound = -1;
    Cyborg = false;
    NotHuman = false;
    Ivan = false;
    DirectionDistance = 0;
    Occupier = false;
    Assaulter = false;
    HarvestRate = 1;
    Fearless = false;
    Crawls = true;
    Infiltrate = false;
    Fraidycat = false;
    TiberiumProof = false;
    Civilian = false;
    C4 = false;
    Engineer = false;
    Agent = false;
    Thief = false;
    VehicleThief = false;
    Doggie = false;
    Deployer = false;
    DeployedCrushable = true;
    UseOwnName = false;
    JumpJetTurn = false;
    Sequence = static_cast<SequenceStruct*>(YRMemory::Allocate(sizeof(SequenceStruct)));
    if (!Sequence) throw std::bad_alloc();
    new (Sequence) SequenceStruct{};
    for (auto& sequence : Sequence->Sequences) sequence.Facing = static_cast<::SequenceFacing>(-1);
    Create_ID();
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
}

InfantryTypeClass::~InfantryTypeClass() {
    NotifyTypeExpired();
    Array.Remove(this);
    YRMemory::Deallocate(Sequence);
    Sequence = nullptr;
}
int InfantryTypeClass::GetArrayIndex() const { return ArrayIndex; }
