/*
 * Adapts EA REDALERT/RULES.CPP (MPlayer, Heap_Maximums), revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
 * Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later with the additional
 * terms in third_party/ea/LICENSE.TXT. YR fields, read order and returns are
 * calibrated to 66D150, 66D1F0, 671EA0, 672230 and 6743D0.
 */
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "RulesClassReaders.hpp"

bool RulesClass::Read_ElevationModel(CCINIClass* ini) {
    const char* section = "ElevationModel";
    if (!ini || !ini->GetSection(section)) return false;
    ElevationIncrement = ini->ReadInteger(section, "ElevationIncrement", ElevationIncrement);
    ElevationIncrementBonus = ini->ReadDouble(section, "ElevationIncrementBonus", ElevationIncrementBonus);
    ElevationBonusCap = ini->ReadDouble(section, "ElevationBonusCap", ElevationBonusCap);
    return true;
}

bool RulesClass::Read_WallModel(CCINIClass* ini) {
    const char* section = "WallModel";
    if (!ini || !ini->GetSection(section)) return false;
    AlliedWallTransparency = ini->ReadBool(section, "AlliedWallTransparency", AlliedWallTransparency);
    WallPenetratorThreshold = ini->ReadDouble(section, "WallPenetratorThreshold", WallPenetratorThreshold);
    return true;
}

bool RulesClass::Read_MultiplayerDialogSettings(CCINIClass* ini) {
    const char* section = "MultiplayerDialogSettings";
    if (!ini || !ini->GetSection(section)) return false;
    MinMoney = ini->ReadInteger(section, "MinMoney", MinMoney);
    Money = ini->ReadInteger(section, "Money", Money);
    MaxMoney = ini->ReadInteger(section, "MaxMoney", MaxMoney);
    MoneyIncrement = ini->ReadInteger(section, "MoneyIncrement", MoneyIncrement);
    MinUnitCount = ini->ReadInteger(section, "MinUnitCount", MinUnitCount);
    UnitCount = ini->ReadInteger(section, "UnitCount", UnitCount);
    MaxUnitCount = ini->ReadInteger(section, "MaxUnitCount", MaxUnitCount);
    TechLevel = ini->ReadInteger(section, "TechLevel", TechLevel);
    GameSpeed = ini->ReadInteger(section, "GameSpeed", GameSpeed);
    AIDifficultyStruct = ini->ReadInteger(section, "AIDifficulty", AIDifficultyStruct);
    AIPlayers = ini->ReadInteger(section, "AIPlayers", AIPlayers);
    BridgeDestruction = ini->ReadBool(section, "BridgeDestruction", BridgeDestruction);
    ShadowGrow = ini->ReadBool(section, "ShadowGrow", ShadowGrow);
    Shroud = ini->ReadBool(section, "Shroud", Shroud);
    Bases = ini->ReadBool(section, "Bases", Bases);
    TiberiumGrows = ini->ReadBool(section, "TiberiumGrows", TiberiumGrows);
    Crates = ini->ReadBool(section, "Crates", Crates);
    CaptureTheFlag = ini->ReadBool(section, "CaptureTheFlag", CaptureTheFlag);
    HarvesterTruce = ini->ReadBool(section, "HarvesterTruce", HarvesterTruce);
    MultiEngineer = ini->ReadBool(section, "MultiEngineer", MultiEngineer);
    AlliesAllowed = ini->ReadBool(section, "AlliesAllowed", AlliesAllowed);
    AllyChangeAllowed = ini->ReadBool(section, "AllyChangeAllowed", AllyChangeAllowed);
    ShortGame = ini->ReadBool(section, "ShortGame", ShortGame);
    SuperWeaponsAllowed = ini->ReadBool(section, "SuperWeaponsAllowed", SuperWeaponsAllowed);
    BuildOffAlly = ini->ReadBool(section, "BuildOffAlly", BuildOffAlly);
    FogOfWar = ini->ReadBool(section, "FogOfWar", FogOfWar);
    MCVRedeploys = ini->ReadBool(section, "MCVRedeploys", MCVRedeploys);
    return true;
}

bool RulesClass::Read_Maximums(CCINIClass* ini) {
    if (!ini) return false;
    if (ini->GetSection("Maximums"))
        Players = ini->ReadInteger("Maximums", "Players", Players);
    // 672230 returns true even when the section is absent.
    return true;
}

bool RulesClass::Read_JumpjetControls(CCINIClass* ini) {
    const char* section = "JumpjetControls";
    if (!ini || !ini->GetSection(section)) return false;
    TurnRate = ini->ReadInteger(section, "TurnRate", TurnRate);
    Speed = ini->ReadInteger(section, "Speed", Speed);
    Climb = ini->ReadDouble(section, "Climb", Climb);
    CruiseHeight = ini->ReadInteger(section, "CruiseHeight", CruiseHeight);
    Acceleration = ini->ReadDouble(section, "Acceleration", Acceleration);
    WobblesPerSecond = ini->ReadDouble(section, "WobblesPerSecond", WobblesPerSecond);
    WobbleDeviation = ini->ReadInteger(section, "WobbleDeviation", WobbleDeviation);
    return true;
}

// YR 66D530. Primitive fields, original type lists and all target conversion quirks.
bool RulesClass::Read_DamageFireTypes(CCINIClass* ini) noexcept {
    try {
        if (!ini || !ini->GetSection("General")) return false;
        read_rule_type_list(*ini, "General", "DamageFireTypes", AbstractType::AnimType, DamageFireTypes);
        return true;
    } catch (...) { return false; }
}

bool RulesClass::Read_General(CCINIClass* ini) {
    const char* section = "General";
    if (!ini || !ini->GetSection(section)) return false;
    read_rule_type_list(*ini, section, "DamageFireTypes", AbstractType::AnimType, DamageFireTypes);
    read_rule_type(*ini, section, "OreTwinkle", AbstractType::AnimType, OreTwinkle);
    read_rule_type(*ini, section, "BarrelExplode", AbstractType::AnimType, BarrelExplode);
    read_rule_type_list(*ini, section, "BarrelDebris", AbstractType::VoxelAnimType, BarrelDebris);
    read_rule_type(*ini, section, "BarrelParticle", AbstractType::ParticleSystemType, BarrelParticle);
    read_rule_type(*ini, section, "NukeTakeOff", AbstractType::AnimType, NukeTakeOff);
    read_rule_type(*ini, section, "Wake", AbstractType::AnimType, Wake);
    read_rule_type_list(*ini, section, "DropPod", AbstractType::AnimType, DropPod);
    read_rule_type_list(*ini, section, "DeadBodies", AbstractType::AnimType, DeadBodies);
    read_rule_type_list(*ini, section, "MetallicDebris", AbstractType::AnimType, MetallicDebris);
    read_rule_type_list(*ini, section, "BridgeExplosions", AbstractType::AnimType, BridgeExplosions);
    read_rule_type(*ini, section, "IonBlast", AbstractType::AnimType, IonBlast);
    read_rule_type(*ini, section, "IonBeam", AbstractType::AnimType, IonBeam);
    read_rule_type_list(*ini, section, "WeatherConClouds", AbstractType::AnimType, WeatherConClouds);
    read_rule_type_list(*ini, section, "WeatherConBolts", AbstractType::AnimType, WeatherConBolts);
    read_rule_type(*ini, section, "WeatherConBoltExplosion", AbstractType::AnimType, WeatherConBoltExplosion);
    read_rule_type(*ini, section, "DominatorWarhead", AbstractType::WarheadType, DominatorWarhead);
    read_rule_type(*ini, section, "DominatorFirstAnim", AbstractType::AnimType, DominatorFirstAnim);
    read_rule_type(*ini, section, "DominatorSecondAnim", AbstractType::AnimType, DominatorSecondAnim);
    DominatorFireAtPercentage = ini->ReadInteger(section, "DominatorFireAtPercentage", DominatorFireAtPercentage);
    DominatorCaptureRange = ini->ReadInteger(section, "DominatorCaptureRange", DominatorCaptureRange);
    DominatorDamage = ini->ReadInteger(section, "DominatorDamage", DominatorDamage);
    read_rule_type(*ini, section, "ChronoPlacement", AbstractType::AnimType, ChronoPlacement);
    read_rule_type(*ini, section, "ChronoBeam", AbstractType::AnimType, ChronoBeam);
    read_rule_type(*ini, section, "ChronoBlast", AbstractType::AnimType, ChronoBlast);
    read_rule_type(*ini, section, "ChronoBlastDest", AbstractType::AnimType, ChronoBlastDest);
    read_rule_type(*ini, section, "WarpIn", AbstractType::AnimType, WarpIn);
    read_rule_type(*ini, section, "WarpOut", AbstractType::AnimType, WarpOut);
    read_rule_type(*ini, section, "WarpAway", AbstractType::AnimType, WarpAway);
    read_rule_type(*ini, section, "IronCurtainInvokeAnim", AbstractType::AnimType, IronCurtainInvokeAnim);
    read_rule_type(*ini, section, "ForceShieldInvokeAnim", AbstractType::AnimType, ForceShieldInvokeAnim);
    read_rule_type(*ini, section, "WeaponNullifyAnim", AbstractType::AnimType, WeaponNullifyAnim);
    read_rule_type(*ini, section, "ChronoSparkle1", AbstractType::AnimType, ChronoSparkle1);
    read_rule_type(*ini, section, "InfantryExplode", AbstractType::AnimType, InfantryExplode);
    read_rule_type(*ini, section, "FlamingInfantry", AbstractType::AnimType, FlamingInfantry);
    read_rule_type(*ini, section, "InfantryHeadPop", AbstractType::AnimType, InfantryHeadPop);
    read_rule_type(*ini, section, "InfantryNuked", AbstractType::AnimType, InfantryNuked);
    read_rule_type(*ini, section, "InfantryVirus", AbstractType::AnimType, InfantryVirus);
    read_rule_type(*ini, section, "InfantryBrute", AbstractType::AnimType, InfantryBrute);
    read_rule_type(*ini, section, "InfantryMutate", AbstractType::AnimType, InfantryMutate);
    read_rule_type(*ini, section, "Behind", AbstractType::AnimType, Behind);
    read_rule_type(*ini, section, "MoveFlash", AbstractType::AnimType, MoveFlash);
    read_rule_type(*ini, section, "Parachute", AbstractType::AnimType, Parachute);
    read_rule_type(*ini, section, "BombParachute", AbstractType::AnimType, BombParachute);
    read_rule_type(*ini, section, "DropZoneAnim", AbstractType::AnimType, DropZoneAnim);
    read_rule_type(*ini, section, "EMPulseSparkles", AbstractType::AnimType, EMPulseSparkles);
    read_rule_type(*ini, section, "LargeVisceroid", AbstractType::UnitType, LargeVisceroid);
    read_rule_type(*ini, section, "SmallVisceroid", AbstractType::UnitType, SmallVisceroid);
    TiberiumHeal = ini->ReadDouble(section, "TiberiumHeal", TiberiumHeal);
    SelfHealInfantryFrames = ini->ReadInteger(section, "SelfHealInfantryFrames", SelfHealInfantryFrames);
    SelfHealInfantryAmount = ini->ReadInteger(section, "SelfHealInfantryAmount", SelfHealInfantryAmount);
    SelfHealUnitFrames = ini->ReadInteger(section, "SelfHealUnitFrames", SelfHealUnitFrames);
    SelfHealUnitAmount = ini->ReadInteger(section, "SelfHealUnitAmount", SelfHealUnitAmount);
    read_rule_prerequisites(*ini, section, "PrerequisitePower", PrerequisitePower);
    read_rule_prerequisites(*ini, section, "PrerequisiteFactory", PrerequisiteFactory);
    read_rule_prerequisites(*ini, section, "PrerequisiteBarracks", PrerequisiteBarracks);
    read_rule_prerequisites(*ini, section, "PrerequisiteRadar", PrerequisiteRadar);
    read_rule_prerequisites(*ini, section, "PrerequisiteTech", PrerequisiteTech);
    read_rule_prerequisites(*ini, section, "PrerequisiteProc", PrerequisiteProc);
    ZoomInFactor = ini->ReadDouble(section, "ZoomInFactor", ZoomInFactor);
    RevealByHeight = ini->ReadBool(section, "RevealByHeight", RevealByHeight);
    AllowShroudedSubteranneanMoves = ini->ReadBool(section, "AllowShroudedSubteranneanMoves", AllowShroudedSubteranneanMoves);
    AircraftFogReveal = ini->ReadInteger(section, "AircraftFogReveal", AircraftFogReveal);
    MinLowPowerProductionSpeed = static_cast<float>(ini->ReadDouble(section, "MinLowPowerProductionSpeed", MinLowPowerProductionSpeed));
    MaxLowPowerProductionSpeed = static_cast<float>(ini->ReadDouble(section, "MaxLowPowerProductionSpeed", MaxLowPowerProductionSpeed));
    LowPowerPenaltyModifier = static_cast<float>(ini->ReadDouble(section, "LowPowerPenaltyModifier", LowPowerPenaltyModifier));
    MultipleFactory = static_cast<float>(ini->ReadDouble(section, "MultipleFactory", MultipleFactory));
    MaximumCheerRate = ini->ReadInteger(section, "MaximumCheerRate", MaximumCheerRate);
    TreeFlammability = ini->ReadDouble(section, "TreeFlammability", TreeFlammability);
    MissileROTVar = ini->ReadDouble(section, "MissileROTVar", MissileROTVar);
    MissileSafetyAltitude = ini->ReadInteger(section, "MissileSafetyAltitude", MissileSafetyAltitude);
    MissileSpeedVar = ini->ReadDouble(section, "MissileSpeedVar", MissileSpeedVar);
    read_rule_type(*ini, section, "DropPodWeapon", AbstractType::WeaponType, DropPodWeapon);
    DropPodHeight = ini->ReadInteger(section, "DropPodHeight", DropPodHeight);
    DropPodSpeed = ini->ReadInteger(section, "DropPodSpeed", DropPodSpeed);
    DropPodAngle = ini->ReadDouble(section, "DropPodAngle", DropPodAngle);
    if (DropPodAngle >= 1.1780972450961724) DropPodAngle = 1.1780972450961724;
    if (DropPodAngle <= 0.39269908169872414) DropPodAngle = 0.39269908169872414;
    CrewEscape = ini->ReadDouble(section, "CrewEscape", CrewEscape);
    TunnelSpeed = ini->ReadDouble(section, "TunnelSpeed", TunnelSpeed);
    HoverDampen = ini->ReadDouble(section, "HoverDampen", HoverDampen);
    HoverBob = ini->ReadDouble(section, "HoverBob", HoverBob);
    HoverHeight = ini->ReadInteger(section, "HoverHeight", HoverHeight);
    HoverBoost = ini->ReadDouble(section, "HoverBoost", HoverBoost);
    HoverAcceleration = ini->ReadDouble(section, "HoverAcceleration", HoverAcceleration);
    HoverBrake = ini->ReadDouble(section, "HoverBrake", HoverBrake);
    VeteranRatio = ini->ReadDouble(section, "VeteranRatio", VeteranRatio);
    VeteranCombat = ini->ReadDouble(section, "VeteranCombat", VeteranCombat);
    VeteranSpeed = ini->ReadDouble(section, "VeteranSpeed", VeteranSpeed);
    VeteranSight = ini->ReadDouble(section, "VeteranSight", VeteranSight);
    VeteranArmor = ini->ReadDouble(section, "VeteranArmor", VeteranArmor);
    VeteranROF = ini->ReadDouble(section, "VeteranROF", VeteranROF);
    VeteranCap = ini->ReadDouble(section, "VeteranCap", VeteranCap);
    read_rule_type_list(*ini, section, "ExplosiveVoxelDebris", AbstractType::VoxelAnimType, ExplosiveVoxelDebris);
    BridgeVoxelMax = ini->ReadInteger(section, "BridgeVoxelMax", BridgeVoxelMax);
    read_rule_type(*ini, section, "TireVoxelDebris", AbstractType::VoxelAnimType, TireVoxelDebris);
    read_rule_type(*ini, section, "ScrapVoxelDebris", AbstractType::VoxelAnimType, ScrapVoxelDebris);
    CloakingStages = ini->ReadInteger(section, "CloakingStages", CloakingStages);
    ShipSinkingWeight = ini->ReadDouble(section, "ShipSinkingWeight", ShipSinkingWeight);
    IceCrackingWeight = ini->ReadDouble(section, "IceCrackingWeight", IceCrackingWeight);
    IceBreakingWeight = ini->ReadDouble(section, "IceBreakingWeight", IceBreakingWeight);
    CliffBackImpassability = static_cast<byte>(ini->ReadInteger(section, "CliffBackImpassability", CliffBackImpassability));
    PlacementDelay = ini->ReadDouble(section, "PlacementDelay", PlacementDelay);
    TrackedUphill = ini->ReadDouble(section, "TrackedUphill", TrackedUphill);
    TrackedDownhill = ini->ReadDouble(section, "TrackedDownhill", TrackedDownhill);
    WheeledUphill = ini->ReadDouble(section, "WheeledUphill", WheeledUphill);
    WheeledDownhill = ini->ReadDouble(section, "WheeledDownhill", WheeledDownhill);
    WindDirection = ini->ReadInteger(section, "WindDirection", WindDirection);
    CameraRange = read_rule_distance(*ini, section, "CameraRange", CameraRange);
    FlightLevel = ini->ReadInteger(section, "FlightLevel", FlightLevel);
    ParachuteMaxFallRate = ini->ReadInteger(section, "ParachuteMaxFallRate", ParachuteMaxFallRate);
    NoParachuteMaxFallRate = ini->ReadInteger(section, "NoParachuteMaxFallRate", NoParachuteMaxFallRate);
    read_rule_type_list(*ini, section, "RepairBay", AbstractType::BuildingType, RepairBay);
    read_rule_type(*ini, section, "GDIGateOne", AbstractType::BuildingType, GDIGateOne);
    read_rule_type(*ini, section, "GDIGateTwo", AbstractType::BuildingType, GDIGateTwo);
    read_rule_type(*ini, section, "NodGateOne", AbstractType::BuildingType, NodGateOne);
    read_rule_type(*ini, section, "NodGateTwo", AbstractType::BuildingType, NodGateTwo);
    read_rule_type(*ini, section, "WallTower", AbstractType::BuildingType, WallTower);
    read_rule_type_list(*ini, section, "Shipyard", AbstractType::BuildingType, Shipyard);
    read_rule_type(*ini, section, "GDIPowerPlant", AbstractType::BuildingType, GDIPowerPlant);
    read_rule_type(*ini, section, "NodRegularPower", AbstractType::BuildingType, NodRegularPower);
    read_rule_type(*ini, section, "NodAdvancedPower", AbstractType::BuildingType, NodAdvancedPower);
    read_rule_type(*ini, section, "ThirdPowerPlant", AbstractType::BuildingType, ThirdPowerPlant);
    read_rule_type(*ini, section, "PrerequisiteProcAlternate", AbstractType::UnitType, PrerequisiteProcAlternate);
    read_rule_type_list(*ini, section, "BaseUnit", AbstractType::UnitType, BaseUnit);
    read_rule_type_list(*ini, section, "HarvesterUnit", AbstractType::UnitType, HarvesterUnit);
    read_rule_type_list(*ini, section, "PadAircraft", AbstractType::AircraftType, PadAircraft);
    read_rule_type(*ini, section, "Paratrooper", AbstractType::InfantryType, Paratrooper);
    read_rule_type_list(*ini, section, "SecretInfantry", AbstractType::InfantryType, SecretInfantry);
    read_rule_type_list(*ini, section, "SecretUnits", AbstractType::UnitType, SecretUnits);
    read_rule_type_list(*ini, section, "SecretBuildings", AbstractType::BuildingType, SecretBuildings);
    SecretSum = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(SecretInfantry.Count) +
        static_cast<std::uint32_t>(SecretUnits.Count) + static_cast<std::uint32_t>(SecretBuildings.Count));
    ChronoDelay = ini->ReadInteger(section, "ChronoDelay", ChronoDelay);
    ChronoReinfDelay = ini->ReadInteger(section, "ChronoReinfDelay", ChronoReinfDelay);
    ChronoDistanceFactor = ini->ReadInteger(section, "ChronoDistanceFactor", ChronoDistanceFactor);
    ChronoTrigger = ini->ReadBool(section, "ChronoTrigger", ChronoTrigger);
    ChronoMinimumDelay = ini->ReadInteger(section, "ChronoMinimumDelay", ChronoMinimumDelay);
    ChronoRangeMinimum = ini->ReadInteger(section, "ChronoRangeMinimum", ChronoRangeMinimum);
    read_rule_type(*ini, section, "AlliedDisguise", AbstractType::InfantryType, AlliedDisguise);
    read_rule_type(*ini, section, "SovietDisguise", AbstractType::InfantryType, SovietDisguise);
    read_rule_type(*ini, section, "ThirdDisguise", AbstractType::InfantryType, ThirdDisguise);
    SpyPowerBlackout = ini->ReadInteger(section, "SpyPowerBlackout", SpyPowerBlackout);
    SpyMoneyStealPercent = static_cast<float>(ini->ReadDouble(section, "SpyMoneyStealPercent", SpyMoneyStealPercent));
    AttackCursorOnDisguise = ini->ReadBool(section, "AttackCursorOnDisguise", AttackCursorOnDisguise);
    PurifierBonus = static_cast<float>(ini->ReadDouble(section, "PurifierBonus", PurifierBonus));
    read_rule_type(*ini, section, "Engineer", AbstractType::InfantryType, Engineer);
    read_rule_type(*ini, section, "Technician", AbstractType::InfantryType, Technician);
    read_rule_type(*ini, section, "Pilot", AbstractType::InfantryType, Pilot);
    read_rule_type(*ini, section, "AlliedCrew", AbstractType::InfantryType, AlliedCrew);
    read_rule_type(*ini, section, "SovietCrew", AbstractType::InfantryType, SovietCrew);
    read_rule_type(*ini, section, "ThirdCrew", AbstractType::InfantryType, ThirdCrew);
    CurleyShuffle = ini->ReadBool(section, "CurleyShuffle", CurleyShuffle);
    FineDiffControl = ini->ReadBool(section, "FineDiffControl", FineDiffControl);
    read_rule_integer_list(*ini, section, "TeamDelays", TeamDelays);
    read_rule_integer_list(*ini, section, "AIHateDelays", AIHateDelays);
    AIAlternateProductionCreditCutoff = ini->ReadInteger(section, "AIAlternateProductionCreditCutoff", AIAlternateProductionCreditCutoff);
    AIUseTurbineUpgradeProbability = ini->ReadDouble(section, "AIUseTurbineUpgradeProbability", AIUseTurbineUpgradeProbability);
    NodAIBuildsWalls = ini->ReadBool(section, "NodAIBuildsWalls", NodAIBuildsWalls);
    AIBuildsWalls = ini->ReadBool(section, "AIBuildsWalls", AIBuildsWalls);
    read_rule_integer_list(*ini, section, "FillEarliestTeamProbability", FillEarliestTeamProbability);
    read_rule_integer_list(*ini, section, "MinimumAIDefensiveTeams", MinimumAIDefensiveTeams);
    read_rule_integer_list(*ini, section, "MaximumAIDefensiveTeams", MaximumAIDefensiveTeams);
    read_rule_integer_list(*ini, section, "TotalAITeamCap", TotalAITeamCap);
    UseMinDefenseRule = ini->ReadBool(section, "UseMinDefenseRule", UseMinDefenseRule);
    DissolveUnfilledTeamDelay = ini->ReadInteger(section, "DissolveUnfilledTeamDelay", DissolveUnfilledTeamDelay);
    AISafeDistance = ini->ReadInteger(section, "AISafeDistance", AISafeDistance);
    AIMinorSuperReadyPercent = static_cast<float>(ini->ReadDouble(section, "AIMinorSuperReadyPercent", AIMinorSuperReadyPercent));
    HarvesterTooFarDistance = ini->ReadInteger(section, "HarvesterTooFarDistance", HarvesterTooFarDistance);
    ChronoHarvTooFarDistance = ini->ReadInteger(section, "ChronoHarvTooFarDistance", ChronoHarvTooFarDistance);
    read_rule_integer_list(*ini, section, "AlliedBaseDefenseCounts", AlliedBaseDefenseCounts);
    read_rule_integer_list(*ini, section, "SovietBaseDefenseCounts", SovietBaseDefenseCounts);
    read_rule_integer_list(*ini, section, "ThirdBaseDefenseCounts", ThirdBaseDefenseCounts);
    read_rule_integer_list(*ini, section, "AIPickWallDefensePercent", AIPickWallDefensePercent);
    AIRestrictReplaceTime = ini->ReadInteger(section, "AIRestrictReplaceTime", AIRestrictReplaceTime);
    ThreatPerOccupant = ini->ReadInteger(section, "ThreatPerOccupant", ThreatPerOccupant);
    ApproachTargetResetMultiplier = ini->ReadInteger(section, "ApproachTargetResetMultiplier", ApproachTargetResetMultiplier);
    CampaignMoneyDeltaEasy = ini->ReadInteger(section, "CampaignMoneyDeltaEasy", CampaignMoneyDeltaEasy);
    CampaignMoneyDeltaHard = ini->ReadInteger(section, "CampaignMoneyDeltaHard", CampaignMoneyDeltaHard);
    GuardAreaTargetingDelay = ini->ReadInteger(section, "GuardAreaTargetingDelay", GuardAreaTargetingDelay);
    NormalTargetingDelay = ini->ReadInteger(section, "NormalTargetingDelay", NormalTargetingDelay);
    AINavalYardAdjacency = ini->ReadInteger(section, "AINavalYardAdjacency", AINavalYardAdjacency);
    read_rule_integer_list(*ini, section, "DisabledDisguiseDetectionPercent", DisabledDisguiseDetectionPercent);
    read_rule_integer_list(*ini, section, "AIAutoDeployFrameDelay", AIAutoDeployFrameDelay);
    MaximumBuildingPlacementFailures = ini->ReadInteger(section, "MaximumBuildingPlacementFailures", MaximumBuildingPlacementFailures);
    TiberiumShortScan = read_rule_distance(*ini, section, "TiberiumShortScan", TiberiumShortScan);
    TiberiumLongScan = read_rule_distance(*ini, section, "TiberiumLongScan", TiberiumLongScan);
    SlaveMinerShortScan = read_rule_distance(*ini, section, "SlaveMinerShortScan", SlaveMinerShortScan);
    SlaveMinerSlaveScan = read_rule_distance(*ini, section, "SlaveMinerSlaveScan", SlaveMinerSlaveScan);
    SlaveMinerLongScan = read_rule_distance(*ini, section, "SlaveMinerLongScan", SlaveMinerLongScan);
    SlaveMinerScanCorrection = read_rule_distance(*ini, section, "SlaveMinerScanCorrection", SlaveMinerScanCorrection);
    SlaveMinerKickFrameDelay = ini->ReadInteger(section, "SlaveMinerKickFrameDelay", SlaveMinerKickFrameDelay);
    read_rule_integer_list(*ini, section, "AISuperDefenseProbability", AISuperDefenseProbability);
    AISuperDefenseFrames = ini->ReadInteger(section, "AISuperDefenseFrames", AISuperDefenseFrames);
    AISuperDefenseDistance = read_rule_distance(*ini, section, "AISuperDefenseDistance", AISuperDefenseDistance);
    read_rule_integer_list(*ini, section, "AICaptureNormal", AICaptureNormal);
    read_rule_integer_list(*ini, section, "AICaptureWounded", AICaptureWounded);
    read_rule_integer_list(*ini, section, "AICaptureLowPower", AICaptureLowPower);
    read_rule_integer_list(*ini, section, "AICaptureLowMoney", AICaptureLowMoney);
    AICaptureLowMoneyMark = ini->ReadInteger(section, "AICaptureLowMoneyMark", AICaptureLowMoneyMark);
    AICaptureWoundedMark = static_cast<float>(ini->ReadDouble(section, "AICaptureWoundedMark", AICaptureWoundedMark));
    read_rule_integer_list(*ini, section, "MultiplayerAICM", MultiplayerAICM);
    read_rule_integer_list(*ini, section, "AIVirtualPurifiers", AIVirtualPurifiers);
    read_rule_integer_list(*ini, section, "AISlaveMinerNumber", AISlaveMinerNumber);
    read_rule_integer_list(*ini, section, "HarvestersPerRefinery", HarvestersPerRefinery);
    read_rule_integer_list(*ini, section, "AIExtraRefineries", AIExtraRefineries);
    read_rule_type_list(*ini, section, "AmerParaDropInf", AbstractType::InfantryType, AmerParaDropInf);
    read_rule_integer_list(*ini, section, "AmerParaDropNum", AmerParaDropNum);
    read_rule_type_list(*ini, section, "AllyParaDropInf", AbstractType::InfantryType, AllyParaDropInf);
    read_rule_integer_list(*ini, section, "AllyParaDropNum", AllyParaDropNum);
    read_rule_type_list(*ini, section, "SovParaDropInf", AbstractType::InfantryType, SovParaDropInf);
    read_rule_integer_list(*ini, section, "SovParaDropNum", SovParaDropNum);
    read_rule_type_list(*ini, section, "YuriParaDropInf", AbstractType::InfantryType, YuriParaDropInf);
    read_rule_integer_list(*ini, section, "YuriParaDropNum", YuriParaDropNum);
    read_rule_type_list(*ini, section, "AnimToInfantry", AbstractType::InfantryType, AnimToInfantry);
    read_rule_integer_list(*ini, section, "AIIonCannonConYardValue", AIIonCannonConYardValue);
    read_rule_integer_list(*ini, section, "AIIonCannonWarFactoryValue", AIIonCannonWarFactoryValue);
    read_rule_integer_list(*ini, section, "AIIonCannonPowerValue", AIIonCannonPowerValue);
    read_rule_integer_list(*ini, section, "AIIonCannonTechCenterValue", AIIonCannonTechCenterValue);
    read_rule_integer_list(*ini, section, "AIIonCannonEngineerValue", AIIonCannonEngineerValue);
    read_rule_integer_list(*ini, section, "AIIonCannonThiefValue", AIIonCannonThiefValue);
    read_rule_integer_list(*ini, section, "AIIonCannonHarvesterValue", AIIonCannonHarvesterValue);
    read_rule_integer_list(*ini, section, "AIIonCannonMCVValue", AIIonCannonMCVValue);
    read_rule_integer_list(*ini, section, "AIIonCannonAPCValue", AIIonCannonAPCValue);
    read_rule_integer_list(*ini, section, "AIIonCannonBaseDefenseValue", AIIonCannonBaseDefenseValue);
    read_rule_integer_list(*ini, section, "AIIonCannonPlugValue", AIIonCannonPlugValue);
    read_rule_integer_list(*ini, section, "AIIonCannonHelipadValue", AIIonCannonHelipadValue);
    read_rule_integer_list(*ini, section, "AIIonCannonTempleValue", AIIonCannonTempleValue);
    CloakDelay = ini->ReadDouble(section, "CloakDelay", CloakDelay);
    GameSpeedBias = ini->ReadDouble(section, "GameSpeedBias", GameSpeedBias);
    BaseBias = ini->ReadDouble(section, "BaseBias", BaseBias);
    SeparateAircraft = ini->ReadBool(section, "SeparateAircraft", SeparateAircraft);
    BaseDefenseDelay = ini->ReadDouble(section, "BaseDefenseDelay", BaseDefenseDelay);
    SuspendPriority = ini->ReadInteger(section, "SuspendPriority", SuspendPriority);
    SuspendDelay = ini->ReadDouble(section, "SuspendDelay", SuspendDelay);
    SurvivorRate = ini->ReadDouble(section, "SurvivorRate", SurvivorRate);
    AlliedSurvivorDivisor = ini->ReadInteger(section, "AlliedSurvivorDivisor", AlliedSurvivorDivisor);
    SovietSurvivorDivisor = ini->ReadInteger(section, "SovietSurvivorDivisor", SovietSurvivorDivisor);
    ThirdSurvivorDivisor = ini->ReadInteger(section, "ThirdSurvivorDivisor", ThirdSurvivorDivisor);
    ReloadRate = ini->ReadDouble(section, "ReloadRate", ReloadRate);
    BuildupTime = ini->ReadDouble(section, "BuildupTime", BuildupTime);
    HarvesterDumpRate = ini->ReadDouble(section, "HarvesterDumpRate", HarvesterDumpRate);
    HarvesterLoadRate = ini->ReadInteger(section, "HarvesterLoadRate", HarvesterLoadRate);
    BuildSpeed = ini->ReadDouble(section, "BuildSpeed", BuildSpeed);
    DamageDelay = ini->ReadDouble(section, "DamageDelay", DamageDelay);
    GrowthRate = ini->ReadDouble(section, "GrowthRate", GrowthRate);
    RefundPercent = ini->ReadDouble(section, "RefundPercent", RefundPercent);
    RepairPercent = ini->ReadDouble(section, "RepairPercent", RepairPercent);
    RepairStep = ini->ReadInteger(section, "RepairStep", RepairStep);
    IRepairStep = ini->ReadInteger(section, "IRepairStep", IRepairStep);
    RepairRate = ini->ReadDouble(section, "RepairRate", RepairRate);
    URepairRate = ini->ReadDouble(section, "URepairRate", URepairRate);
    IRepairRate = ini->ReadDouble(section, "IRepairRate", IRepairRate);
    Stray = read_rule_distance(*ini, section, "Stray", Stray);
    RelaxedStray = read_rule_distance(*ini, section, "RelaxedStray", RelaxedStray);
    GuardModeStray = read_rule_distance(*ini, section, "GuardModeStray", GuardModeStray);
    CloseEnough = read_rule_distance(*ini, section, "CloseEnough", CloseEnough);
    BlendedFog = ini->ReadBool(section, "BlendedFog", BlendedFog);
    AttackingAircraftSightRange = ini->ReadInteger(section, "AttackingAircraftSightRange", AttackingAircraftSightRange);
    LeptonsPerSightIncrease = ini->ReadInteger(section, "LeptonsPerSightIncrease", LeptonsPerSightIncrease);
    TiberiumTransmogrify = ini->ReadInteger(section, "TiberiumTransmogrify", TiberiumTransmogrify);
    LightningDeferment = ini->ReadInteger(section, "LightningDeferment", LightningDeferment);
    LightningDamage = ini->ReadInteger(section, "LightningDamage", LightningDamage);
    LightningStormDuration = ini->ReadInteger(section, "LightningStormDuration", LightningStormDuration);
    LightningHitDelay = ini->ReadInteger(section, "LightningHitDelay", LightningHitDelay);
    LightningScatterDelay = ini->ReadInteger(section, "LightningScatterDelay", LightningScatterDelay);
    LightningCellSpread = ini->ReadInteger(section, "LightningCellSpread", LightningCellSpread);
    LightningSeparation = ini->ReadInteger(section, "LightningSeparation", LightningSeparation);
    read_rule_type(*ini, section, "LightningWarhead", AbstractType::WarheadType, LightningWarhead);
    LightningPrintText = ini->ReadBool(section, "LightningPrintText", LightningPrintText);
    ForceShieldRadius = ini->ReadInteger(section, "ForceShieldRadius", ForceShieldRadius);
    ForceShieldDuration = ini->ReadInteger(section, "ForceShieldDuration", ForceShieldDuration);
    ForceShieldBlackoutDuration = ini->ReadInteger(section, "ForceShieldBlackoutDuration", ForceShieldBlackoutDuration);
    ForceShieldPlayFadeSoundTime = ini->ReadInteger(section, "ForceShieldPlayFadeSoundTime", ForceShieldPlayFadeSoundTime);
    MutateExplosion = ini->ReadBool(section, "MutateExplosion", MutateExplosion);
    read_rule_type(*ini, section, "PrismType", AbstractType::BuildingType, PrismType);
    PrismSupportModifier = rule_scaled_integer(ini->ReadDouble(section, "PrismSupportModifier", PrismSupportModifier), 100);
    PrismSupportMax = ini->ReadInteger(section, "PrismSupportMax", PrismSupportMax);
    PrismSupportDelay = ini->ReadInteger(section, "PrismSupportDelay", PrismSupportDelay);
    PrismSupportDuration = ini->ReadInteger(section, "PrismSupportDuration", PrismSupportDuration);
    PrismSupportHeight = ini->ReadInteger(section, "PrismSupportHeight", PrismSupportHeight);
    V3Rocket.PauseFrames = ini->ReadInteger(section, "V3RocketPauseFrames", V3Rocket.PauseFrames);
    V3Rocket.TiltFrames = ini->ReadInteger(section, "V3RocketTiltFrames", V3Rocket.TiltFrames);
    V3Rocket.PitchInitial = static_cast<float>(ini->ReadDouble(section, "V3RocketPitchInitial", V3Rocket.PitchInitial));
    V3Rocket.PitchFinal = static_cast<float>(ini->ReadDouble(section, "V3RocketPitchFinal", V3Rocket.PitchFinal));
    V3Rocket.TurnRate = static_cast<float>(ini->ReadDouble(section, "V3RocketTurnRate", V3Rocket.TurnRate));
    V3Rocket.RaiseRate = rule_integer(ini->ReadDouble(section, "V3RocketRaiseRate", V3Rocket.RaiseRate));
    V3Rocket.Acceleration = static_cast<float>(ini->ReadDouble(section, "V3RocketAcceleration", V3Rocket.Acceleration));
    V3Rocket.Altitude = ini->ReadInteger(section, "V3RocketAltitude", V3Rocket.Altitude);
    V3Rocket.Damage = ini->ReadInteger(section, "V3RocketDamage", V3Rocket.Damage);
    V3Rocket.EliteDamage = ini->ReadInteger(section, "V3RocketEliteDamage", V3Rocket.EliteDamage);
    V3Rocket.BodyLength = ini->ReadInteger(section, "V3RocketBodyLength", V3Rocket.BodyLength);
    V3Rocket.LazyCurve = ini->ReadBool(section, "V3RocketLazyCurve", V3Rocket.LazyCurve);
    read_rule_type(*ini, section, "V3RocketType", AbstractType::AircraftType, V3Rocket.Type);
    DMisl.PauseFrames = ini->ReadInteger(section, "DMislPauseFrames", DMisl.PauseFrames);
    DMisl.TiltFrames = ini->ReadInteger(section, "DMislTiltFrames", DMisl.TiltFrames);
    DMisl.PitchInitial = static_cast<float>(ini->ReadDouble(section, "DMislPitchInitial", DMisl.PitchInitial));
    DMisl.PitchFinal = static_cast<float>(ini->ReadDouble(section, "DMislPitchFinal", DMisl.PitchFinal));
    DMisl.TurnRate = static_cast<float>(ini->ReadDouble(section, "DMislTurnRate", DMisl.TurnRate));
    DMisl.RaiseRate = rule_integer(ini->ReadDouble(section, "DMislRaiseRate", DMisl.RaiseRate));
    DMisl.Acceleration = static_cast<float>(ini->ReadDouble(section, "DMislAcceleration", DMisl.Acceleration));
    DMisl.Altitude = ini->ReadInteger(section, "DMislAltitude", DMisl.Altitude);
    DMisl.Damage = ini->ReadInteger(section, "DMislDamage", DMisl.Damage);
    DMisl.EliteDamage = ini->ReadInteger(section, "DMislEliteDamage", DMisl.EliteDamage);
    DMisl.BodyLength = ini->ReadInteger(section, "DMislBodyLength", DMisl.BodyLength);
    DMisl.LazyCurve = ini->ReadBool(section, "DMislLazyCurve", DMisl.LazyCurve);
    read_rule_type(*ini, section, "DMislType", AbstractType::AircraftType, DMisl.Type);
    CMisl.PauseFrames = ini->ReadInteger(section, "CMislPauseFrames", CMisl.PauseFrames);
    CMisl.TiltFrames = ini->ReadInteger(section, "CMislTiltFrames", CMisl.TiltFrames);
    CMisl.PitchInitial = static_cast<float>(ini->ReadDouble(section, "CMislPitchInitial", CMisl.PitchInitial));
    CMisl.PitchFinal = static_cast<float>(ini->ReadDouble(section, "CMislPitchFinal", CMisl.PitchFinal));
    CMisl.TurnRate = static_cast<float>(ini->ReadDouble(section, "CMislTurnRate", CMisl.TurnRate));
    CMisl.RaiseRate = rule_integer(ini->ReadDouble(section, "CMislRaiseRate", CMisl.RaiseRate));
    // Target falls back to the already-read Dreadnought missile acceleration.
    CMisl.Acceleration = static_cast<float>(ini->ReadDouble(section, "CMislAcceleration", DMisl.Acceleration));
    CMisl.Altitude = ini->ReadInteger(section, "CMislAltitude", CMisl.Altitude);
    CMisl.Damage = ini->ReadInteger(section, "CMislDamage", CMisl.Damage);
    CMisl.EliteDamage = ini->ReadInteger(section, "CMislEliteDamage", CMisl.EliteDamage);
    CMisl.BodyLength = ini->ReadInteger(section, "CMislBodyLength", CMisl.BodyLength);
    CMisl.LazyCurve = ini->ReadBool(section, "CMislLazyCurve", CMisl.LazyCurve);
    read_rule_type(*ini, section, "CMislType", AbstractType::AircraftType, CMisl.Type);
    ParadropRadius = ini->ReadInteger(section, "ParadropRadius", ParadropRadius);
    SpotlightMovementRadius = ini->ReadInteger(section, "SpotlightMovementRadius", SpotlightMovementRadius);
    SpotlightLocationRadius = ini->ReadInteger(section, "SpotlightLocationRadius", SpotlightLocationRadius);
    SpotlightSpeed = ini->ReadDouble(section, "SpotlightSpeed", SpotlightSpeed);
    SpotlightAcceleration = ini->ReadDouble(section, "SpotlightAcceleration", SpotlightAcceleration);
    SpotlightAngle = ini->ReadDouble(section, "SpotlightAngle", SpotlightAngle);
    SpotlightRadius = ini->ReadInteger(section, "SpotlightRadius", SpotlightRadius);
    RevealTriggerRadius = ini->ReadInteger(section, "RevealTriggerRadius", RevealTriggerRadius);
    ChargeToDrainRatio = ini->ReadDouble(section, "ChargeToDrainRatio", ChargeToDrainRatio);
    WallBuildSpeedCoefficient = ini->ReadDouble(section, "WallBuildSpeedCoefficient", WallBuildSpeedCoefficient);
    ConditionYellowSparkingProbability = ini->ReadDouble(section, "ConditionYellowSparkingProbability", ConditionYellowSparkingProbability);
    ConditionRedSparkingProbability = ini->ReadDouble(section, "ConditionRedSparkingProbability", ConditionRedSparkingProbability);
    AITriggerSuccessWeightDelta = ini->ReadDouble(section, "AITriggerSuccessWeightDelta", AITriggerSuccessWeightDelta);
    AITriggerFailureWeightDelta = ini->ReadDouble(section, "AITriggerFailureWeightDelta", AITriggerFailureWeightDelta);
    AITriggerTrackRecordCoefficient = ini->ReadDouble(section, "AITriggerTrackRecordCoefficient", AITriggerTrackRecordCoefficient);
    WeedCapacity = ini->ReadInteger(section, "WeedCapacity", WeedCapacity);
    FlashFrameTime = ini->ReadInteger(section, "FlashFrameTime", FlashFrameTime);
    if (!Read_Radar(ini)) return false;
    MyEffectivenessCoefficientDefault = ini->ReadDouble(section, "MyEffectivenessCoefficientDefault", MyEffectivenessCoefficientDefault);
    TargetEffectivenessCoefficientDefault = ini->ReadDouble(section, "TargetEffectivenessCoefficientDefault", TargetEffectivenessCoefficientDefault);
    TargetSpecialThreatCoefficientDefault = ini->ReadDouble(section, "TargetSpecialThreatCoefficientDefault", TargetSpecialThreatCoefficientDefault);
    TargetStrengthCoefficientDefault = ini->ReadDouble(section, "TargetStrengthCoefficientDefault", TargetStrengthCoefficientDefault);
    TargetDistanceCoefficientDefault = ini->ReadDouble(section, "TargetDistanceCoefficientDefault", TargetDistanceCoefficientDefault);
    DumbMyEffectivenessCoefficient = ini->ReadDouble(section, "DumbMyEffectivenessCoefficient", DumbMyEffectivenessCoefficient);
    DumbTargetEffectivenessCoefficient = ini->ReadDouble(section, "DumbTargetEffectivenessCoefficient", DumbTargetEffectivenessCoefficient);
    DumbTargetSpecialThreatCoefficient = ini->ReadDouble(section, "DumbTargetSpecialThreatCoefficient", DumbTargetSpecialThreatCoefficient);
    DumbTargetStrengthCoefficient = ini->ReadDouble(section, "DumbTargetStrengthCoefficient", DumbTargetStrengthCoefficient);
    DumbTargetDistanceCoefficient = ini->ReadDouble(section, "DumbTargetDistanceCoefficient", DumbTargetDistanceCoefficient);
    EnemyHouseThreatBonus = ini->ReadDouble(section, "EnemyHouseThreatBonus", EnemyHouseThreatBonus);
    VeinholeMonsterStrength = ini->ReadInteger(section, "VeinholeMonsterStrength", VeinholeMonsterStrength);
    MaxVeinholeGrowth = ini->ReadInteger(section, "MaxVeinholeGrowth", MaxVeinholeGrowth);
    VeinholeGrowthRate = ini->ReadInteger(section, "VeinholeGrowthRate", VeinholeGrowthRate);
    VeinholeShrinkRate = ini->ReadInteger(section, "VeinholeShrinkRate", VeinholeShrinkRate);
    VeinDamage = ini->ReadInteger(section, "VeinDamage", VeinDamage);
    read_rule_type(*ini, section, "VeinholeTypeClass", AbstractType::TerrainType, VeinholeTypeClass);
    read_rule_type_list(*ini, section, "DefaultMirageDisguises", AbstractType::TerrainType, DefaultMirageDisguises);
    InfantryBlinkDisguiseTime = ini->ReadInteger(section, "InfantryBlinkDisguiseTime", InfantryBlinkDisguiseTime);
    MaximumQueuedObjects = ini->ReadInteger(section, "MaximumQueuedObjects", MaximumQueuedObjects);
    MaxWaypointPathLength = ini->ReadInteger(section, "MaxWaypointPathLength", MaxWaypointPathLength);
    TreeStrength = ini->ReadInteger(section, "TreeStrength", TreeStrength);
    EngineerCaptureLevel = static_cast<float>(ini->ReadDouble(section, "EngineerCaptureLevel", EngineerCaptureLevel));
    EngineerCaptureLevel_ = static_cast<float>(ini->ReadDouble(section, "EngineerCaptureLevel", EngineerCaptureLevel_));
    TalkBubbleTime = static_cast<DWORD>(rule_scaled_integer(ini->ReadDouble(section, "TalkBubbleTime", static_cast<double>(TalkBubbleTime) * static_cast<double>(std::bit_cast<float>(0x3c888889u))), 60));
    return true;
}
