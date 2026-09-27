// YRpp 9402d7da; constructor 0075CEC0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/WarheadTypeClass.h"
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

namespace { DynamicVectorClass<WarheadTypeClass*> types; }
DynamicVectorClass<WarheadTypeClass*>& WarheadTypeClass::Array = types;
WarheadTypeClass* YRPP_FASTCALL WarheadTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL WarheadTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }
WarheadTypeClass* YRPP_FASTCALL WarheadTypeClass::FindOrAllocate(const char* id) { return game::allocate_type<WarheadTypeClass>(id); }

WarheadTypeClass::WarheadTypeClass(const char* pID)
    : AbstractTypeClass(pID),
      Deform{},
      Verses{},
      ProneDamage{},
      DeformTreshold{},
      AnimList{},
      InfDeath{},
      CellSpread{},
      CellInset{},
      PercentAtMax{},
      CausesDelayKill{},
      DelayKillFrames{},
      DelayKillAtMax{},
      CombatLightSize{},
      Particle{},
      Wall{},
      WallAbsoluteDestroyer{},
      PenetratesBunker{},
      Wood{},
      Tiberium{},
      unknown_bool_149{},
      Sparky{},
      Sonic{},
      Fire{},
      Conventional{},
      Rocker{},
      DirectRocker{},
      Bright{},
      CLDisableRed{},
      CLDisableGreen{},
      CLDisableBlue{},
      EMEffect{},
      MindControl{},
      Poison{},
      IvanBomb{},
      ElectricAssault{},
      Parasite{},
      Temporal{},
      IsLocomotor{},
      Locomotor{},
      Airstrike{},
      Psychedelic{},
      BombDisarm{},
      Paralyzes{},
      Culling{},
      MakesDisguise{},
      NukeMaker{},
      Radiation{},
      PsychicDamage{},
      AffectsAllies{},
      Bullets{},
      Veinhole{},
      ShakeXlo{},
      ShakeXhi{},
      ShakeYlo{},
      ShakeYhi{},
      DebrisTypes{},
      DebrisMaximums{},
      MaxDebris{},
      MinDebris{},
      unused_1CC{} {
    Deform = 0.0;
    Verses[0] = 1.0;
    ProneDamage = 1.0;
    DeformTreshold = 0;
    InfDeath = static_cast<::InfDeath>(0x0u);
    CellSpread = 0.0f;
    CellInset = 0.0f;
    PercentAtMax = 1.0f;
    CausesDelayKill = false;
    DelayKillFrames = 5;
    DelayKillAtMax = 1.0f;
    CombatLightSize = 0.0f;
    Particle = nullptr;
    Wall = false;
    WallAbsoluteDestroyer = false;
    PenetratesBunker = false;
    Wood = false;
    Tiberium = false;
    unknown_bool_149 = false;
    Sparky = false;
    Sonic = false;
    Fire = false;
    Conventional = false;
    Rocker = false;
    DirectRocker = false;
    Bright = false;
    CLDisableRed = false;
    CLDisableGreen = false;
    CLDisableBlue = false;
    EMEffect = false;
    MindControl = false;
    Poison = false;
    IvanBomb = false;
    ElectricAssault = false;
    Parasite = false;
    Temporal = false;
    IsLocomotor = false;
    Locomotor = {0x4a582747u, 0x9839, 0x11d1, {0xb7, 0x09, 0x00, 0xa0, 0x24, 0xdd, 0xaf, 0xd1}};
    Airstrike = false;
    Psychedelic = false;
    BombDisarm = false;
    Paralyzes = 0;
    Culling = false;
    MakesDisguise = false;
    NukeMaker = false;
    Radiation = false;
    PsychicDamage = false;
    AffectsAllies = true;
    Bullets = false;
    Veinhole = false;
    ShakeXlo = 0;
    ShakeXhi = 0;
    ShakeYlo = 0;
    ShakeYhi = 0;
    MaxDebris = 0;
    MinDebris = 0;
    for (double& verse : Verses) verse = 1.0;
    Create_ID();
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
}

WarheadTypeClass::~WarheadTypeClass() {
    NotifyTypeExpired();
    Array.Remove(this);
    TypeExpirationListeners.Remove(this);
}
void WarheadTypeClass::PointerExpired(AbstractClass* object, bool removed) {
    (void)removed;
    if (Particle == object) Particle = nullptr;
    for (int i = 0; i < AnimList.Count; ++i) if (AnimList[i] == object) { AnimList.RemoveItem(i); break; }
}
