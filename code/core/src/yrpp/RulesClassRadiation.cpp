// YR-specific RulesClass::Read_Radiation, calibrated at 66CF70..66D140.
// No counterpart in pinned EA REDALERT/RULES.CPP; retain YR's existing fields.
#include "yrpp/RulesClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "RulesClassReaders.hpp"

bool RulesClass::Read_Radiation(CCINIClass* ini) {
    const char* section = "Radiation";
    if (!ini || !ini->GetSection(section)) return false;
    RadDurationMultiple = ini->ReadInteger(section, "RadDurationMultiple", RadDurationMultiple);
    RadApplicationDelay = ini->ReadInteger(section, "RadApplicationDelay", RadApplicationDelay);
    RadLevelMax = ini->ReadInteger(section, "RadLevelMax", RadLevelMax);
    RadLevelDelay = ini->ReadInteger(section, "RadLevelDelay", RadLevelDelay);
    RadLightDelay = ini->ReadInteger(section, "RadLightDelay", RadLightDelay);
    RadLevelFactor = ini->ReadDouble(section, "RadLevelFactor", RadLevelFactor);
    RadLightFactor = ini->ReadDouble(section, "RadLightFactor", RadLightFactor);
    RadTintFactor = ini->ReadDouble(section, "RadTintFactor", RadTintFactor);
    auto* color = reinterpret_cast<byte*>(&RadColor);
    ini->Read3Bytes(color, section, "RadColor", color);
    read_rule_type(*ini, section, "RadSiteWarhead", AbstractType::WarheadType, RadSiteWarhead);
    return true;
}
