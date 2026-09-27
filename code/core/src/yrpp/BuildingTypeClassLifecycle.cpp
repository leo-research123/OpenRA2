// YRpp 9402d7da; constructor 0045DD90, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/BuildingTypeClass.h"
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

namespace { DynamicVectorClass<BuildingTypeClass*> types; }
DynamicVectorClass<BuildingTypeClass*>& BuildingTypeClass::Array = types;
BuildingTypeClass* YRPP_FASTCALL BuildingTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL BuildingTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

BuildingTypeClass::BuildingTypeClass(const char* pID, const ConstructionDefaults& defaults)
    : TechnoTypeClass(pID, static_cast<::SpeedType>(0)),
      ArrayIndex{},
      FoundationData{},
      Buildup{},
      BuildupLoaded{},
      BuildCat{},
      HalfDamageSmokeLocation1{},
      HalfDamageSmokeLocation2{},
      align_E24{},
      GateCloseDelay{},
      LightVisibility{},
      LightIntensity{},
      LightRedTint{},
      LightGreenTint{},
      LightBlueTint{},
      PrimaryFirePixelOffset{},
      SecondaryFirePixelOffset{},
      ToOverlay{},
      ToTile{},
      BuildupFile{},
      BuildupSound{},
      PackupSound{},
      CreateUnitSound{},
      UnitEnterSound{},
      UnitExitSound{},
      WorkingSound{},
      NotWorkingSound{},
      PowersUpBuilding{},
      FreeUnit{},
      SecretInfantry{},
      SecretUnit{},
      SecretBuilding{},
      field_EB0{},
      Adjacent{},
      Factory{},
      TargetCoordOffset{},
      ExitCoord{},
      FoundationOutside{},
      StartFacing{},
      DeployFacing{},
      PowerBonus{},
      PowerDrain{},
      ExtraPowerBonus{},
      ExtraPowerDrain{},
      Foundation{},
      Height{},
      OccupyHeight{},
      MidPoint{},
      DoorStages{},
      BuildingAnimFrame{},
      BuildingAnim{},
      Upgrades{},
      DeployingAnim{},
      DeployingAnimLoaded{},
      UnderDoorAnim{},
      UnderDoorAnimLoaded{},
      Rubble{},
      RubbleLoaded{},
      RoofDeployingAnim{},
      RoofDeployingAnimLoaded{},
      UnderRoofDoorAnim{},
      UnderRoofDoorAnimLoaded{},
      DoorAnim{},
      SpecialZOverlay{},
      SpecialZOverlayZAdjust{},
      BibShape{},
      BibShapeLoaded{},
      NormalZAdjust{},
      AntiAirValue{},
      AntiArmorValue{},
      AntiInfantryValue{},
      ZShapePointMove{},
      unknown_1538{},
      unknown_153C{},
      unknown_1540{},
      unknown_1544{},
      ExtraLight{},
      TogglePower{},
      HasSpotlight{},
      IsTemple{},
      IsPlug{},
      HoverPad{},
      BaseNormal{},
      EligibileForAllyBuilding{},
      EligibleForDelayKill{},
      NeedsEngineer{},
      CaptureEvaEvent{},
      ProduceCashStartup{},
      ProduceCashAmount{},
      ProduceCashDelay{},
      InfantryGainSelfHeal{},
      UnitsGainSelfHeal{},
      RefinerySmokeFrames{},
      Bib{},
      Wall{},
      Capturable{},
      Powered{},
      PoweredSpecial{},
      Overpowerable{},
      Spyable{},
      CanC4{},
      WantsExtraSpace{},
      Unsellable{},
      ClickRepairable{},
      CanBeOccupied{},
      CanOccupyFire{},
      MaxNumberOccupants{},
      ShowOccupantPips{},
      MuzzleFlash{},
      DamageFireOffset{},
      QueueingCell{},
      NumberImpassableRows{},
      RemoveOccupy{},
      AddOccupy{},
      Radar{},
      SpySat{},
      ChargeAnim{},
      IsAnimDelayedFire{},
      SiloDamage{},
      UnitRepair{},
      UnitReload{},
      Bunker{},
      Cloning{},
      Grinding{},
      UnitAbsorb{},
      InfantryAbsorb{},
      SecretLab{},
      DoubleThick{},
      Flat{},
      DockUnload{},
      Recoilless{},
      HasStupidGuardMode{},
      BridgeRepairHut{},
      Gate{},
      SAM{},
      ConstructionYard{},
      NukeSilo{},
      Refinery{},
      Weeder{},
      WeaponsFactory{},
      LaserFencePost{},
      LaserFence{},
      FirestormWall{},
      Hospital{},
      Armory{},
      EMPulseCannon{},
      TickTank{},
      TurretAnimIsVoxel{},
      BarrelAnimIsVoxel{},
      CloakGenerator{},
      SensorArray{},
      ICBMLauncher{},
      Artillary{},
      Helipad{},
      OrePurifier{},
      FactoryPlant{},
      InfantryCostBonus{},
      UnitsCostBonus{},
      AircraftCostBonus{},
      BuildingsCostBonus{},
      DefensesCostBonus{},
      GDIBarracks{},
      NODBarracks{},
      YuriBarracks{},
      ChargedAnimTime{},
      DelayedFireDelay{},
      SuperWeapon{},
      SuperWeapon2{},
      GateStages{},
      PowersUpToLevel{},
      DamagedDoor{},
      InvisibleInGame{},
      TerrainPalette{},
      PlaceAnywhere{},
      ExtraDamageStage{},
      AIBuildThis{},
      IsBaseDefense{},
      CloakRadiusInCells{},
      ConcentricRadialIndicator{},
      PsychicDetectionRadius{},
      BarrelStartPitch{},
      VoxelBarrelFile{},
      VoxelBarrelScale{},
      VoxelBarrelOffsetToPitchPivotPoint{},
      VoxelBarrelOffsetToRotatePivotPoint{},
      VoxelBarrelOffsetToBuildingPivotPoint{},
      VoxelBarrelOffsetToBarrelEnd{},
      DemandLoad{},
      DemandLoadBuildup{},
      FreeBuildup{},
      IsThreatRatingNode{},
      PrimaryFireDualOffset{},
      ProtectWithWall{},
      CanHideThings{},
      CrateBeneath{},
      LeaveRubble{},
      CrateBeneathIsMoney{},
      TheaterSpecificID{},
      NumberOfDocks{},
      DockingOffsets(1),
      align_1794{} {
    TechnoTypeClass::Trainable = false;
    TechnoTypeClass::Bunkerable = false;
    TechnoTypeClass::ImmuneToPsionics = true;
    TechnoTypeClass::ImmuneToPsionicWeapons = true;
    TechnoTypeClass::Parasiteable = false;
    TechnoTypeClass::ImmuneToPoison = false;
    TechnoTypeClass::ConsideredAircraft = false;
    TechnoTypeClass::Organic = false;
    ArrayIndex = -1;
    FoundationData = nullptr;
    Buildup = nullptr;
    BuildupLoaded = false;
    BuildCat = static_cast<::BuildCat>(0x0u);
    GateCloseDelay = 0.0;
    LightVisibility = 5000;
    LightIntensity = 0;
    LightRedTint = 1000000;
    LightGreenTint = 1000000;
    LightBlueTint = 1000000;
    PrimaryFirePixelOffset.X = 65535;
    PrimaryFirePixelOffset.Y = 65535;
    SecondaryFirePixelOffset.X = 65535;
    SecondaryFirePixelOffset.Y = 65535;
    ToOverlay = nullptr;
    ToTile = nullptr;
    BuildupFile[0] = static_cast<char>(0);
    BuildupSound = -1;
    PackupSound = -1;
    CreateUnitSound = -1;
    UnitEnterSound = -1;
    UnitExitSound = -1;
    WorkingSound = -1;
    NotWorkingSound = -1;
    PowersUpBuilding[0] = static_cast<char>(0);
    FreeUnit = nullptr;
    SecretInfantry = nullptr;
    SecretUnit = nullptr;
    SecretBuilding = nullptr;
    field_EB0 = -1;
    Adjacent = 3;
    Factory = static_cast<::AbstractType>(0x0u);
    TargetCoordOffset.X = 0;
    TargetCoordOffset.Y = 0;
    TargetCoordOffset.Z = 0;
    ExitCoord.X = 0;
    ExitCoord.Y = 0;
    ExitCoord.Z = 0;
    FoundationOutside = nullptr;
    StartFacing = 0;
    DeployFacing = 128;
    PowerBonus = 0;
    PowerDrain = 0;
    ExtraPowerBonus = 0;
    ExtraPowerDrain = 0;
    Foundation = static_cast<::Foundation>(0x0u);
    Height = 2;
    OccupyHeight = 2;
    MidPoint = 0;
    DoorStages = 0;
    BuildingAnimFrame[0].dwUnknown = 0x0u;
    BuildingAnimFrame[0].FrameCount = 1;
    BuildingAnimFrame[0].FrameDuration = 0;
    BuildingAnimFrame[1].dwUnknown = 0x0u;
    BuildingAnimFrame[1].FrameCount = 1;
    BuildingAnimFrame[1].FrameDuration = 0;
    BuildingAnimFrame[2].dwUnknown = 0x0u;
    BuildingAnimFrame[2].FrameCount = 1;
    BuildingAnimFrame[2].FrameDuration = 0;
    BuildingAnimFrame[4].dwUnknown = 0x0u;
    BuildingAnimFrame[4].FrameCount = 1;
    BuildingAnimFrame[4].FrameDuration = 0;
    BuildingAnimFrame[5].dwUnknown = 0x0u;
    BuildingAnimFrame[5].FrameCount = 1;
    BuildingAnimFrame[5].FrameDuration = 0;
    Upgrades = 0;
    DeployingAnim = nullptr;
    DeployingAnimLoaded = false;
    UnderDoorAnim = nullptr;
    UnderDoorAnimLoaded = false;
    Rubble = nullptr;
    RubbleLoaded = false;
    RoofDeployingAnim = nullptr;
    RoofDeployingAnimLoaded = false;
    UnderRoofDoorAnim = nullptr;
    UnderRoofDoorAnimLoaded = false;
    DoorAnim = nullptr;
    SpecialZOverlay = nullptr;
    SpecialZOverlayZAdjust = 0;
    BibShape = nullptr;
    BibShapeLoaded = false;
    NormalZAdjust = 0;
    AntiAirValue = 0;
    AntiArmorValue = 0;
    AntiInfantryValue = 0;
    ZShapePointMove.X = 0;
    ZShapePointMove.Y = 0;
    ExtraLight = 0x0u;
    TogglePower = true;
    HasSpotlight = false;
    IsTemple = false;
    IsPlug = false;
    HoverPad = false;
    BaseNormal = true;
    EligibileForAllyBuilding = false;
    EligibleForDelayKill = false;
    NeedsEngineer = false;
    CaptureEvaEvent = -1;
    ProduceCashStartup = 0;
    ProduceCashAmount = 0;
    ProduceCashDelay = 0;
    InfantryGainSelfHeal = 0;
    UnitsGainSelfHeal = 0;
    RefinerySmokeFrames = 25;
    Bib = false;
    Wall = false;
    Capturable = false;
    Powered = false;
    PoweredSpecial = false;
    Overpowerable = false;
    Spyable = false;
    CanC4 = true;
    WantsExtraSpace = false;
    Unsellable = false;
    ClickRepairable = true;
    CanBeOccupied = false;
    CanOccupyFire = false;
    MaxNumberOccupants = 0;
    ShowOccupantPips = true;
    QueueingCell.X = 0;
    QueueingCell.Y = 0;
    NumberImpassableRows = -1;
    Radar = false;
    SpySat = false;
    ChargeAnim = false;
    IsAnimDelayedFire = false;
    SiloDamage = false;
    UnitRepair = false;
    UnitReload = false;
    Bunker = false;
    Cloning = false;
    Grinding = false;
    UnitAbsorb = false;
    InfantryAbsorb = false;
    SecretLab = false;
    DoubleThick = false;
    Flat = false;
    DockUnload = false;
    Recoilless = false;
    HasStupidGuardMode = true;
    BridgeRepairHut = false;
    Gate = false;
    SAM = false;
    ConstructionYard = false;
    NukeSilo = false;
    Refinery = false;
    Weeder = false;
    WeaponsFactory = false;
    LaserFencePost = false;
    LaserFence = false;
    FirestormWall = false;
    Hospital = false;
    Armory = false;
    EMPulseCannon = false;
    TickTank = false;
    TurretAnimIsVoxel = false;
    BarrelAnimIsVoxel = false;
    CloakGenerator = false;
    SensorArray = false;
    ICBMLauncher = false;
    Artillary = false;
    Helipad = false;
    OrePurifier = false;
    FactoryPlant = false;
    InfantryCostBonus = 1.0f;
    UnitsCostBonus = 1.0f;
    AircraftCostBonus = 1.0f;
    BuildingsCostBonus = 1.0f;
    DefensesCostBonus = 1.0f;
    GDIBarracks = false;
    NODBarracks = false;
    YuriBarracks = false;
    ChargedAnimTime = 999.0f;
    DelayedFireDelay = 0;
    SuperWeapon = -1;
    SuperWeapon2 = -1;
    GateStages = 9;
    PowersUpToLevel = -1;
    DamagedDoor = false;
    InvisibleInGame = false;
    TerrainPalette = false;
    PlaceAnywhere = false;
    ExtraDamageStage = true;
    AIBuildThis = false;
    IsBaseDefense = false;
    CloakRadiusInCells = 0x14u;
    ConcentricRadialIndicator = false;
    PsychicDetectionRadius = 0;
    BarrelStartPitch = 64;
    VoxelBarrelFile[0] = static_cast<char>(0);
    DemandLoad = false;
    DemandLoadBuildup = false;
    FreeBuildup = false;
    IsThreatRatingNode = false;
    PrimaryFireDualOffset = false;
    ProtectWithWall = false;
    CanHideThings = true;
    CrateBeneath = false;
    LeaveRubble = false;
    CrateBeneathIsMoney = false;
    TheaterSpecificID[0] = static_cast<char>(0);
    NumberOfDocks = 1;
    for (auto& animation : BuildingAnim) animation.Powered = true;
    for (auto& value : MuzzleFlash) value = Point2D{defaults.coordinate.X, defaults.coordinate.Y};
    for (auto& value : DamageFireOffset) value = Point2D{defaults.coordinate.X, defaults.coordinate.Y};
    HalfDamageSmokeLocation1 = defaults.coordinate;
    HalfDamageSmokeLocation2 = defaults.coordinate;
    // Explicit target fixed-data values; not guessed empty rectangle semantics.
    unknown_1538 = defaults.unknown_1538[0];
    unknown_153C = defaults.unknown_1538[1];
    unknown_1540 = defaults.unknown_1538[2];
    unknown_1544 = defaults.unknown_1538[3];
    for (auto& value : RemoveOccupy) value = Point2D{65535, 65535};
    for (auto& value : AddOccupy) value = Point2D{65535, 65535};
    if (DockingOffsets.Capacity) DockingOffsets[0] = CoordStruct::Empty;
    Create_ID();
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
}

BuildingTypeClass::~BuildingTypeClass() {
    if (BuildupLoaded) YRMemory::Deallocate(Buildup);
    Buildup = nullptr;
    if (BibShapeLoaded) YRMemory::Deallocate(BibShape);
    BibShape = nullptr;
    if (DeployingAnimLoaded) YRMemory::Deallocate(DeployingAnim);
    DeployingAnim = nullptr;
    if (UnderDoorAnimLoaded) YRMemory::Deallocate(UnderDoorAnim);
    UnderDoorAnim = nullptr;
    if (RubbleLoaded) YRMemory::Deallocate(Rubble);
    Rubble = nullptr;
    if (RoofDeployingAnimLoaded) YRMemory::Deallocate(RoofDeployingAnim);
    RoofDeployingAnim = nullptr;
    if (UnderRoofDoorAnimLoaded) YRMemory::Deallocate(UnderRoofDoorAnim);
    UnderRoofDoorAnim = nullptr;
    NotifyTypeExpired();
    Array.Remove(this);
}
int BuildingTypeClass::GetArrayIndex() const { return ArrayIndex; }

BuildingTypeClass* YRPP_FASTCALL BuildingTypeClass::FindOrAllocate(const char* id, const ConstructionDefaults& defaults) {
    if (!id || !_strcmpi(id, "none") || !_strcmpi(id, "<none>")) return nullptr;
    if (auto* found = Find(id)) return found;
    void* memory = YRMemory::Allocate(sizeof(BuildingTypeClass));
    if (!memory) return nullptr;
    try { return new (memory) BuildingTypeClass(id, defaults); }
    catch (...) { YRMemory::Deallocate(memory); throw; }
}
