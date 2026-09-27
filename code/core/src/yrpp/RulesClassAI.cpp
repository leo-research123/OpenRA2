/*
 * Adapts EA REDALERT/RULES.CPP (IQ, Difficulty_Get, Difficulty), revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
 * Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later with the additional
 * terms in third_party/ea/LICENSE.TXT. YR double fields and return semantics
 * calibrated to 674240, 66D270 and 674500.
 */
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "RulesClassReaders.hpp"

bool RulesClass::Read_IQ(CCINIClass* ini) {
    const char* section = "IQ";
    if (!ini || !ini->GetSection(section)) return false;
    MaxIQLevels = ini->ReadInteger(section, "MaxIQLevels", MaxIQLevels);
    SuperWeapons = ini->ReadInteger(section, "SuperWeapons", SuperWeapons);
    Production = ini->ReadInteger(section, "Production", Production);
    GuardArea = ini->ReadInteger(section, "GuardArea", GuardArea);
    RepairSell = ini->ReadInteger(section, "RepairSell", RepairSell);
    AutoCrush = ini->ReadInteger(section, "AutoCrush", AutoCrush);
    Scatter = ini->ReadInteger(section, "Scatter", Scatter);
    ContentScan = ini->ReadInteger(section, "ContentScan", ContentScan);
    Aircraft = ini->ReadInteger(section, "Aircraft", Aircraft);
    Harvester = ini->ReadInteger(section, "Harvester", Harvester);
    SellBack = ini->ReadInteger(section, "SellBack", SellBack);
    return true;
}

bool YRPP_FASTCALL RulesClass::Read_Difficulty(CCINIClass* ini,
        DifficultyStruct* output, const char* section) {
    if (!ini || !output || !section || !ini->GetSection(section)) return false;
    auto& difficulty = *output;
    // A present section resets missing keys to these fixed defaults, rather
    // than inheriting the current difficulty as most Rules readers do.
    // INI lookup is case-sensitive: retain FirePower, Groundspeed and Airspeed.
    difficulty.Firepower = ini->ReadDouble(section, "FirePower", 1.0);
    difficulty.GroundSpeed = ini->ReadDouble(section, "Groundspeed", 1.0);
    difficulty.AirSpeed = ini->ReadDouble(section, "Airspeed", 1.0);
    difficulty.Armor = ini->ReadDouble(section, "Armor", 1.0);
    difficulty.ROF = ini->ReadDouble(section, "ROF", 1.0);
    difficulty.Cost = ini->ReadDouble(section, "Cost", 1.0);
    difficulty.RepairDelay = ini->ReadDouble(section, "RepairDelay", 0.02);
    difficulty.BuildDelay = ini->ReadDouble(section, "BuildDelay", 0.03);
    difficulty.BuildSlowdown = ini->ReadBool(section, "BuildSlowdown", false);
    difficulty.BuildTime = ini->ReadDouble(section, "BuildTime", 1.0);
    difficulty.DestroyWalls = ini->ReadBool(section, "DestroyWalls", true);
    difficulty.ContentScan = ini->ReadBool(section, "ContentScan", false);
    return difficulty.ContentScan;
}

bool RulesClass::Read_Difficulties(CCINIClass* ini) {
    if (!ini) return false;
    Read_Difficulty(ini, &Easy, "Easy");
    Read_Difficulty(ini, &Normal, "Normal");
    Read_Difficulty(ini, &Difficult, "Difficult");
    return true;
}

// EA RulesClass::AI adapted to YR 672AE0: type lists and all scalar reads
// retain target order, spelling and current-value defaults.
bool RulesClass::Read_AI(CCINIClass* ini) {
    const char* section = "AI";
    if (!ini || !ini->GetSection(section)) return false;
    read_rule_type_list(*ini, section, "BuildConst", AbstractType::BuildingType, BuildConst);
    read_rule_type_list(*ini, section, "BuildPower", AbstractType::BuildingType, BuildPower);
    read_rule_type_list(*ini, section, "BuildRefinery", AbstractType::BuildingType, BuildRefinery);
    read_rule_type_list(*ini, section, "BuildBarracks", AbstractType::BuildingType, BuildBarracks);
    read_rule_type_list(*ini, section, "BuildTech", AbstractType::BuildingType, BuildTech);
    read_rule_type_list(*ini, section, "BuildWeapons", AbstractType::BuildingType, BuildWeapons);
    read_rule_type_list(*ini, section, "AlliedBaseDefenses", AbstractType::BuildingType, AlliedBaseDefenses);
    read_rule_type_list(*ini, section, "SovietBaseDefenses", AbstractType::BuildingType, SovietBaseDefenses);
    read_rule_type_list(*ini, section, "ThirdBaseDefenses", AbstractType::BuildingType, ThirdBaseDefenses);
    read_rule_integer_list(*ini, section, "AIForcePredictionFudge", AIForcePredictionFudge);
    read_rule_type_list(*ini, section, "BuildDefense", AbstractType::BuildingType, BuildDefense);
    read_rule_type_list(*ini, section, "BuildPDefense", AbstractType::BuildingType, BuildPDefense);
    read_rule_type_list(*ini, section, "BuildAA", AbstractType::BuildingType, BuildAA);
    read_rule_type_list(*ini, section, "BuildHelipad", AbstractType::BuildingType, BuildHelipad);
    read_rule_type_list(*ini, section, "BuildRadar", AbstractType::BuildingType, BuildRadar);
    read_rule_type_list(*ini, section, "ConcreteWalls", AbstractType::BuildingType, ConcreteWalls);
    read_rule_type_list(*ini, section, "NSGates", AbstractType::BuildingType, NSGates);
    read_rule_type_list(*ini, section, "EWGates", AbstractType::BuildingType, EWGates);
    read_rule_type_list(*ini, section, "BuildNavalYard", AbstractType::BuildingType, BuildNavalYard);
    read_rule_type_list(*ini, section, "BuildDummy", AbstractType::BuildingType, BuildDummy);
    read_rule_type_list(*ini, section, "NeutralTechBuildings", AbstractType::BuildingType, NeutralTechBuildings);
    AttackInterval = ini->ReadDouble(section, "AttackInterval", AttackInterval);
    AttackDelay = ini->ReadDouble(section, "AttackDelay", AttackDelay);
    PatrolScan = ini->ReadDouble(section, "PatrolScan", PatrolScan);
    CreditReserve = ini->ReadInteger(section, "CreditReserve", CreditReserve);
    PathDelay = ini->ReadDouble(section, "PathDelay", PathDelay);
    BlockagePathDelay = ini->ReadInteger(section, "BlockagePathDelay", BlockagePathDelay);
    AutocreateTime = ini->ReadDouble(section, "AutocreateTime", AutocreateTime);
    InfantryReserve = ini->ReadInteger(section, "InfantryReserve", InfantryReserve);
    InfantryBaseMult = ini->ReadInteger(section, "InfantryBaseMult", InfantryBaseMult);
    PowerSurplus = ini->ReadInteger(section, "PowerSurplus", PowerSurplus);
    BaseSizeAdd = ini->ReadInteger(section, "BaseSizeAdd", BaseSizeAdd);
    RefineryRatio = ini->ReadDouble(section, "RefineryRatio", RefineryRatio);
    RefineryLimit = ini->ReadInteger(section, "RefineryLimit", RefineryLimit);
    BarracksRatio = ini->ReadDouble(section, "BarracksRatio", BarracksRatio);
    BarracksLimit = ini->ReadInteger(section, "BarracksLimit", BarracksLimit);
    WarRatio = ini->ReadDouble(section, "WarRatio", WarRatio);
    WarLimit = ini->ReadInteger(section, "WarLimit", WarLimit);
    DefenseRatio = ini->ReadDouble(section, "DefenseRatio", DefenseRatio);
    DefenseLimit = ini->ReadInteger(section, "DefenseLimit", DefenseLimit);
    AARatio = ini->ReadDouble(section, "AARatio", AARatio);
    AALimit = ini->ReadInteger(section, "AALimit", AALimit);
    TeslaRatio = ini->ReadDouble(section, "TeslaRatio", TeslaRatio);
    TeslaLimit = ini->ReadInteger(section, "TeslaLimit", TeslaLimit);
    HelipadRatio = ini->ReadDouble(section, "HelipadRatio", HelipadRatio);
    HelipadLimit = ini->ReadInteger(section, "HelipadLimit", HelipadLimit);
    AirstripRatio = ini->ReadDouble(section, "AirstripRatio", AirstripRatio);
    AirstripLimit = ini->ReadInteger(section, "AirstripLimit", AirstripLimit);
    CompEasyBonus = ini->ReadBool(section, "CompEasyBonus", CompEasyBonus);
    Paranoid = ini->ReadBool(section, "Paranoid", Paranoid);
    PowerEmergency = ini->ReadDouble(section, "PowerEmergency", PowerEmergency);
    AIBaseSpacing = ini->ReadInteger(section, "AIBaseSpacing", AIBaseSpacing);
    GDIWallDefense = ini->ReadDouble(section, "GDIWallDefense", GDIWallDefense);
    GDIWallDefenseCoefficient = ini->ReadDouble(section, "GDIWallDefenseCoefficient", GDIWallDefenseCoefficient);
    NodBaseDefenseCoefficient = ini->ReadDouble(section, "NodBaseDefenseCoefficient", NodBaseDefenseCoefficient);
    GDIBaseDefenseCoefficient = ini->ReadDouble(section, "GDIBaseDefenseCoefficient", GDIBaseDefenseCoefficient);
    MaximumBaseDefenseValue = ini->ReadInteger(section, "MaximumBaseDefenseValue", MaximumBaseDefenseValue);
    ComputerBaseDefenseResponse = ini->ReadInteger(section, "ComputerBaseDefenseResponse", ComputerBaseDefenseResponse);
    return true;
}
