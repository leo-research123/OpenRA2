// YRpp 9402d7da; constructor 00427530, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/AnimTypeClass.h"
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

AnimTypeClass* YRPP_FASTCALL AnimTypeClass::FindOrAllocate(const char* id) { return game::allocate_type<AnimTypeClass>(id); }

AnimTypeClass::AnimTypeClass(const char* pID)
    : ObjectTypeClass(pID),
      ArrayIndex{},
      MiddleFrameIndex{},
      MiddleFrameWidth{},
      MiddleFrameHeight{},
      unknown_2A4{},
      Damage{},
      Rate{},
      Start{},
      LoopStart{},
      LoopEnd{},
      End{},
      LoopCount{},
      Next{},
      SpawnsParticle{},
      NumParticles{},
      DetailLevel{},
      TranslucencyDetailLevel{},
      RandomLoopDelay{},
      RandomRate{},
      Translucency{},
      Spawns{},
      SpawnCount{},
      Report{},
      StopSound{},
      BounceAnim{},
      ExpireAnim{},
      TrailerAnim{},
      TrailerSeperation{},
      Elasticity{},
      MinZVel{},
      unknown_double_320{},
      MaxXYVel{},
      Warhead{},
      DamageRadius{},
      TiberiumSpawnType{},
      TiberiumSpreadRadius{},
      YSortAdjust{},
      YDrawOffset{},
      ZAdjust{},
      MakeInfantry{},
      RunningFrames{},
      IsFlamingGuy{},
      IsVeins{},
      IsMeteor{},
      TiberiumChainReaction{},
      IsTiberium{},
      HideIfNoOre{},
      Bouncer{},
      Tiled{},
      ShouldUseCellDrawer{},
      UseNormalLight{},
      DemandLoad{},
      FreeLoad{},
      IsAnimatedTiberium{},
      AltPalette{},
      Normalized{},
      Layer{},
      DoubleThick{},
      Flat{},
      Translucent{},
      Scorch{},
      Flamer{},
      Crater{},
      ForceBigCraters{},
      Sticky{},
      PingPong{},
      Reverse{},
      Shadow{},
      PsiWarning{},
      ShouldFogRemove{} {
    ObjectTypeClass::RadarInvisible = true;
    ObjectTypeClass::Selectable = false;
    ObjectTypeClass::LegalTarget = false;
    ObjectTypeClass::Insignificant = true;
    ObjectTypeClass::Immune = true;
    ObjectTypeClass::IsLogic = true;
    ObjectTypeClass::AllowCellContent = false;
    ArrayIndex = -1;
    MiddleFrameIndex = 0;
    MiddleFrameWidth = 0;
    MiddleFrameHeight = 0;
    unknown_2A4 = 0x0u;
    Damage = 0.0;
    Rate = 1;
    Start = 0;
    LoopStart = 0;
    LoopEnd = 0;
    End = 0;
    LoopCount = 0;
    Next = nullptr;
    SpawnsParticle = -1;
    NumParticles = 0;
    DetailLevel = 0;
    TranslucencyDetailLevel = 0;
    RandomLoopDelay.Min = 0;
    RandomLoopDelay.Max = 0;
    RandomRate.Min = 0;
    RandomRate.Max = 0;
    Translucency = 0;
    Spawns = nullptr;
    SpawnCount = 0;
    Report = -1;
    StopSound = -1;
    BounceAnim = nullptr;
    ExpireAnim = nullptr;
    TrailerAnim = nullptr;
    TrailerSeperation = 0;
    Elasticity = 0.800000011920929;
    MinZVel = 3.5;
    unknown_double_320 = 3.5;
    MaxXYVel = 15.0;
    Warhead = nullptr;
    DamageRadius = 0;
    TiberiumSpawnType = nullptr;
    TiberiumSpreadRadius = 0;
    YSortAdjust = 0;
    YDrawOffset = 0;
    ZAdjust = 0;
    MakeInfantry = -1;
    RunningFrames = 0;
    IsFlamingGuy = false;
    IsVeins = false;
    IsMeteor = false;
    TiberiumChainReaction = false;
    IsTiberium = false;
    HideIfNoOre = false;
    Bouncer = false;
    Tiled = false;
    ShouldUseCellDrawer = true;
    UseNormalLight = false;
    DemandLoad = false;
    FreeLoad = false;
    IsAnimatedTiberium = false;
    AltPalette = false;
    Normalized = false;
    Layer = static_cast<::Layer>(0x3u);
    DoubleThick = false;
    Flat = false;
    Translucent = false;
    Scorch = false;
    Flamer = false;
    Crater = false;
    ForceBigCraters = false;
    Sticky = false;
    PingPong = false;
    Reverse = false;
    Shadow = false;
    PsiWarning = false;
    ShouldFogRemove = true;
    Create_ID();
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
    TypeExpirationListeners.AddItem(this);
}

AnimTypeClass::~AnimTypeClass() {
    TypeExpirationListeners.Remove(this);
    Array.Remove(this);
}
int AnimTypeClass::GetArrayIndex() const { return ArrayIndex; }
void AnimTypeClass::PointerExpired(AbstractClass* object, bool removed) {
    (void)removed;
    if (Next == object) Next = nullptr;
}
