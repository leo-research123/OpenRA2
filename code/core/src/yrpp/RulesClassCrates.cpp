/*
 * Adapts the crate fields of EA REDALERT/RULES.CPP::General, fixed revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts
 * Inc. GPL-3.0-or-later with terms in third_party/ea/LICENSE.TXT.
 * YR moves these fields to CrateRules (66B900) and adds image/sound references.
 */
#include "yrpp/RulesClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "RulesClassReaders.hpp"

bool RulesClass::Read_CrateRules(CCINIClass* ini) {
    const char* section = "CrateRules";
    if (!ini || !ini->GetSection(section)) return false;
    FreeMCV = ini->ReadBool(section, "FreeMCV", FreeMCV);
    read_rule_type(*ini, section, "WoodCrateImg", AbstractType::OverlayType, WoodCrateImg);
    read_rule_type(*ini, section, "CrateImg", AbstractType::OverlayType, CrateImg);
    read_rule_type(*ini, section, "WaterCrateImg", AbstractType::OverlayType, WaterCrateImg);
    read_rule_sound(*ini, section, "HealCrateSound", HealCrateSound);
    CrateMinimum = ini->ReadInteger(section, "CrateMinimum", CrateMinimum);
    CrateMaximum = ini->ReadInteger(section, "CrateMaximum", CrateMaximum);
    CrateRadius = read_rule_distance(*ini, section, "CrateRadius", CrateRadius);
    CrateRegen = ini->ReadDouble(section, "CrateRegen", CrateRegen);
    read_rule_type(*ini, section, "UnitCrateType", AbstractType::UnitType, UnitCrateType);
    SoloCrateMoney = ini->ReadInteger(section, "SoloCrateMoney", SoloCrateMoney);
    SilverCrate = static_cast<Powerup>(ini->ReadPowerup(section, "SilverCrate", static_cast<int>(SilverCrate)));
    WoodCrate = static_cast<Powerup>(ini->ReadPowerup(section, "WoodCrate", static_cast<int>(WoodCrate)));
    WaterCrate = static_cast<Powerup>(ini->ReadPowerup(section, "WaterCrate", static_cast<int>(WaterCrate)));
    return true;
}
