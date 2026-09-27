// YRpp 9402d7da; constructor 00710AF0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/TechnoTypeClass.h"
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

namespace { DynamicVectorClass<TechnoTypeClass*> types; }
DynamicVectorClass<TechnoTypeClass*>& TechnoTypeClass::Array = types;
TechnoTypeClass* YRPP_FASTCALL TechnoTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL TechnoTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

TechnoTypeClass::TechnoTypeClass(const char* id, ::SpeedType speedtype)
    : ObjectTypeClass(id),
      WalkRate{},
      IdleRate{},
      VeteranAbilities{},
      EliteAbilities{},
      SpecialThreatValue{},
      MyEffectivenessCoefficient{},
      TargetEffectivenessCoefficient{},
      TargetSpecialThreatCoefficient{},
      TargetStrengthCoefficient{},
      TargetDistanceCoefficient{},
      ThreatAvoidanceCoefficient{},
      SlowdownDistance{},
      align_2FC{},
      DecelerationFactor{},
      AccelerationFactor{},
      CloakingSpeed{},
      DebrisTypes{},
      DebrisMaximums{},
      Locomotor{},
      align_35C{},
      VoxelScaleX{},
      VoxelScaleY{},
      Weight{},
      PhysicalSize{},
      Size{},
      SizeLimit{},
      HoverAttack{},
      VHPScan{},
      unknown_int_398{},
      align_39C{},
      RollAngle{},
      PitchSpeed{},
      PitchAngle{},
      BuildLimit{},
      Category{},
      unknown_3C0{},
      align_3C4{},
      DeployTime{},
      FireAngle{},
      PipScale{},
      PipsDrawForAll{},
      LeptonMindControlOffset{},
      PixelSelectionBracketDelta{},
      PipWrap{},
      Dock{},
      DeploysInto{},
      UndeploysInto{},
      PowersUnit{},
      PoweredUnit{},
      VoiceSelect{},
      VoiceSelectEnslaved{},
      VoiceSelectDeactivated{},
      VoiceMove{},
      VoiceAttack{},
      VoiceSpecialAttack{},
      VoiceDie{},
      VoiceFeedback{},
      MoveSound{},
      DieSound{},
      AuxSound1{},
      AuxSound2{},
      CreateSound{},
      DamageSound{},
      ImpactWaterSound{},
      ImpactLandSound{},
      CrashingSound{},
      SinkingSound{},
      VoiceFalling{},
      VoiceCrashing{},
      VoiceSinking{},
      VoiceEnter{},
      VoiceCapture{},
      TurretRotateSound{},
      EnterTransportSound{},
      LeaveTransportSound{},
      DeploySound{},
      UndeploySound{},
      ChronoInSound{},
      ChronoOutSound{},
      VoiceHarvest{},
      VoicePrimaryWeaponAttack{},
      VoicePrimaryEliteWeaponAttack{},
      VoiceSecondaryWeaponAttack{},
      VoiceSecondaryEliteWeaponAttack{},
      VoiceDeploy{},
      VoiceUndeploy{},
      EnterGrinderSound{},
      LeaveGrinderSound{},
      EnterBioReactorSound{},
      LeaveBioReactorSound{},
      ActivateSound{},
      DeactivateSound{},
      MindClearedSound{},
      MovementZone{},
      GuardRange{},
      MaxDebris{},
      MinDebris{},
      DebrisAnims{},
      Passengers{},
      OpenTopped{},
      Sight{},
      ResourceGatherer{},
      ResourceDestination{},
      RevealToAll{},
      Drainable{},
      SensorsSight{},
      DetectDisguiseRange{},
      BombSight{},
      LeadershipRating{},
      NavalTargeting{},
      LandTargeting{},
      BuildTimeMultiplier{},
      MindControlRingOffset{},
      Cost{},
      Soylent{},
      FlightLevel{},
      AirstrikeTeam{},
      EliteAirstrikeTeam{},
      AirstrikeTeamType{},
      EliteAirstrikeTeamType{},
      AirstrikeRechargeTime{},
      EliteAirstrikeRechargeTime{},
      TechLevel{},
      Prerequisite{},
      PrerequisiteOverride{},
      ThreatPosed{},
      Points{},
      Speed{},
      SpeedType{},
      InitialAmmo{},
      Ammo{},
      IFVMode{},
      AirRangeBonus{},
      BerserkFriendly{},
      SprayAttack{},
      Pushy{},
      Natural{},
      Unnatural{},
      CloseRange{},
      Reload{},
      EmptyReload{},
      ReloadIncrement{},
      RadialFireSegments{},
      DeployFireWeapon{},
      DeployFire{},
      DeployToLand{},
      MobileFire{},
      OpportunityFire{},
      DistributedFire{},
      DamageReducesReadiness{},
      ReadinessReductionMultiplier{},
      UnloadingClass{},
      DeployingAnim{},
      AttackFriendlies{},
      AttackCursorOnFriendlies{},
      UndeployDelay{},
      PreventAttackMove{},
      OwnerFlags{},
      AIBasePlanningSide{},
      StupidHunt{},
      AllowedToStartInMultiplayer{},
      CameoFile{},
      align_6EF{},
      Cameo{},
      CameoAllocated{},
      AltCameoFile{},
      align_70E{},
      AltCameo{},
      AltCameoAllocated{},
      RotCount{},
      ROT{},
      TurretOffset{},
      CanBeHidden{},
      Points2{},
      Explosion{},
      DestroyAnim{},
      NaturalParticleSystem{},
      NaturalParticleSystemLocation{},
      RefinerySmokeParticleSystem{},
      DamageParticleSystems{},
      DestroyParticleSystems{},
      DamageSmokeOffset{},
      DamSmkOffScrnRel{},
      DestroySmokeOffset{},
      RefinerySmokeOffsetOne{},
      RefinerySmokeOffsetTwo{},
      RefinerySmokeOffsetThree{},
      RefinerySmokeOffsetFour{},
      ShadowIndex{},
      Storage{},
      TurretNotExportedOnGround{},
      Gunner{},
      HasTurretTooltips{},
      TurretCount{},
      WeaponCount{},
      IsChargeTurret{},
      TurretWeapon{},
      AlternativeFLH{},
      Weapon{},
      ClearAllWeapons{},
      EliteWeapon{},
      TypeImmune{},
      MoveToShroud{},
      Trainable{},
      DamageSparks{},
      TargetLaser{},
      ImmuneToVeins{},
      TiberiumHeal{},
      CloakStop{},
      IsTrain{},
      IsDropship{},
      ToProtect{},
      Disableable{},
      Unbuildable{},
      DoubleOwned{},
      Invisible{},
      RadarVisible{},
      HasPrimary{},
      Sensors{},
      Nominal{},
      DontScore{},
      DamageSelf{},
      Turret{},
      TurretRecoil{},
      TurretAnimData{},
      unknown_bool_CB4{},
      BarrelAnimData{},
      unknown_bool_CC8{},
      align_CC9{},
      align_CCA{},
      align_CCB{},
      Repairable{},
      Crewed{},
      Naval{},
      Remapable{},
      Cloakable{},
      GapGenerator{},
      GapRadiusInCells{},
      SuperGapRadiusInCells{},
      Teleporter{},
      IsGattling{},
      WeaponStages{},
      WeaponStage{},
      EliteStage{},
      RateUp{},
      RateDown{},
      SelfHealing{},
      Explodes{},
      DeathWeapon{},
      DeathWeaponDamageModifier{},
      NoAutoFire{},
      TurretSpins{},
      TiltCrashJumpjet{},
      Normalized{},
      ManualReload{},
      VisibleLoad{},
      LightningRod{},
      HunterSeeker{},
      Crusher{},
      OmniCrusher{},
      OmniCrushResistant{},
      TiltsWhenCrushes{},
      IsSubterranean{},
      AutoCrush{},
      Bunkerable{},
      CanDisguise{},
      PermaDisguise{},
      DetectDisguise{},
      DisguiseWhenStill{},
      CanApproachTarget{},
      CanRecalcApproachTarget{},
      ImmuneToPsionics{},
      ImmuneToPsionicWeapons{},
      ImmuneToRadiation{},
      Parasiteable{},
      DefaultToGuardArea{},
      Warpable{},
      ImmuneToPoison{},
      ReselectIfLimboed{},
      RejoinTeamIfLimboed{},
      Slaved{},
      Enslaves{},
      SlavesNumber{},
      SlaveRegenRate{},
      SlaveReloadRate{},
      OpenTransportWeapon{},
      Spawned{},
      Spawns{},
      SpawnsNumber{},
      SpawnRegenRate{},
      SpawnReloadRate{},
      MissileSpawn{},
      Underwater{},
      BalloonHover{},
      SuppressionThreshold{},
      JumpjetTurnRate{},
      JumpjetSpeed{},
      JumpjetClimb{},
      JumpjetCrash{},
      JumpjetHeight{},
      JumpjetAccel{},
      JumpjetWobbles{},
      JumpjetNoWobbles{},
      JumpjetDeviation{},
      JumpJet{},
      Crashable{},
      ConsideredAircraft{},
      Organic{},
      NoShadow{},
      CanPassiveAquire{},
      CanRetaliate{},
      RequiresStolenThirdTech{},
      RequiresStolenSovietTech{},
      RequiresStolenAlliedTech{},
      RequiredHouses{},
      ForbiddenHouses{},
      SecretHouses{},
      UseBuffer{},
      SecondSpawnOffset{},
      IsSelectableCombatant{},
      Accelerates{},
      DisableVoxelCache{},
      DisableShadowCache{},
      ZFudgeCliff{},
      ZFudgeColumn{},
      ZFudgeTunnel{},
      ZFudgeBridge{},
      PaletteFile{},
      Palette{},
      align_DF4{} {
    ObjectTypeClass::IsLogic = true;
    WalkRate = 1;
    IdleRate = 0;
    VeteranAbilities.FASTER = false;
    VeteranAbilities.STRONGER = false;
    VeteranAbilities.FIREPOWER = false;
    VeteranAbilities.SCATTER = false;
    VeteranAbilities.ROF = false;
    VeteranAbilities.SIGHT = false;
    VeteranAbilities.CLOAK = false;
    VeteranAbilities.TIBERIUM_PROOF = false;
    VeteranAbilities.VEIN_PROOF = false;
    VeteranAbilities.SELF_HEAL = false;
    VeteranAbilities.EXPLODES = false;
    VeteranAbilities.RADAR_INVISIBLE = false;
    VeteranAbilities.SENSORS = false;
    VeteranAbilities.FEARLESS = false;
    VeteranAbilities.C4 = false;
    VeteranAbilities.TIBERIUM_HEAL = false;
    VeteranAbilities.GUARD_AREA = false;
    VeteranAbilities.CRUSHER = false;
    EliteAbilities.FASTER = false;
    EliteAbilities.STRONGER = false;
    EliteAbilities.FIREPOWER = false;
    EliteAbilities.SCATTER = false;
    EliteAbilities.ROF = false;
    EliteAbilities.SIGHT = false;
    EliteAbilities.CLOAK = false;
    EliteAbilities.TIBERIUM_PROOF = false;
    EliteAbilities.VEIN_PROOF = false;
    EliteAbilities.SELF_HEAL = false;
    EliteAbilities.EXPLODES = false;
    EliteAbilities.RADAR_INVISIBLE = false;
    EliteAbilities.SENSORS = false;
    EliteAbilities.FEARLESS = false;
    EliteAbilities.C4 = false;
    EliteAbilities.TIBERIUM_HEAL = false;
    EliteAbilities.GUARD_AREA = false;
    EliteAbilities.CRUSHER = false;
    SpecialThreatValue = 0.0;
    MyEffectivenessCoefficient = 0.0;
    TargetEffectivenessCoefficient = 0.0;
    TargetSpecialThreatCoefficient = 0.0;
    TargetStrengthCoefficient = 0.0;
    TargetDistanceCoefficient = 0.0;
    ThreatAvoidanceCoefficient = 0.0;
    SlowdownDistance = 500;
    DecelerationFactor = 0.002;
    AccelerationFactor = 0.03;
    CloakingSpeed = 7;
    Locomotor = {0x4a582747u, 0x9839, 0x11d1, {0xb7, 0x09, 0x00, 0xa0, 0x24, 0xdd, 0xaf, 0xd1}};
    VoxelScaleX = 0.0;
    VoxelScaleY = 0.0;
    Weight = 1.0;
    PhysicalSize = 2.0;
    Size = 1.0;
    SizeLimit = 0.0;
    HoverAttack = false;
    VHPScan = 0;
    unknown_int_398 = 15;
    RollAngle = 0.5235987755982988;
    PitchSpeed = 0.25;
    PitchAngle = 0.3490658503988659;
    BuildLimit = 2147483647;
    Category = static_cast<::Category>(0xffffffffu);
    unknown_3C0 = 0x0u;
    DeployTime = 0.0;
    FireAngle = 8;
    PipScale = static_cast<::PipScale>(0x0u);
    PipsDrawForAll = false;
    LeptonMindControlOffset = 70;
    PixelSelectionBracketDelta = 0;
    PipWrap = 0;
    DeploysInto = nullptr;
    UndeploysInto = nullptr;
    PowersUnit = nullptr;
    PoweredUnit = false;
    AuxSound1 = -1;
    AuxSound2 = -1;
    CreateSound = -1;
    DamageSound = -1;
    ImpactWaterSound = -1;
    ImpactLandSound = -1;
    CrashingSound = -1;
    SinkingSound = -1;
    VoiceFalling = -1;
    VoiceCrashing = -1;
    VoiceSinking = -1;
    VoiceEnter = -1;
    VoiceCapture = -1;
    TurretRotateSound = -1;
    EnterTransportSound = -1;
    LeaveTransportSound = -1;
    DeploySound = -1;
    UndeploySound = -1;
    ChronoInSound = -1;
    ChronoOutSound = -1;
    VoiceHarvest = -1;
    VoicePrimaryWeaponAttack = -1;
    VoicePrimaryEliteWeaponAttack = -1;
    VoiceSecondaryWeaponAttack = -1;
    VoiceSecondaryEliteWeaponAttack = -1;
    VoiceDeploy = -1;
    VoiceUndeploy = -1;
    EnterGrinderSound = -1;
    LeaveGrinderSound = -1;
    EnterBioReactorSound = -1;
    LeaveBioReactorSound = -1;
    ActivateSound = -1;
    DeactivateSound = -1;
    MindClearedSound = -1;
    MovementZone = static_cast<::MovementZone>(0x0u);
    GuardRange = 0;
    MaxDebris = 0;
    MinDebris = 0;
    Passengers = 0;
    OpenTopped = false;
    Sight = 0;
    ResourceGatherer = false;
    ResourceDestination = false;
    RevealToAll = false;
    Drainable = false;
    SensorsSight = 0;
    DetectDisguiseRange = 0;
    BombSight = 0;
    LeadershipRating = 5;
    NavalTargeting = static_cast<::NavalTargetingType>(0x0u);
    LandTargeting = static_cast<::LandTargetingType>(0x0u);
    BuildTimeMultiplier = 1.0f;
    MindControlRingOffset = 140;
    Cost = 0;
    Soylent = 0;
    FlightLevel = -1;
    AirstrikeTeam = 0;
    EliteAirstrikeTeam = 0;
    AirstrikeTeamType = nullptr;
    EliteAirstrikeTeamType = nullptr;
    AirstrikeRechargeTime = 0;
    EliteAirstrikeRechargeTime = 0;
    TechLevel = 255;
    ThreatPosed = 0;
    Points = 0;
    Speed = 0;
    InitialAmmo = -1;
    Ammo = -1;
    IFVMode = 0;
    AirRangeBonus = 0;
    BerserkFriendly = false;
    SprayAttack = false;
    Pushy = false;
    Natural = false;
    Unnatural = false;
    CloseRange = false;
    Reload = 0;
    EmptyReload = -1;
    ReloadIncrement = 0;
    RadialFireSegments = 0;
    DeployFireWeapon = 1;
    DeployFire = false;
    DeployToLand = false;
    MobileFire = true;
    OpportunityFire = false;
    DistributedFire = false;
    DamageReducesReadiness = false;
    ReadinessReductionMultiplier = 0;
    UnloadingClass = nullptr;
    DeployingAnim = nullptr;
    AttackFriendlies = false;
    AttackCursorOnFriendlies = false;
    UndeployDelay = -1;
    PreventAttackMove = false;
    OwnerFlags = 0x0u;
    AIBasePlanningSide = -1;
    StupidHunt = false;
    AllowedToStartInMultiplayer = true;
    CameoFile[0] = static_cast<char>(0);
    CameoFile[24] = static_cast<char>(0);
    Cameo = nullptr;
    CameoAllocated = false;
    AltCameoFile[0] = static_cast<char>(0);
    AltCameoFile[24] = static_cast<char>(0);
    AltCameo = nullptr;
    AltCameoAllocated = false;
    RotCount = 0;
    ROT = 0;
    TurretOffset = 0;
    CanBeHidden = true;
    Points2 = 0;
    NaturalParticleSystem = nullptr;
    NaturalParticleSystemLocation.X = 0;
    NaturalParticleSystemLocation.Y = 0;
    NaturalParticleSystemLocation.Z = 0;
    RefinerySmokeParticleSystem = nullptr;
    DamageSmokeOffset.X = 0;
    DamageSmokeOffset.Y = 0;
    DamageSmokeOffset.Z = 0;
    DamSmkOffScrnRel = false;
    DestroySmokeOffset.X = 0;
    DestroySmokeOffset.Y = 0;
    DestroySmokeOffset.Z = 0;
    RefinerySmokeOffsetOne.X = 0;
    RefinerySmokeOffsetOne.Y = 0;
    RefinerySmokeOffsetOne.Z = 0;
    RefinerySmokeOffsetTwo.X = 0;
    RefinerySmokeOffsetTwo.Y = 0;
    RefinerySmokeOffsetTwo.Z = 0;
    RefinerySmokeOffsetThree.X = 0;
    RefinerySmokeOffsetThree.Y = 0;
    RefinerySmokeOffsetThree.Z = 0;
    RefinerySmokeOffsetFour.X = 0;
    RefinerySmokeOffsetFour.Y = 0;
    RefinerySmokeOffsetFour.Z = 0;
    ShadowIndex = 0;
    Storage = 0;
    TurretNotExportedOnGround = false;
    Gunner = false;
    HasTurretTooltips = false;
    TurretCount = 0;
    IsChargeTurret = false;
    ClearAllWeapons = false;
    TypeImmune = false;
    MoveToShroud = true;
    Trainable = true;
    DamageSparks = false;
    TargetLaser = false;
    ImmuneToVeins = false;
    TiberiumHeal = false;
    CloakStop = false;
    IsTrain = false;
    IsDropship = false;
    ToProtect = false;
    Disableable = true;
    Unbuildable = false;
    DoubleOwned = false;
    Invisible = false;
    RadarVisible = false;
    HasPrimary = false;
    Sensors = false;
    Nominal = false;
    DontScore = false;
    DamageSelf = false;
    Turret = false;
    TurretRecoil = false;
    TurretAnimData.Travel = 2;
    TurretAnimData.CompressFrames = 1;
    TurretAnimData.RecoverFrames = 1;
    TurretAnimData.HoldFrames = 1;
    unknown_bool_CB4 = false;
    BarrelAnimData.Travel = 2;
    BarrelAnimData.CompressFrames = 1;
    BarrelAnimData.RecoverFrames = 1;
    BarrelAnimData.HoldFrames = 1;
    unknown_bool_CC8 = false;
    Repairable = true;
    Crewed = false;
    Naval = false;
    Remapable = false;
    Cloakable = false;
    GapGenerator = false;
    GapRadiusInCells = static_cast<char>(0);
    SuperGapRadiusInCells = static_cast<char>(0);
    Teleporter = false;
    IsGattling = false;
    WeaponStages = 0;
    RateUp = 0;
    RateDown = 0;
    SelfHealing = false;
    Explodes = false;
    DeathWeapon = nullptr;
    DeathWeaponDamageModifier = 1.0f;
    NoAutoFire = false;
    TurretSpins = false;
    TiltCrashJumpjet = false;
    Normalized = false;
    ManualReload = false;
    VisibleLoad = false;
    LightningRod = false;
    HunterSeeker = false;
    Crusher = false;
    OmniCrusher = false;
    OmniCrushResistant = false;
    TiltsWhenCrushes = true;
    IsSubterranean = false;
    AutoCrush = false;
    CanDisguise = false;
    PermaDisguise = false;
    DetectDisguise = false;
    DisguiseWhenStill = false;
    CanApproachTarget = true;
    CanRecalcApproachTarget = true;
    ImmuneToRadiation = false;
    DefaultToGuardArea = false;
    Warpable = true;
    ReselectIfLimboed = false;
    RejoinTeamIfLimboed = false;
    Slaved = false;
    Enslaves = nullptr;
    SlavesNumber = 0;
    SlaveRegenRate = 0;
    SlaveReloadRate = 0;
    OpenTransportWeapon = -1;
    Spawned = false;
    Spawns = nullptr;
    SpawnsNumber = 0;
    SpawnRegenRate = 0;
    SpawnReloadRate = 0;
    MissileSpawn = false;
    Underwater = false;
    BalloonHover = false;
    SuppressionThreshold = 0;
    JumpjetTurnRate = 4;
    JumpjetSpeed = 14;
    JumpjetClimb = 5.0f;
    JumpjetCrash = 5.0f;
    JumpjetHeight = 500;
    JumpjetAccel = 2.0f;
    JumpjetWobbles = 0.15000000596046448f;
    JumpjetNoWobbles = false;
    JumpjetDeviation = 40;
    JumpJet = false;
    Crashable = false;
    NoShadow = false;
    CanPassiveAquire = true;
    CanRetaliate = true;
    RequiresStolenThirdTech = false;
    RequiresStolenSovietTech = false;
    RequiresStolenAlliedTech = false;
    RequiredHouses = 0xffffffffu;
    ForbiddenHouses = 0xffffffffu;
    SecretHouses = 0xffffffffu;
    UseBuffer = false;
    SecondSpawnOffset.X = 0;
    SecondSpawnOffset.Y = 0;
    SecondSpawnOffset.Z = 0;
    IsSelectableCombatant = false;
    Accelerates = true;
    DisableVoxelCache = false;
    DisableShadowCache = false;
    ZFudgeCliff = 10;
    ZFudgeColumn = 5;
    ZFudgeTunnel = 10;
    ZFudgeBridge = 0;
    PaletteFile[0] = static_cast<char>(0);
    this->SpeedType = speedtype;
    for (auto& turret : TurretWeapon) turret = -1;
    TypeExpirationListeners.AddItem(this);
    Array.AddItem(this);
}

TechnoTypeClass::~TechnoTypeClass() {
    if (CameoAllocated) YRMemory::Deallocate(Cameo);
    Cameo = nullptr;
    if (AltCameoAllocated) YRMemory::Deallocate(AltCameo);
    AltCameo = nullptr;
    TypeExpirationListeners.Remove(this);
    Array.Remove(this);
}
