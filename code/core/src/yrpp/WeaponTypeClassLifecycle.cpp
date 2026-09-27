// YRpp 9402d7da; constructor 00771C70, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/WeaponTypeClass.h"
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

namespace { DynamicVectorClass<WeaponTypeClass*> types; }
DynamicVectorClass<WeaponTypeClass*>& WeaponTypeClass::Array = types;
WeaponTypeClass* YRPP_FASTCALL WeaponTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL WeaponTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }
WeaponTypeClass* YRPP_FASTCALL WeaponTypeClass::FindOrAllocate(const char* id) { return game::allocate_type<WeaponTypeClass>(id); }

WeaponTypeClass::WeaponTypeClass(const char* pID)
    : AbstractTypeClass(pID),
      AmbientDamage{},
      Burst{},
      Projectile{},
      Damage{},
      Speed{},
      Warhead{},
      ROF{},
      Range{},
      MinimumRange{},
      Report{},
      DownReport{},
      Anim{},
      OccupantAnim{},
      AssaultAnim{},
      OpenToppedAnim{},
      AttachedParticleSystem{},
      LaserInnerColor{},
      LaserOuterColor{},
      LaserOuterSpread{},
      UseFireParticles{},
      UseSparkParticles{},
      OmniFire{},
      DistributedWeaponFire{},
      IsRailgun{},
      Lobber{},
      Bright{},
      IsSonic{},
      Spawner{},
      LimboLaunch{},
      DecloakToFire{},
      CellRangefinding{},
      FireOnce{},
      NeverUse{},
      RevealOnFire{},
      TerrainFire{},
      SabotageCursor{},
      MigAttackCursor{},
      DisguiseFireOnly{},
      DisguiseFakeBlinkTime{},
      InfiniteMindControl{},
      FireWhileMoving{},
      DrainWeapon{},
      FireInTransport{},
      Suicide{},
      TurboBoost{},
      Supress{},
      Camera{},
      Charges{},
      IsLaser{},
      DiskLaser{},
      IsLine{},
      IsBigLaser{},
      IsHouseColor{},
      LaserDuration{},
      IonSensitive{},
      AreaFire{},
      IsElectricBolt{},
      DrawBoltAsLaser{},
      IsAlternateColor{},
      IsRadBeam{},
      IsRadEruption{},
      RadLevel{},
      IsMagBeam{} {
    AmbientDamage = 0;
    Burst = 1;
    Projectile = nullptr;
    Damage = 0;
    Speed = 0;
    Warhead = nullptr;
    ROF = 0;
    Range = 0;
    MinimumRange = 0;
    OccupantAnim = nullptr;
    AssaultAnim = nullptr;
    OpenToppedAnim = nullptr;
    AttachedParticleSystem = nullptr;
    LaserInnerColor.R = 0x0u;
    LaserInnerColor.G = 0x0u;
    LaserInnerColor.B = 0x0u;
    LaserOuterColor.R = 0x0u;
    LaserOuterColor.G = 0x0u;
    LaserOuterColor.B = 0x0u;
    LaserOuterSpread.R = 0x0u;
    LaserOuterSpread.G = 0x0u;
    LaserOuterSpread.B = 0x0u;
    UseFireParticles = false;
    UseSparkParticles = false;
    OmniFire = false;
    DistributedWeaponFire = false;
    IsRailgun = false;
    Lobber = false;
    Bright = false;
    IsSonic = false;
    Spawner = false;
    LimboLaunch = false;
    DecloakToFire = true;
    CellRangefinding = false;
    FireOnce = false;
    NeverUse = false;
    RevealOnFire = true;
    TerrainFire = false;
    SabotageCursor = false;
    MigAttackCursor = false;
    DisguiseFireOnly = false;
    DisguiseFakeBlinkTime = 0;
    InfiniteMindControl = false;
    FireWhileMoving = true;
    DrainWeapon = false;
    FireInTransport = true;
    Suicide = false;
    TurboBoost = false;
    Supress = false;
    Camera = false;
    Charges = false;
    IsLaser = false;
    DiskLaser = false;
    IsLine = false;
    IsBigLaser = false;
    IsHouseColor = false;
    LaserDuration = static_cast<char>(10);
    IonSensitive = false;
    AreaFire = false;
    IsElectricBolt = false;
    DrawBoltAsLaser = false;
    IsAlternateColor = false;
    IsRadBeam = false;
    IsRadEruption = false;
    RadLevel = 0;
    IsMagBeam = false;
    Create_ID();
    Array.AddItem(this);
}

WeaponTypeClass::~WeaponTypeClass() {
    NotifyTypeExpired();
    Projectile = nullptr; Warhead = nullptr;
    Array.Remove(this);
    Anim.~TypeList(); DownReport.~TypeList(); Report.~TypeList();
}
