// YRpp 9402d7da; constructor 0074AD80, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/VoxelAnimTypeClass.h"
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

namespace { DynamicVectorClass<VoxelAnimTypeClass*> types; }
DynamicVectorClass<VoxelAnimTypeClass*>& VoxelAnimTypeClass::Array = types;
VoxelAnimTypeClass* YRPP_FASTCALL VoxelAnimTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL VoxelAnimTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

VoxelAnimTypeClass::VoxelAnimTypeClass(const char* pID)
    : ObjectTypeClass(pID),
      Normalized{},
      Translucent{},
      SourceShared{},
      unused_297{},
      VoxelIndex{},
      Duration{},
      Elasticity{},
      MinAngularVelocity{},
      MaxAngularVelocity{},
      MinZVel{},
      MaxZVel{},
      MaxXYVel{},
      IsMeteor{},
      unused_2D1{},
      Spawns{},
      SpawnCount{},
      StartSound{},
      StopSound{},
      BounceAnim{},
      ExpireAnim{},
      TrailerAnim{},
      Damage{},
      DamageRadius{},
      Warhead{},
      AttachedSystem{},
      IsTiberium{},
      unused_301{} {
    ObjectTypeClass::RadarInvisible = true;
    ObjectTypeClass::Selectable = true;
    ObjectTypeClass::LegalTarget = false;
    ObjectTypeClass::Insignificant = true;
    ObjectTypeClass::Immune = true;
    ObjectTypeClass::IsLogic = true;
    ObjectTypeClass::AllowCellContent = false;
    Normalized = false;
    Translucent = false;
    SourceShared = false;
    VoxelIndex = 0;
    Duration = 30;
    Elasticity = 0.800000011920929;
    MinAngularVelocity = 0.0;
    MaxAngularVelocity = 0.17452777777777778;
    MinZVel = 3.5;
    MaxZVel = 5.0;
    MaxXYVel = 15.0;
    IsMeteor = false;
    Spawns = nullptr;
    SpawnCount = 0;
    StartSound = -1;
    StopSound = -1;
    BounceAnim = nullptr;
    ExpireAnim = nullptr;
    TrailerAnim = nullptr;
    Damage = 0;
    DamageRadius = 0;
    Warhead = nullptr;
    AttachedSystem = nullptr;
    IsTiberium = false;
    Create_ID();
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
}

VoxelAnimTypeClass::~VoxelAnimTypeClass() {
    NotifyTypeExpired();
    TypeExpirationListeners.Remove(this);
    Array.Remove(this);
    if (SourceShared) { MainVoxel.VXL = nullptr; MainVoxel.HVA = nullptr; }
}
void VoxelAnimTypeClass::PointerExpired(AbstractClass* object, bool removed) {
    (void)removed;
    if (Warhead == object) Warhead = nullptr;
    if (BounceAnim == object) BounceAnim = nullptr;
    if (ExpireAnim == object) ExpireAnim = nullptr;
    if (TrailerAnim == object) TrailerAnim = nullptr;
    if (Spawns == object) Spawns = nullptr;
    if (AttachedSystem == object) AttachedSystem = nullptr;
}
