// Original constructor: 0x004F54A0.
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/Notifications.h"
#include "yrpp/SuperClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/SessionClass.h"
#include "x87_integer.hpp"
#include <cstring>
#include <cwchar>
#include <new>
HouseClass* YRPP_FASTCALL HouseClass::FindByCountryIndex(int country) {
    for (auto* house : Array)
        // 0x502D4B compares the country selector at +0xB8, not the
        // HouseType array position at +0xB4.
        if (house->Type->ArrayIndex2 == country) return house;
    return nullptr;
}
signed int YRPP_FASTCALL HouseClass::FindIndexByName(const char* name) {
    for (int i = 0; i < Array.Count; ++i)
        if (!std::strcmp(Array[i]->PlainName, name)) return i;
    return -1;
}
HouseClass* YRPP_FASTCALL HouseClass::FindByIndex(int playerAt) {
    if (!Index_IsMP(playerAt)) return nullptr;
    return Array.GetItemOrDefault(ScenarioClass::Instance->HouseIndices[playerAt - PlayerAtA]);
}
bool HouseClass::IsControlledByHuman() const {
    return IsHumanPlayer || (SessionClass::Instance.GameMode == GameMode::Campaign && IsInPlayerControl);
}
bool HouseClass::IsControlledByCurrentPlayer() const {
    if (SessionClass::Instance.GameMode == GameMode::Campaign) return IsHumanPlayer || IsInPlayerControl;
    return IsCurrentPlayer();
}
namespace {
DynamicVectorClass<HouseClass*> houses;
HouseClass* player = nullptr;
HouseClass* observer = nullptr;
}
DynamicVectorClass<HouseClass*>& HouseClass::Array = houses;
HouseClass*& HouseClass::CurrentPlayer = player;
HouseClass*& HouseClass::Observer = observer;
HouseClass::HouseClass(HouseTypeClass* pType) noexcept
    // 0x4F54A0..0x4F5DC6 member defaults. Timers start on the current frame.
    : AbstractClass(), ArrayIndex{-1}, Type{pType}, RelatedTags{}, ConYards{}, Buildings{}, UnitRepairStations{},
      Grinders{}, Absorbers{}, Bunkers{}, Occupiables{}, CloningVats{}, SecretLabs{}, PsychicDetectionBuildings{},
      FactoryPlants{}, CountResourceGatherers{}, CountResourceDestinations{}, CountWarfactories{}, InfantrySelfHeal{},
      UnitsSelfHeal{}, StartingUnits{}, AIDifficulty{static_cast<::AIDifficulty>(ScenarioClass::Instance->Difficulty2)},
      FirepowerMultiplier{1.0}, GroundspeedMultiplier{1.0}, AirspeedMultiplier{1.0}, ArmorMultiplier{1.0},
      ROFMultiplier{1.0}, CostMultiplier{1.0}, BuildTimeMultiplier{1.0}, RepairDelay{}, BuildDelay{}, IQLevel{},
      TechLevel{1}, AltAllies{}, StartingCredits{}, StartingEdge{Edge::None}, AIState_1E4{}, SideIndex{-1},
      IsHumanPlayer{}, IsInPlayerControl{}, Production{}, AutocreateAllowed{}, NodeLogic_1F0{true}, ShipYardConst_1F1{},
      AITriggersActive{}, AutoBaseBuilding{}, DiscoveredByPlayer{}, Defeated{}, IsGameOver{}, IsWinner{}, IsLoser{},
      CiviliansEvacuated{}, FirestormActive{}, HasThreatNode{}, RecheckTechTree{true}, IPAddress{}, TournamentTeamID{},
      LostConnection{}, SelectedPathIndex{-1}, PlanningPaths{}, Visionary{}, MapIsClear{}, IsTiberiumShort{},
      HasBeenSpied{}, HasBeenThieved{}, Repairing{}, IsBuiltSomething{}, IsResigner{}, IsGiverUpper{}, AllToHunt{},
      IsParanoid{}, IsToLook{true}, IQLevel2{}, AIMode{}, Supers{}, LastBuiltBuildingType{-1},
      LastBuiltInfantryType{-1}, LastBuiltAircraftType{-1}, LastBuiltVehicleType{-1}, AllowWinBlocks{}, RepairTimer{},
      AlertTimer{}, BorrowedTime{}, PowerBlackoutTimer{}, RadarBlackoutTimer{}, Side2TechInfiltrated{},
      Side1TechInfiltrated{}, Side0TechInfiltrated{}, BarracksInfiltrated{}, WarFactoryInfiltrated{},
      InfantryAltOwner{}, UnitAltOwner{}, AircraftAltOwner{}, BuildingAltOwner{}, AirportDocks{}, PoweredUnitCenters{},
      CreditsSpent{}, HarvestedCredits{}, StolenBuildingsCredits{}, OwnedUnits{}, OwnedNavy{}, OwnedBuildings{},
      OwnedInfantry{}, OwnedAircraft{}, OwnedTiberium{}, Balance{}, TotalStorage{}, OwnedWeed{}, unknown_324{},
      BuiltAircraftTypes{}, BuiltInfantryTypes{}, BuiltUnitTypes{}, BuiltBuildingTypes{}, KilledAircraftTypes{},
      KilledInfantryTypes{}, KilledUnitTypes{}, KilledBuildingTypes{}, CapturedBuildings{}, CollectedCrates{},
      NumAirpads{}, NumBarracks{}, NumWarFactories{}, NumConYards{}, NumShipyards{}, NumOrePurifiers{},
      CostInfantryMult{1.0f}, CostUnitsMult{1.0f}, CostAircraftMult{1.0f}, CostBuildingsMult{1.0f},
      CostDefensesMult{1.0f}, PowerOutput{}, PowerDrain{}, Primary_ForAircraft{}, Primary_ForInfantry{},
      Primary_ForVehicles{}, Primary_ForShips{}, Primary_ForBuildings{}, Primary_Unused1{}, Primary_Unused2{},
      Primary_Unused3{}, Primary_ForDefenses{}, AircraftType_53D0{}, InfantryType_53D1{}, VehicleType_53D2{},
      ShipType_53D3{}, BuildingType_53D4{}, unknown_53D5{}, unknown_53D6{}, unknown_53D7{}, DefenseType_53D8{},
      unknown_53D9{}, unknown_53DA{}, unknown_53DB{}, OurFlagCarrier{}, OurFlagCoords{}, KilledUnitsOfHouses{},
      TotalKilledUnits{}, KilledBuildingsOfHouses{}, TotalKilledBuildings{}, WhoLastHurtMe{-1},
      BaseSpawnCell{CellStruct::Empty}, BaseCenter{CellStruct::Empty}, Radius{}, ZoneInfos{}, LATime{}, LAEnemy{-1},
      ToCapture{}, RadarVisibleTo{}, PointTotal{}, PreferredTargetType{QuarryType::Anything}, PreferredTargetCell{},
      PreferredDefensiveCell{}, PreferredDefensiveCell2{}, PreferredDefensiveCellStartTime{-100},
      OwnedBuildingTypes{}, OwnedUnitTypes{}, OwnedInfantryTypes{}, OwnedAircraftTypes{}, ActiveBuildingTypes{},
      ActiveUnitTypes{}, ActiveInfantryTypes{}, ActiveAircraftTypes{}, FactoryProducedBuildingTypes{},
      FactoryProducedUnitTypes{}, FactoryProducedInfantryTypes{}, FactoryProducedAircraftTypes{}, AttackTimer{},
      InitialAttackDelay{}, EnemyHouseIndex{-1}, AngerNodes{}, ScoutNodes{}, AITimer{}, Unknown_Timer_5640{},
      ProducingBuildingTypeIndex{-1}, ProducingUnitTypeIndex{-1}, ProducingInfantryTypeIndex{-1},
      ProducingAircraftTypeIndex{-1}, RatioAITriggerTeam{100}, RatioTeamAircraft{75}, RatioTeamInfantry{75},
      RatioTeamBuildings{75}, BaseDefenseTeamCount{}, DropshipData{}, CurrentDropshipIndex{}, HasCloakingRanges{},
      Color{}, LaserColor{255, 255, 255}, Base{}, RecheckPower{true}, RecheckRadar{true}, SpySatActive{},
      IsBeingDrained{}, Edge{}, EMPTarget{}, NukeTarget{}, Allies{}, DamageDelayTimer{}, TeamDelayTimer{},
      TriggerDelayTimer{}, SpeakAttackDelayTimer{}, SpeakPowerDelayTimer{}, SpeakMoneyDelayTimer{},
      SpeakMaxedDelayTimer{}, AIGeneral{}, ThreatPosedEstimates{}, PlainName{}, UINameString{}, UIName{},
      ColorSchemeIndex{}, StartingPoint{}, StartingAllies{}, BaseDefenseScores{}, WaypointPath{}, unknown_1607C{},
      unknown_16080{}, unknown_16084{}, unused_16088{1.0}, unused_16090{1.0}, padding_16098{},
      PredictionEnemyArmor{0.33f}, PredictionEnemyAir{0.33f}, PredictionEnemyInfantry{0.33f},
      TotalOwnedInfantryCost{}, TotalOwnedVehicleCost{}, TotalOwnedAircraftCost{}, PowerSurplus{} {
    RepairTimer.Start(0);
    AlertTimer.Start(0);
    BorrowedTime.Start(0);
    PowerBlackoutTimer.Start(0);
    RadarBlackoutTimer.Start(0);
    AttackTimer.Start(0);
    AITimer.Start(0);
    Unknown_Timer_5640.Start(0);
    // The products run on x87 under the game's truncating 0x0E7F control word.
    DamageDelayTimer.Start(game::x87_integer(game::x87_truncated_product(RulesClass::Instance->DamageDelay, 900.0)));
    TeamDelayTimer.Start(1);
    TriggerDelayTimer.Start(0);
    SpeakAttackDelayTimer.Start(1);
    SpeakPowerDelayTimer.Start(1);
    SpeakMoneyDelayTimer.Start(1);
    SpeakMaxedDelayTimer.Start(1);
    Create_ID();
    if (Type) ColorSchemeIndex = Type->ColorSchemeIndex;
    // 0x4F5E60..0x4F6001 appends to the Type, Factory, Waypoint, Tag and Object
    // receiver tables. Type has no House consumer in the native implementation.
    // A failed append leaves that table unchanged.
    PointerExpiredNotification::NotifyInvalidFactory.Array.AddItem(this);
    PointerExpiredNotification::NotifyInvalidWaypoint.Array.AddItem(this);
    TagExpirationListeners.AddItem(this);
    AbstractClass::Array.AddItem(this);
    ArrayIndex = Array.Count;
    // 0x4F6013..0x4F6196: a typed house enters both relation tables of every
    // existing house, and each existing house enters this house's tables.
    if (Type)
        for (int i = 0; i < Array.Count; ++i) {
            Array[i]->AngerNodes.AddItem(AngerStruct{this, 0});
            AngerNodes.AddItem(AngerStruct{Array[i], 0});
            Array[i]->ScoutNodes.AddItem(ScoutStruct{this, false});
            ScoutNodes.AddItem(ScoutStruct{Array[i], false});
        }
    Array.AddItem(this);
    // 0x4F6208..0x4F6290: one instance per registered type, in type-list order.
    // The nullable allocation stores a null entry when it fails.
    for (auto* type : SuperWeaponTypeClass::Array) {
        SuperClass* super = nullptr;
        if (void* storage = YRMemory::Allocate(sizeof(SuperClass))) super = ::new (storage) SuperClass(type, this);
        Supers.AddItem(super);
    }
    if (Type) {
        std::strncpy(PlainName, Type->ID, 20);
        PlainName[20] = 0;
        // 0x4F630A copies without a bound: a longer country name continues into the
        // following members of this object, as in the original.
        auto* target = reinterpret_cast<unsigned char*>(UIName);
        const auto room = static_cast<std::size_t>(reinterpret_cast<unsigned char*>(this) + sizeof(HouseClass) - target);
        const std::size_t bytes = (std::wcslen(Type->UIName) + 1) * sizeof(wchar_t);
        std::memcpy(target, Type->UIName, bytes < room ? bytes : room);
    }
    // 0x4F633D sets the house bit in the +0x1D8 alliance field, not in Allies.
    AltAllies.Add(ArrayIndex);
    // 0x4F634E draws the first attack delay from the scenario generator.
    const int minutes = ScenarioClass::Instance->Random.RandomRanged(450, 1800);
    const int delay = game::x87_integer(game::x87_truncated_product(minutes, RulesClass::Instance->AttackDelay));
    AttackTimer.Start(delay);
    InitialAttackDelay = delay;
    BuiltAircraftTypes.PopulateUnitCount(AircraftTypeClass::Array.Count);
    BuiltInfantryTypes.PopulateUnitCount(InfantryTypeClass::Array.Count);
    BuiltUnitTypes.PopulateUnitCount(UnitTypeClass::Array.Count);
    BuiltBuildingTypes.PopulateUnitCount(BuildingTypeClass::Array.Count);
    KilledAircraftTypes.PopulateUnitCount(AircraftTypeClass::Array.Count);
    KilledInfantryTypes.PopulateUnitCount(InfantryTypeClass::Array.Count);
    KilledUnitTypes.PopulateUnitCount(UnitTypeClass::Array.Count);
    KilledBuildingTypes.PopulateUnitCount(BuildingTypeClass::Array.Count);
    CapturedBuildings.PopulateUnitCount(BuildingTypeClass::Array.Count);
    CollectedCrates.PopulateUnitCount(19);
    if (Type) {
        HasThreatNode = true;
        if (Type->SideIndex >= 0 && Type->SideIndex <= 2) SideIndex = Type->SideIndex;
    }
    // 0x4F6494..0x4F652B registers the external AI connection point. Native
    // houses expose no COM connection points, so WaypointPath stays empty.
}
HouseClass::~HouseClass() {
    // 0x4F7179 announces while Type, owned objects and receiver entries are live.
    NotifyObjectExpired(true);
    Type = nullptr;
    for (auto*& path : PlanningPaths) {
        GameDelete(path);
        path = nullptr;
    }
    for (int i = 0; i < Supers.Count; ++i)
        GameDelete(Supers[i]);
    Supers.Clear();
    // Tag destruction broadcasts back to this House and compacts this live list.
    while (RelatedTags.Count > 0)
        GameDelete(RelatedTags[0]);
    for (int i = StartingUnits.Count - 1; i >= 0; --i)
        GameDelete(StartingUnits[i]);
    StartingUnits.Clear();
    // Native houses have no external AI COM connection or connection points.
    WaypointPath.Clear();
    unknown_1607C = 0;
    PointerExpiredNotification::NotifyInvalidWaypoint.Remove(this);
    PointerExpiredNotification::NotifyInvalidFactory.Remove(this);
    TagExpirationListeners.Remove(this);
    AbstractClass::Array.Remove(this);
    if (CurrentPlayer == this) CurrentPlayer = nullptr;
    if (Observer == this) Observer = nullptr;
    Array.Remove(this);
    WaypointPath.~DynamicVectorClass();
    Base.~BaseClass();
    for (int i = 2; i >= 0; --i)
        DropshipData[i].~DropshipStruct();
    ScoutNodes.~DynamicVectorClass();
    AngerNodes.~DynamicVectorClass();
    FactoryProducedAircraftTypes.~CounterClass();
    FactoryProducedInfantryTypes.~CounterClass();
    FactoryProducedUnitTypes.~CounterClass();
    FactoryProducedBuildingTypes.~CounterClass();
    ActiveAircraftTypes.~CounterClass();
    ActiveInfantryTypes.~CounterClass();
    ActiveUnitTypes.~CounterClass();
    ActiveBuildingTypes.~CounterClass();
    OwnedAircraftTypes.~CounterClass();
    OwnedInfantryTypes.~CounterClass();
    OwnedUnitTypes.~CounterClass();
    OwnedBuildingTypes.~CounterClass();
    CollectedCrates.~UnitTrackerClass();
    CapturedBuildings.~UnitTrackerClass();
    KilledBuildingTypes.~UnitTrackerClass();
    KilledUnitTypes.~UnitTrackerClass();
    KilledInfantryTypes.~UnitTrackerClass();
    KilledAircraftTypes.~UnitTrackerClass();
    BuiltBuildingTypes.~UnitTrackerClass();
    BuiltUnitTypes.~UnitTrackerClass();
    BuiltInfantryTypes.~UnitTrackerClass();
    BuiltAircraftTypes.~UnitTrackerClass();
    Supers.~DynamicVectorClass();
    StartingUnits.~DynamicVectorClass();
    FactoryPlants.~DynamicVectorClass();
    PsychicDetectionBuildings.~DynamicVectorClass();
    SecretLabs.~DynamicVectorClass();
    CloningVats.~DynamicVectorClass();
    Occupiables.~DynamicVectorClass();
    Bunkers.~DynamicVectorClass();
    Absorbers.~DynamicVectorClass();
    Grinders.~DynamicVectorClass();
    UnitRepairStations.~DynamicVectorClass();
    Buildings.~DynamicVectorClass();
    ConYards.~DynamicVectorClass();
    RelatedTags.~DynamicVectorClass();
}
