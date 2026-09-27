// YRpp 9402d7da; constructor 0046BBC0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/BulletTypeClass.h"
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

namespace { DynamicVectorClass<BulletTypeClass*> types; }
DynamicVectorClass<BulletTypeClass*>& BulletTypeClass::Array = types;
BulletTypeClass* YRPP_FASTCALL BulletTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL BulletTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }
BulletTypeClass* YRPP_FASTCALL BulletTypeClass::FindOrAllocate(const char* id) { return game::allocate_type<BulletTypeClass>(id); }

BulletTypeClass::BulletTypeClass(const char* pID)
    : ObjectTypeClass(pID),
      Airburst{},
      Floater{},
      SubjectToCliffs{},
      SubjectToElevation{},
      SubjectToWalls{},
      VeryHigh{},
      Shadow{},
      Arcing{},
      Dropping{},
      Level{},
      Inviso{},
      Proximity{},
      Ranged{},
      NoRotate{},
      Inaccurate{},
      FlakScatter{},
      AA{},
      AG{},
      Degenerates{},
      Bouncy{},
      AnimPalette{},
      FirersPalette{},
      Cluster{},
      AirburstWeapon{},
      ShrapnelWeapon{},
      ShrapnelCount{},
      DetonationAltitude{},
      Vertical{},
      Elasticity{},
      Acceleration{},
      Color{},
      Trailer{},
      ROT{},
      CourseLockDuration{},
      SpawnDelay{},
      ScaledSpawnDelay{},
      Scalable{},
      Arm{},
      AnimLow{},
      AnimHigh{},
      AnimRate{},
      Flat{} {
    ObjectTypeClass::RadarInvisible = true;
    ObjectTypeClass::Selectable = false;
    ObjectTypeClass::LegalTarget = false;
    ObjectTypeClass::Insignificant = true;
    ObjectTypeClass::Immune = true;
    ObjectTypeClass::IsLogic = true;
    ObjectTypeClass::AllowCellContent = false;
    Airburst = false;
    Floater = false;
    SubjectToCliffs = false;
    SubjectToElevation = false;
    SubjectToWalls = false;
    VeryHigh = false;
    Shadow = true;
    Arcing = false;
    Dropping = false;
    Level = false;
    Inviso = false;
    Proximity = false;
    Ranged = false;
    NoRotate = true;
    Inaccurate = false;
    FlakScatter = false;
    AA = false;
    AG = true;
    Degenerates = false;
    Bouncy = false;
    AnimPalette = false;
    FirersPalette = false;
    Cluster = 1;
    AirburstWeapon = nullptr;
    ShrapnelWeapon = nullptr;
    ShrapnelCount = 0;
    DetonationAltitude = 0;
    Vertical = false;
    Elasticity = 0.75;
    Acceleration = 3;
    Color = 0;
    Trailer = nullptr;
    ROT = 0;
    CourseLockDuration = 0;
    SpawnDelay = 3;
    ScaledSpawnDelay = 0;
    Scalable = false;
    Arm = 0;
    AnimLow = 0x0u;
    AnimHigh = 0x0u;
    AnimRate = 0x0u;
    Flat = false;
    Create_ID();
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
}

BulletTypeClass::~BulletTypeClass() {
    NotifyTypeExpired();
    TypeExpirationListeners.Remove(this);
    Array.Remove(this);
}
void BulletTypeClass::PointerExpired(AbstractClass* object, bool removed) {
    (void)removed;
    if (Trailer == object) Trailer = nullptr;
}
