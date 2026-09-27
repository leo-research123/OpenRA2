// Existing RulesClass Read_General radar subsection, 671970..671AD7.
// Shared by full rules loading and the terrain-view startup subset.
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include "RulesClassReaders.hpp"

bool RulesClass::Read_Radar(CCINIClass* ini) noexcept {
    try {
    const char* section="General";
    if (!ini || !ini->GetSection(section)) return false;
    RadarCombatFlashTime = ini->ReadInteger(section, "RadarCombatFlashTime", RadarCombatFlashTime);
    RadarEventSpeed = static_cast<float>(ini->ReadDouble(section, "RadarEventSpeed", RadarEventSpeed));
    RadarEventRotationSpeed = static_cast<float>(ini->ReadDouble(section, "RadarEventRotationSpeed", RadarEventRotationSpeed));
    read_rule_integer_list(*ini, section, "RadarEventSuppressionDistances", RadarEventSuppressionDistances);
    read_rule_integer_list(*ini, section, "RadarEventVisibilityDurations", RadarEventVisibilityDurations);
    read_rule_integer_list(*ini, section, "RadarEventDurations", RadarEventDurations);
    RadarEventMinRadius = ini->ReadInteger(section, "RadarEventMinRadius", RadarEventMinRadius);
    RadarEventColorSpeed = static_cast<float>(ini->ReadDouble(section, "RadarEventColorSpeed", RadarEventColorSpeed));
    return true;
    } catch (...) { return false; }
}
