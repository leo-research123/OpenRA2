// YR-specific rule reader 668FB0. The existing Warhead/Bullet classes own
// allocation and registration; every registered warhead receives LoadFromINI.
#include "yrpp/RulesClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "RulesClassReaders.hpp"

bool RulesClass::Read_SpecialWeapons(CCINIClass* ini) {
    const char* section = "SpecialWeapons";
    if (!ini || !ini->GetSection(section)) return false;
    read_rule_type(*ini, section, "NukeWarhead", AbstractType::WarheadType, NukeWarhead);
    read_rule_type(*ini, section, "NukeProjectile", AbstractType::BulletType, NukeProjectile);
    read_rule_type(*ini, section, "NukeDown", AbstractType::BulletType, NukeDown);
    read_rule_type(*ini, section, "MutateWarhead", AbstractType::WarheadType, MutateWarhead);
    read_rule_type(*ini, section, "MutateExplosionWarhead", AbstractType::WarheadType, MutateExplosionWarhead);
    read_rule_type(*ini, section, "EMPulseWarhead", AbstractType::WarheadType, EMPulseWarhead);
    read_rule_type(*ini, section, "EMPulseProjectile", AbstractType::BulletType, EMPulseProjectile);
    load_rule_types(ini, AbstractType::WarheadType);
    return true;
}
