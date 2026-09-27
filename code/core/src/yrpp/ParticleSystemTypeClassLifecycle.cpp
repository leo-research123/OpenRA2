// YRpp 9402d7da; constructor 006440A0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/ParticleSystemTypeClass.h"
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

namespace { DynamicVectorClass<ParticleSystemTypeClass*> types; }
DynamicVectorClass<ParticleSystemTypeClass*>& ParticleSystemTypeClass::Array = types;
ParticleSystemTypeClass* YRPP_FASTCALL ParticleSystemTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL ParticleSystemTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }
ParticleSystemTypeClass* YRPP_FASTCALL ParticleSystemTypeClass::FindOrAllocate(const char* id) { return game::allocate_type<ParticleSystemTypeClass>(id); }

ParticleSystemTypeClass::ParticleSystemTypeClass(const char* pID)
    : ObjectTypeClass(pID),
      HoldsWhat{},
      Spawns{},
      SpawnFrames{},
      Slowdown{},
      ParticleCap{},
      SpawnRadius{},
      SpawnCutoff{},
      SpawnTranslucencyCutoff{},
      BehavesLike{},
      Lifetime{},
      SpawnDirection{},
      ParticlesPerCoord{},
      SpiralDeltaPerCoord{},
      SpiralRadius{},
      PositionPerturbationCoefficient{},
      MovementPerturbationCoefficient{},
      VelocityPerturbationCoefficient{},
      SpawnSparkPercentage{},
      SparkSpawnFrames{},
      LightSize{},
      LaserColor{},
      Laser{},
      OneFrameLight{} {
    ObjectTypeClass::IsLogic = true;
    HoldsWhat = -1;
    Spawns = false;
    SpawnFrames = 1;
    Slowdown = 0.0f;
    ParticleCap = 50;
    SpawnRadius = 0;
    SpawnCutoff = 0.0f;
    SpawnTranslucencyCutoff = 0.0f;
    BehavesLike = static_cast<::BehavesLike>(0xffffffffu);
    Lifetime = -1;
    ParticlesPerCoord = 0.1;
    SpiralDeltaPerCoord = 0.025;
    SpiralRadius = 25.0;
    PositionPerturbationCoefficient = 0.0;
    MovementPerturbationCoefficient = 0.0;
    VelocityPerturbationCoefficient = 0.0;
    SpawnSparkPercentage = 0.0;
    SparkSpawnFrames = 0;
    LightSize = 0;
    LaserColor.R = 0x0u;
    LaserColor.G = 0x0u;
    LaserColor.B = 0x0u;
    Laser = false;
    OneFrameLight = false;
    Create_ID();
    Array.AddItem(this);
}

ParticleSystemTypeClass::~ParticleSystemTypeClass() {
    NotifyTypeExpired();
    Array.Remove(this);
}
