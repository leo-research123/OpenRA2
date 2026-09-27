// Original registry dispatch order at 679A10. Each owning class implements
// its own content reader; animations consume the existing Art INI instead.
#include "yrpp/RulesClass.h"
#include "yrpp/MissionClass.h"
#include "RulesClassReaders.hpp"

bool YRPP_STDCALL RulesClass::LoadTypesFromINI(CCINIClass* ini) {
    if (!ini) return false;
    load_rule_types(ini, AbstractType::HouseType);
    load_rule_types(ini, AbstractType::SuperWeaponType);
    load_rule_types(&CCINIClass::INI_Art, AbstractType::AnimType);
    for (AbstractType kind : {AbstractType::BuildingType, AbstractType::AircraftType,
            AbstractType::UnitType, AbstractType::InfantryType, AbstractType::WeaponType,
            AbstractType::BulletType, AbstractType::WarheadType})
        load_rule_types(ini, kind);
    for (int index = 0;; ++index) {
        int count = 0;
        if (!game::rules_type_count(AbstractType::WeaponType, count))
            throw std::runtime_error("RulesClass weapon enumeration dependency failed");
        if (index >= count) break;
        AbstractTypeClass* weapon = nullptr;
        const auto& runtime = game::rules_runtime();
        if (!game::rules_type_at(AbstractType::WeaponType, index, weapon) ||
            !runtime.finalize_weapon || !runtime.finalize_weapon(runtime.context, weapon))
            throw std::runtime_error("RulesClass weapon speed dependency failed");
    }
    // Original second building pass calls nullsub_7, which has no effects.
    for (AbstractType kind : {AbstractType::TerrainType, AbstractType::SmudgeType,
            AbstractType::OverlayType, AbstractType::ParticleType,
            AbstractType::ParticleSystemType, AbstractType::VoxelAnimType})
        load_rule_types(ini, kind);
    for (int index = 0; index < 32; ++index) {
        MissionControlClass::Array[index].ArrayIndex = index;
        MissionControlClass::Array[index].LoadFromINI(ini);
    }
    return true;
}
